// VEX workbench controller. Runs the shared C++ firmware in vex.wasm one millisecond per step,
// drives the 8-step Practice session (session.js), the 30 s Activity strip, the Screens gallery
// (gallery.js + scenario.js) and the collapsible Inspect drawer (including the Teensy mirror).
'use strict';
const $=id=>document.getElementById(id);
const {BLOCK,RATE,BLOCK_MS}=window.VexScenario;
const POSES=['idle','blink','strum','cheer','levelup','listen','sleep'];
const STRIP_STEP_MS=50,STRIP_LEN=600;   // 600 * 50 ms = 30.0 s

const sim={
 now:0,stage:4,speed:1,paused:false,displayMs:100,
 inputMode:'quiet',
 raw:{mode:0,quest:0,foot:0},
 inject:{rms:0.001,peak:0.002,hz:0,prob:0.99,on:true},
 source:null,audioClock:0,carry:0,lastDraw:-1000,dirty:true,
 lastNotice:'',notices:[],telemetry:[],logDirty:false,
 record:[],span:null,recDirty:false,
 lastState:null,testPattern:false,
 strip:[],stripHead:0,stripDirty:true,lastEventSig:'',
 clipRec:{active:false,chunks:[],samples:0,maxSamples:RATE*30,blobUrl:null}
};

let M,audioPtr,storeBytes,image,ctx,stripCtx,sessionCtrl,galleryCtrl,stripIdx;

function fmtClock(ms){
 const s=Math.floor(ms/1000);
 return `${String(Math.floor(s/60)).padStart(2,'0')}:${String(s%60).padStart(2,'0')}.${String(ms%1000).padStart(3,'0')}`;
}

function readState(){
 return window.VexScenario.readModuleState(M);
}

// Fast zero-alloc sample every 50 simulated ms for the 30 s activity strip.
function clearStrip(){sim.strip.length=0;sim.stripHead=0;sim.stripDirty=true;sim.lastEventSig='';}
function sampleStrip(){
 const p=M._vex_state()>>3;
 const gate=M.HEAPF64[p+stripIdx.gate],rms=M.HEAPF64[p+stripIdx.rms],peak=M.HEAPF64[p+stripIdx.peak];
 const active=M.HEAPF64[p+stripIdx.active]!==0,env=M.HEAPF64[p+stripIdx.env],hold=M.HEAPF64[p+stripIdx.hold_ms];
 const item={at:sim.now,rms,env,peak,gate,active,holding:active&&rms<=gate&&hold>0,clipped:peak>=0.98};
 if(sim.strip.length<STRIP_LEN)sim.strip.push(item);
 else{sim.strip[sim.stripHead]=item;sim.stripHead=(sim.stripHead+1)%STRIP_LEN;}
 sim.stripDirty=true;
}

// Device lifecycle -------------------------------------------------------------------------------
function boot(fromRebootButton){
 sim.now=0;sim.audioClock=0;M._vex_boot(sim.stage,0);
 sim.dirty=true;sim.lastDraw=-1000;sim.lastNotice='';
 clearStrip();
 if(fromRebootButton&&sessionCtrl)sessionCtrl.onReboot();
 note(`boot stage ${sim.stage}`);
}
function factoryReset(){
 M._vex_reset();persist();
 setTestPattern(false);
 sim.stage=4;$('stage').value='4';
 sim.now=0;sim.audioClock=0;sim.dirty=true;sim.lastDraw=-1000;sim.lastNotice='';
 clearStrip();note('boot stage 4');
 if(sessionCtrl)sessionCtrl.resetProgress();
}
function setTestPattern(on){
 sim.testPattern=!!on;M._vex_test(sim.testPattern?1:0);
 if($('testpat'))$('testpat').checked=sim.testPattern;
 sim.dirty=true;
}
function setPaused(p){
 sim.paused=!!p;
 $('pause').textContent=sim.paused?'Run':'Pause';
 $('pause').classList.toggle('on',sim.paused);
}
function togglePause(){setPaused(!sim.paused);}
function note(line){
 sim.telemetry.push(`[${fmtClock(sim.now)}] ${line}`);
 if(sim.telemetry.length>400)sim.telemetry.splice(0,sim.telemetry.length-400);
 sim.logDirty=true;
}
function persist(){try{localStorage.setItem('vex.store',btoa(String.fromCharCode(...storeBytes)));}catch(e){}}
function restoreStore(){
 try{
  const s=localStorage.getItem('vex.store');if(!s)return;
  const b=atob(s);if(b.length!==128&&b.length!==storeBytes.length)return;
  storeBytes.fill(0xff);
  for(let i=0;i<b.length;i++)storeBytes[i]=b.charCodeAt(i);
  M._vex_store_load();
 }catch(e){}
}

// One simulated millisecond -----------------------------------------------------------------------
function stopClipRecording(){
 const cr=sim.clipRec;if(!cr.active)return;
 cr.active=false;
 const btn=$('clip-rec');btn.classList.remove('on');btn.textContent='Record clip';
 if(!cr.samples){$('clip-info').textContent='0 samples captured';return;}
 const pcm=new Int16Array(cr.samples);let off=0;
 for(const ch of cr.chunks){pcm.set(ch,off);off+=ch.length;}
 cr.chunks=[];
 const wavBytes=window.VexScenario.encodeWav16Mono(pcm,RATE);
 const rawName=($('clip-name').value||'take_01').trim().replace(/[^a-zA-Z0-9_-]/g,'_')||'take_01';
 const fileName=rawName.endsWith('.wav')?rawName:`${rawName}.wav`;
 if(cr.blobUrl)URL.revokeObjectURL(cr.blobUrl);
 cr.blobUrl=URL.createObjectURL(new Blob([wavBytes],{type:'audio/wav'}));
 window.__lastClipWav={fileName,samples:cr.samples,durationMs:Math.round(cr.samples*1000/RATE),bytes:wavBytes.byteLength};
 const dl=$('clip-dl');dl.href=cr.blobUrl;dl.download=fileName;dl.textContent=`${fileName} (${(cr.samples/RATE).toFixed(1)}s)`;dl.hidden=false;
 $('clip-info').textContent=`saved ${fileName} · ${(cr.samples/RATE).toFixed(2)} s`;
 try{dl.click();}catch(_){}
 note(`recorded clip ${fileName} (${cr.samples} samples, ${wavBytes.byteLength} B)`);
}
function toggleClipRecording(){
 const cr=sim.clipRec;
 if(cr.active){stopClipRecording();return;}
 cr.active=true;cr.chunks=[];cr.samples=0;
 const btn=$('clip-rec');btn.classList.add('on');btn.textContent='Stop & save WAV';
 $('clip-dl').hidden=true;$('clip-info').textContent='recording 44.1 kHz PCM…';
 note('clip recording armed');
}
function pushBlock(block,now){
 if(sim.clipRec.active){
  const copy=new Int16Array(block);
  sim.clipRec.chunks.push(copy);sim.clipRec.samples+=copy.length;
  if(sim.clipRec.samples>=sim.clipRec.maxSamples)stopClipRecording();
 }
 M.HEAP16.set(block,audioPtr>>1);
 M._vex_audio_push(BLOCK,now);
}
function step(){
 const now=++sim.now;
 if(sim.source){
  if(sim.source.kind==='mic'){let b;while((b=sim.source.next()))pushBlock(b,now);}
  else while(sim.audioClock<=now){
   const b=sim.source.next();
   if(!b){stopSource('finished');break;}
   pushBlock(b,now);sim.audioClock+=BLOCK_MS;
  }
 }else if(sim.inject.on){
  M._vex_observe(sim.inject.rms,sim.inject.peak,now);
  if(sim.inject.hz>0)M._vex_pitch(sim.inject.hz,sim.inject.prob,now);
 }
 M._vex_controls(sim.raw.mode,sim.raw.quest,sim.raw.foot,now);
 M._vex_tick(now);
 if(now%STRIP_STEP_MS===0)sampleStrip();
 if(now%1000===0)telemetryLine();
}
function run(ms){for(let i=0;i<ms;i++)step();sim.dirty=true;}
function telemetryLine(){
 const s=readState();
 note(`stage=${s.stage} act=${s.activity} onsets=${s.onsets} bpm=${s.bpm} rms=${s.rms.toFixed(5)} env=${s.env.toFixed(5)} peak=${s.peak.toFixed(3)} gate=${s.gate.toFixed(5)} active=${+s.active} hold_ms=${s.hold_ms} running=${+s.running} session_s=${Math.floor(s.session_ms/1000)} total_s=${Math.floor(s.active_ms/1000)} fragments=${s.fragments} drops=${s.credit_drops} pitch=yin hz=${s.hz.toFixed(2)} prob=${s.probability.toFixed(2)} yin_us=${M._vex_last_analysis_us()}`);
}

// Audio sources -----------------------------------------------------------------------------------
function synthSource(){
 const buf=new Int16Array(BLOCK);let phase=0,t=0;
 return {
  kind:'synth',
  label:'Synth note through 44.1 kHz analyzer',
  next(){
   const hz=Number($('syn-note').value)*Math.pow(2,Number($('syn-cents').value)/1200);
   const amp=Math.pow(10,Number($('syn-db').value)/20);
   const decay=$('syn-decay').checked;
   for(let i=0;i<BLOCK;i++){
    const env=decay?Math.exp(-((t%2)*2.2)):1;
    t+=1/RATE;phase+=2*Math.PI*hz/RATE;if(phase>2*Math.PI)phase-=2*Math.PI;
    const v=amp*env*(Math.sin(phase)+0.45*Math.sin(2*phase)+0.15*Math.sin(3*phase))/1.6+(Math.random()-0.5)*3e-4;
    buf[i]=Math.max(-32768,Math.min(32767,Math.round(v*32767)));
   }
   return buf;
  }
 };
}

let wavPromise=null;
function loadWav(){
 if(!wavPromise){
  wavPromise=fetch('guitar_notes_left_only.wav').then(r=>{
   if(!r.ok)throw Error('failed to fetch guitar_notes_left_only.wav');
   return r.arrayBuffer();
  }).then(ab=>window.VexScenario.parseWavLeft16(ab));
 }
 return wavPromise;
}
function wavSource(pcm){
 const buf=new Int16Array(BLOCK);let pos=0;
 return {
  kind:'wav',
  label:'24 s reference WAV through analyzer',
  get pos(){return pos;},
  total:pcm.length,
  next(){
   if(pos>=pcm.length)return null;
   buf.fill(0);const n=Math.min(BLOCK,pcm.length-pos);
   buf.set(pcm.subarray(pos,pos+n));pos+=n;return buf;
  }
 };
}

let mic=null;
async function micSource(){
 const stream=await navigator.mediaDevices.getUserMedia({audio:{echoCancellation:false,noiseSuppression:false,autoGainControl:false,channelCount:1}});
 const ac=new AudioContext({sampleRate:RATE,latencyHint:'interactive'});
 await ac.audioWorklet.addModule('worklet.js');
 const node=new AudioWorkletNode(ac,'vex-capture',{numberOfInputs:1,numberOfOutputs:0});
 const queue=[];node.port.onmessage=e=>{if(queue.length<256)queue.push(e.data);};
 ac.createMediaStreamSource(stream).connect(node);
 const label=stream.getAudioTracks()[0].label||'input';
 mic={stream,ctx:ac,node};
 $('mic-info').textContent=`${label} · ${ac.sampleRate} Hz · ${(ac.baseLatency*1000).toFixed(1)} ms`;
 if(ac.sampleRate!==RATE)$('mic-info').textContent+=' · WARNING: not 44.1 kHz';
 return {kind:'mic',label:`Mac input (${label})`,next(){return queue.shift()||null;}};
}
function stopMic(){
 if(!mic)return;
 mic.stream.getTracks().forEach(t=>t.stop());mic.ctx.close();mic=null;
 $('mic-info').textContent='';$('mic-start').textContent='Start Mac input';
}

function setSource(src){
 if(sim.source&&sim.source.kind==='mic'&&(!src||src.kind!=='mic'))stopMic();
 closeSpan();
 sim.source=src;sim.audioClock=sim.now;
 if(src)recordComment(`live analyzer source: ${src.label}`);
}
function stopSource(reason){
 if(!sim.source)return;
 const k=sim.source.kind;setSource(null);
 note(`audio source ${k} ${reason||'stopped'}`);
 if(k==='wav')$('wav-pos').textContent='';
 if(sim.inputMode==='wav'||sim.inputMode==='mic')setInputMode('quiet');
}

const INPUT_LABELS={
 quiet:'Not playing (quiet cable floor)',
 play:'Playing (steady signal)',
 strum:'Strumming (plucks + 280 ms chord gaps)',
 hot:'Too hot (clipping, peak 1.00)',
 unplugged:'Cable unplugged (no fresh samples)',
 synth:'Synth note through 44.1 kHz analyzer',
 wav:'24 s reference WAV through analyzer',
 mic:'Mac / audio interface',
 custom:'Custom synthetic observations'
};

function setInputMode(mode,opts){
 sim.inputMode=mode;
 for(const b of document.querySelectorAll('[data-input]'))b.classList.toggle('sel',b.dataset.input===mode);
 for(const k of ['synth','wav','mic','custom'])$('src-'+k).hidden=mode!==k;
 if(mode==='quiet'){setSource(null);sim.inject={rms:0.001,peak:0.002,hz:0,prob:0.99,on:true};}
 else if(mode==='play'){setSource(null);sim.inject={rms:0.1,peak:0.3,hz:0,prob:0.99,on:true};}
 else if(mode==='strum'){sim.inject.on=false;setSource(window.VexScenario.createStrumSource({chordNotes:true}));}
 else if(mode==='hot'){setSource(null);sim.inject={rms:0.35,peak:1.0,hz:0,prob:0.99,on:true};}
 else if(mode==='unplugged'){setSource(null);sim.inject={rms:0,peak:0,hz:0,prob:0.99,on:false};}
 else if(mode==='synth'){
  if(opts){
   if(opts.hz!==undefined)$('syn-note').value=String(opts.hz);
   if(opts.cents!==undefined){$('syn-cents').value=String(opts.cents);$('syn-cents-t').textContent=`${opts.cents>=0?'+':''}${opts.cents} c`;}
   if(opts.db!==undefined){$('syn-db').value=String(opts.db);$('syn-db-t').textContent=`${opts.db} dBFS`;}
  }
  sim.inject.on=false;setSource(synthSource());
 }else if(mode==='wav'){
  sim.inject.on=false;setSource(null);
  loadWav().then(pcm=>{if(sim.inputMode==='wav')setSource(wavSource(pcm));}).catch(e=>note('WAV failed: '+e.message));
 }else if(mode==='mic'){
  sim.inject.on=false;setSource(null);
 }else if(mode==='custom'){
  setSource(null);applyCustomInject();
 }
 $('audio-status').textContent=INPUT_LABELS[mode]||mode;
 closeSpan();
}
function applyCustomInject(){
 sim.inject={
  rms:Number($('inj-rms').value),peak:Number($('inj-peak').value),
  hz:Number($('inj-hz').value),prob:Number($('inj-prob').value),on:true
 };
 closeSpan();
}

// Controls ----------------------------------------------------------------------------------------
const heldSince={};
function setControl(name,down){
 if(sim.raw[name]===(down?1:0))return;
 sim.raw[name]=down?1:0;
 const el=document.querySelector(`[data-control=${name}]`);
 if(el)el.classList.toggle('down',down);
 if(down)heldSince[name]=sim.now;
 closeSpan();
}
function pressSync(name,ms){
 setControl(name,true);run(ms);setControl(name,false);run(30);
}
for(const el of document.querySelectorAll('[data-control]')){
 const name=el.dataset.control;
 el.addEventListener('pointerdown',e=>{e.preventDefault();if(el.setPointerCapture)try{el.setPointerCapture(e.pointerId);}catch(_){}setControl(name,true);});
 for(const ev of ['pointerup','pointercancel','lostpointercapture'])el.addEventListener(ev,()=>setControl(name,false));
 el.addEventListener('keydown',e=>{if(e.key==='Enter'){e.preventDefault();setControl(name,true);}});
 el.addEventListener('keyup',e=>{if(e.key==='Enter')setControl(name,false);});
}
const KEYS={m:'mode',q:'quest',' ':'foot'};
function typing(e){return /^(INPUT|TEXTAREA|SELECT)$/.test(e.target.tagName)||e.target.isContentEditable;}
document.addEventListener('keydown',e=>{
 if(typing(e)||e.metaKey||e.ctrlKey||e.altKey)return;
 const k=e.key===' '?' ':e.key.toLowerCase();
 if(KEYS[k]){e.preventDefault();if(!e.repeat)setControl(KEYS[k],true);}
 else if(k==='p')togglePause();
});
document.addEventListener('keyup',e=>{
 const k=e.key===' '?' ':e.key.toLowerCase();
 if(KEYS[k]&&!typing(e))setControl(KEYS[k],false);
});
window.addEventListener('blur',()=>{for(const n in sim.raw)setControl(n,false);});

// Scenario recorder -------------------------------------------------------------------------------
function spanKey(){
 const c=sim.raw.mode?'mode':sim.raw.quest?'quest':sim.raw.foot?'foot':'';
 const inj=!sim.source&&sim.inject.on?`${sim.inject.rms} ${sim.inject.peak}${sim.inject.hz>0?` ${sim.inject.hz} ${sim.inject.prob}`:''}`:'';
 return c+'|'+inj;
}
function closeSpan(){
 const key=spanKey();
 if(sim.span&&sim.span.key===key)return;
 if(sim.span){const ms=sim.now-sim.span.at;if(ms>0)sim.record.push({at:sim.span.at,ms,key:sim.span.key});}
 sim.span={at:sim.now,key};sim.recDirty=true;
}
function recordCommand(line){closeSpan();sim.record.push({at:sim.now,cmd:line});sim.span={at:sim.now,key:spanKey()};sim.recDirty=true;}
function recordComment(text){if(text)recordCommand('# '+text);}
function scenarioText(){
 closeSpan();const out=[];let pendingRelease=0;
 for(const r of sim.record){
  if(r.cmd){out.push(r.cmd);continue;}
  const [ctl,inj]=r.key.split('|');let ms=r.ms;
  if(ctl){out.push(`press ${ctl} ${ms}`);pendingRelease=30;continue;}
  ms-=pendingRelease;pendingRelease=0;if(ms<=0)continue;
  out.push(inj?`audio ${ms} ${inj}`:`wait ${ms}`);
 }
 return out.join('\n');
}

// 30 s Activity strip + Note lane renderer --------------------------------------------------------
function drawActivityStrip(s){
 if(!stripCtx)return;
 const c=$('activity-strip'),w=c.width,h=c.height,laneH=20,plotH=h-laneH;
 stripCtx.clearRect(0,0,w,h);
 const n=sim.strip.length;
 const at=i=>sim.strip.length<STRIP_LEN?sim.strip[i]:sim.strip[(sim.stripHead+i)%STRIP_LEN];
 const yOf=v=>laneH+plotH-6-Math.sqrt(Math.min(1,Math.max(0,v)/0.16))*(plotH-11);

 // Dedicated top note lane background + separator.
 stripCtx.fillStyle='#16131d';stripCtx.fillRect(0,0,w,laneH);
 stripCtx.fillStyle='#2a2533';stripCtx.fillRect(0,laneH-1,w,1);

 // 5-second vertical grid lines across the 30 s window (100 samples = 5 s).
 stripCtx.strokeStyle='#24202b';stripCtx.lineWidth=1;
 for(let g=1;g<6;g++){const gx=Math.round(w*g/6)+0.5;stripCtx.beginPath();stripCtx.moveTo(gx,0);stripCtx.lineTo(gx,h);stripCtx.stroke();}

 let creditedCount=0,holdCount=0;
 const colW=w/STRIP_LEN;
 const startCol=STRIP_LEN-n;
 for(let i=0;i<n;i++){
  const p=at(i),x=(startCol+i)*colW;
  if(p.holding){
   holdCount++;
   stripCtx.fillStyle='rgba(201,162,74,0.28)';stripCtx.fillRect(x,laneH,colW+0.6,plotH);
   stripCtx.fillStyle='#c9a24a';stripCtx.fillRect(x,h-3,colW+0.6,3);
  }else if(p.active){
   creditedCount++;
   stripCtx.fillStyle='rgba(111,174,122,0.18)';stripCtx.fillRect(x,laneH,colW+0.6,plotH);
   stripCtx.fillStyle='#6fae7a';stripCtx.fillRect(x,h-3,colW+0.6,3);
  }
  if(p.clipped){stripCtx.fillStyle='#c2425c';stripCtx.fillRect(x,laneH,colW+0.6,3);}
 }

 // Gate line.
 if(s.gate>0){
  const gy=Math.round(yOf(s.gate))+0.5;
  stripCtx.save();stripCtx.strokeStyle='#6fae7a';stripCtx.setLineDash([4,4]);stripCtx.lineWidth=1;
  stripCtx.beginPath();stripCtx.moveTo(0,gy);stripCtx.lineTo(w,gy);stripCtx.stroke();
  stripCtx.restore();
 }

 if(n>1){
  // Raw block RMS trace (dim).
  stripCtx.strokeStyle='#9a9199';stripCtx.lineWidth=1;stripCtx.beginPath();
  for(let i=0;i<n;i++){const x=(startCol+i+0.5)*colW,y=yOf(at(i).rms);i?stripCtx.lineTo(x,y):stripCtx.moveTo(x,y);}
  stripCtx.stroke();
  // 40 ms smoothed level trace (cream).
  stripCtx.strokeStyle='#ece6da';stripCtx.lineWidth=1.6;stripCtx.beginPath();
  for(let i=0;i<n;i++){const x=(startCol+i+0.5)*colW,y=yOf(at(i).env);i?stripCtx.lineTo(x,y):stripCtx.moveTo(x,y);}
  stripCtx.stroke();
 }

 // Note lane: onset ticks and settled note names across the trailing 30 s window.
 const winSpanMs=STRIP_LEN*STRIP_STEP_MS,winStartMs=sim.now-winSpanMs;
 const events=window.VexScenario.readRecentEvents(M).filter(e=>e.onsetMs>=winStartMs&&e.onsetMs<=sim.now+50);
 stripCtx.font='10px ui-monospace,SFMono-Regular,Menlo,monospace';
 stripCtx.fillStyle='#6f6774';stripCtx.fillText('NOTES',4,13);
 let lastLabelRight=38;
 const visibleNotes=[];
 for(const ev of events){
  const x=Math.max(1,Math.min(w-2,((ev.onsetMs-winStartMs)/winSpanMs)*w));
  stripCtx.fillStyle=ev.legato?'#c2425c':'#e8c468';
  stripCtx.fillRect(Math.round(x),2,1.6,laneH-3);
  stripCtx.fillStyle='rgba(232,196,104,0.22)';
  stripCtx.fillRect(Math.round(x),laneH,1,plotH-3);
  const tag=ev.pitched&&ev.note?ev.note:'';
  if(tag)visibleNotes.push(tag);
  if(tag&&x+3>=lastLabelRight&&x<w-18){
   stripCtx.fillStyle=ev.legato?'#f0a4b2':'#ece6da';
   stripCtx.fillText(tag,Math.round(x)+3,13);
   lastLabelRight=x+3+tag.length*6.4+3;
  }
 }

 c.dataset.totalSamples=String(n);
 c.dataset.creditedSamples=String(creditedCount);
 c.dataset.holdSamples=String(holdCount);
 c.dataset.noteEvents=String(events.length);
 c.dataset.lastNotes=visibleNotes.slice(-12).join(',');

 let label;
 if(s.stage!==4)label=`stage ${s.stage} · practice inactive`;
 else if(s.calibrating)label='calibrating noise floor…';
 else if(!s.calibrated)label='uncalibrated · hold QUEST';
 else if(s.tuning)label='tuner on · credit paused';
 else if(!s.running)label='paused';
 else if(s.peak>=0.98)label='clipped · hold cut';
 else if(s.active&&s.rms<=s.gate&&s.hold_ms>0)label=`500 ms hold (${s.hold_ms} ms left)`;
 else if(s.active)label=`credited · smooth ${s.env.toFixed(4)} > gate ${s.gate.toFixed(4)}`;
 else label=`waiting · smooth ${s.env.toFixed(4)} ≤ gate ${s.gate.toFixed(4)}`;
 $('strip-status').textContent=label;
 sim.stripDirty=false;
}

// Panels & EEPROM ---------------------------------------------------------------------------------
function crc32(bytes,n){let c=0xffffffff;for(let i=0;i<n;i++){c^=bytes[i];for(let j=0;j<8;j++)c=(c>>>1)^(0xedb88320&-(c&1));}return (~c)>>>0;}
function slotHtml(i,s){
 const b=storeBytes.subarray(64*i,64*i+48),dv=new DataView(b.buffer,b.byteOffset,48);
 const magic=dv.getUint32(0,true),version=dv.getUint32(4,true),seq=dv.getUint32(8,true),frag=dv.getUint32(12,true),goal=dv.getUint32(16,true),claimed=dv.getUint32(20,true);
 const active=Number(dv.getBigUint64(24,true)),session=Number(dv.getBigUint64(32,true)),sum=dv.getUint32(40,true);
 const valid=magic===0x56455831&&version===1&&goal<3&&claimed<2&&sum===crc32(b,40);
 let hex='';for(let r=0;r<4;r++)hex+=Array.from(storeBytes.subarray(64*i+r*16,64*i+r*16+16),x=>x.toString(16).padStart(2,'0')).join(' ')+'\n';
 return `<h4>slot ${i} ${s.saved_slot===i?'· loaded':''} · ${valid?'valid':'invalid'}</h4><div class="mono">seq ${seq} · frag ${frag} · goal ${goal} · claimed ${claimed}<br>active ${active} ms · session ${session} ms</div><pre style="max-height:none;margin:6px 0 0">${hex}</pre>`;
}

function skillsSlotHtml(i,s){
 const off=128+64*i,b=storeBytes.subarray(off,off+64),dv=new DataView(b.buffer,b.byteOffset,64);
 const magic=dv.getUint32(0,true),ver=dv.getUint32(4,true),seq=dv.getUint32(8,true),sum=dv.getUint32(60,true);
 const valid=magic===0x56585331&&ver===1&&sum===crc32(b,60);
 const defs=[['pulse','pulse-qtr'],['hunt','hunt-str'],['bend','bend-half'],['run','run-penta'],['touch','touch-acc3']];
 const fmt=window.VexSession&&window.VexSession.fmtBest?window.VexSession.fmtBest:(_,v)=>String(v);
 const stats=defs.map(([n,act],k)=>{const v=dv.getUint16(12+k*6,true);return `${n}:${v?fmt(act,v):'—'}`;}).join(' · ');
 return `<h4>skills ${i} (@${off}) ${s.skills_slot===i?'· loaded':''} · ${valid?'valid':'empty/invalid'}</h4><div class="mono">seq ${seq} · ${stats}</div>`;
}

let lastStoreSig='',lastPanelAt=0;
function updatePanels(force){
 const s=readState();sim.lastState=s;
 $('clock').textContent=fmtClock(sim.now);
 $('led').classList.toggle('on',!!s.led);
 $('pose').textContent=sim.testPattern?'test pattern':(s.stage===4?`pose ${POSES[s.anim]||'?'}/${s.frame}`:'');
 $('notice').textContent=s.calibrating?'MUTE STRINGS...':s.notice;
 if(s.notice!==sim.lastNotice){
  sim.lastNotice=s.notice;sim.notices.push(`[${fmtClock(sim.now)}] ${s.notice}`);
  if(sim.notices.length>60)sim.notices.shift();
  $('notices').textContent=sim.notices.join('\n');
 }
 for(const el of document.querySelectorAll('.ctl')){
  const n=el.dataset.control;if(sim.raw[n])el.querySelector('.held').textContent=`held ${sim.now-heldSince[n]} ms`;
 }
 const banner=$('banner');banner.classList.toggle('show',s.stage!==4);
 banner.textContent=s.stage===2?'Stage 2 · screen and input check: controls only mirror switch levels; practice, saves and tuner are inert.':s.stage===3?'Stage 3 · audio check: level and YIN pitch telemetry; controls are inert.':'';

 $('m-rms').style.width=`${Math.min(100,s.rms/0.25*100)}%`;
 $('m-env').style.width=`${Math.min(100,s.env/0.25*100)}%`;
 $('m-gate').style.left=`${Math.min(100,s.gate/0.25*100)}%`;
 $('t-rms').textContent=`${s.rms.toFixed(4)} / ${s.env.toFixed(4)}`;
 const peak=$('m-peak');peak.style.width=`${Math.min(100,s.peak*100)}%`;peak.classList.toggle('clip',s.peak>=0.98);
 $('t-peak').textContent=s.peak.toFixed(3);

 const pitchWanted=s.stage===3||s.tuning||s.activity.startsWith('hunt')||s.activity.startsWith('bend')||s.activity.startsWith('run');
 $('t-pitch').textContent=pitchWanted?`yin hz=${s.hz.toFixed(2)} prob=${s.probability.toFixed(2)} · midi ${s.pitch_midi} ${s.pitch_cents>=0?'+':''}${s.pitch_cents.toFixed(1)} c ${s.pitch_valid?'valid':''} · ${s.analyses} windows`:`pitch gated (${s.activity} · ${s.analyses} windows)`;
 $('t-yin').textContent=`analyze ${M._vex_last_analysis_us()} µs`;
 if(sim.source&&sim.source.kind==='wav')$('wav-pos').textContent=`${(sim.source.pos/RATE).toFixed(1)} / ${(sim.source.total/RATE).toFixed(1)} s`;
 $('power').textContent=s.power?'power on':'POWER CUT · reboot to continue';

 updateDrillsPanel(s);
 const evSig=`${s.onsets}:${s.note_midi}`;
 if(evSig!==sim.lastEventSig){sim.lastEventSig=evSig;sim.stripDirty=true;}
 if(sim.stripDirty||force)drawActivityStrip(s);
 if(sessionCtrl)sessionCtrl.evaluate();

 const nowWall=performance.now();
 if($('inspect-drawer').open&&(force||nowWall-lastPanelAt>=90)){
  lastPanelAt=nowWall;
  if($('tab-state').classList.contains('sel')){
   $('state').innerHTML=window.VexScenario.stateNames(M).map(k=>`<tr><td>${k}</td><td>${typeof s[k]==='number'?(Number.isInteger(s[k])?s[k]:s[k].toFixed(4)):s[k]}</td></tr>`).join('');
  }
  if($('tab-eeprom').classList.contains('sel')){
   const sig=Array.from(storeBytes).join(',')+s.saved_slot+':'+s.skills_slot;
   if(sig!==lastStoreSig){
    lastStoreSig=sig;$('slot0').innerHTML=slotHtml(0,s);$('slot1').innerHTML=slotHtml(1,s);
    $('slot0').classList.toggle('live',s.saved_slot===0);$('slot1').classList.toggle('live',s.saved_slot===1);
    $('skslot0').innerHTML=skillsSlotHtml(0,s);$('skslot1').innerHTML=skillsSlotHtml(1,s);
    $('skslot0').classList.toggle('live',s.skills_slot===0);$('skslot1').classList.toggle('live',s.skills_slot===1);
   }
  }
  if(sim.logDirty){sim.logDirty=false;const pre=$('telemetry');pre.textContent=sim.telemetry.slice(-200).join('\n');pre.scrollTop=pre.scrollHeight;}
  if(sim.recDirty&&$('tab-scenario').classList.contains('sel')){sim.recDirty=false;$('rec-out').textContent=scenarioText();$('rec-count').textContent=`${sim.record.length} segments`;}
  if(dev.wctx&&$('tab-device').classList.contains('sel')){dev.wctx.putImageData(image,0,0);devDiff();}
 }
}

let storeSig=0;
function persistIfChanged(){
 let h=0;for(let i=0;i<storeBytes.length;i++)h=(h*31+storeBytes[i])>>>0;
 if(h!==storeSig){storeSig=h;persist();}
}

function renderScreenNow(){
 sim.lastDraw=sim.now;sim.dirty=false;
 M._vex_render(sim.now);ctx.putImageData(image,0,0);
}

// Main loop ---------------------------------------------------------------------------------------
let lastT=performance.now();
function frame(t){
 const real=Math.min(t-lastT,250);lastT=t;
 if(!sim.paused){
  sim.carry+=real*sim.speed;let n=Math.floor(sim.carry);sim.carry-=n;
  if(n>30000)n=30000;
  for(let i=0;i<n;i++)step();
  if(n)sim.dirty=true;
 }
 if(sim.dirty&&(!$('cadence').checked||sim.now-sim.lastDraw>=sim.displayMs||sim.now<sim.lastDraw)){
  renderScreenNow();
 }
 updatePanels(false);
 persistIfChanged();
 requestAnimationFrame(frame);
}

// Scenario replay (shared by Screens gallery and Inspect -> Scenario) ------------------------------
async function executeScenarioOnPage(text,opts){
 const o=Object.assign({fresh:true,upToLine:Infinity,label:''},opts);
 setPaused(true);
 const savedSource=sim.source,savedMode=sim.inputMode;
 if(savedSource&&savedSource.kind==='mic')stopMic();
 sim.source=null;sim.raw={mode:0,quest:0,foot:0};
 for(const el of document.querySelectorAll('.ctl'))el.classList.remove('down');
 clearStrip();
 const res=await window.VexScenario.runScenario(M,text,{
  fresh:o.fresh,upToLine:o.upToLine,checkExpect:true,label:o.label,
  loadWav:()=>loadWav(),
  onGolden:o.onGolden
 });
 sim.now=res.state.now_ms;sim.audioClock=sim.now;
 sim.stage=res.state.stage;$('stage').value=String(sim.stage);
 sim.testPattern=!!res.state.test;$('testpat').checked=sim.testPattern;
 sampleStrip();
 renderScreenNow();
 setInputMode(savedMode==='mic'?'quiet':savedMode);
 updatePanels(true);
 return res;
}

// Teensy hardware bridge (Inspect -> Device + header pill) ----------------------------------------
const dev={gen:-1,frameGen:-1,status:'off',state:null,telemetry:[],image:null,ctx:null,wctx:null};
function devCanvas(){if(dev.ctx)return;dev.ctx=$('dev-screen').getContext('2d');dev.wctx=$('dev-wasm').getContext('2d');dev.image=dev.ctx.createImageData(240,240);}
function devRgb565(bytes){
 const d=dev.image.data;
 for(let i=0;i<240*240;i++){
  const v=bytes[2*i]|bytes[2*i+1]<<8;
  d[4*i]=((v>>11)&31)*255/31|0;d[4*i+1]=((v>>5)&63)*255/63|0;d[4*i+2]=(v&31)*255/31|0;d[4*i+3]=255;
 }
}
function devDiff(){
 if(!dev.image||dev.frameGen<0)return;
 const n=window.VexScenario.diffRgba(dev.image.data,image.data);
 $('dev-diff').textContent=n?`${n} px differ from the real panel`:'identical to the real panel';
 return n;
}
async function devFrame(){
 const r=await fetch('/device/frame');const buf=new Uint8Array(await r.arrayBuffer());
 if(buf.length!==240*240*2)return;
 dev.frameGen=Number(r.headers.get('X-Frame-Generation'));devRgb565(buf);dev.ctx.putImageData(dev.image,0,0);
}
function spark(id,rows,key,fmt){
 const c=$(id),g=c.getContext('2d');g.clearRect(0,0,c.width,c.height);
 const v=rows.map(r=>Number(r[key])||0);if(!v.length){$(id+'-v').textContent='';return;}
 const max=Math.max(1,...v);g.strokeStyle='#c9a24a';g.lineWidth=1.5;g.beginPath();
 v.forEach((x,i)=>{const px=i/(Math.max(1,v.length-1))*(c.width-2)+1,py=c.height-1-(x/max)*(c.height-4);i?g.lineTo(px,py):g.moveTo(px,py);});
 g.stroke();g.fillStyle='#9a9199';g.font='10px ui-monospace,monospace';g.fillText(fmt(max),2,10);$(id+'-v').textContent=fmt(v[v.length-1]);
}
function updateBridgePill(j){
 const pill=$('bridge-pill'),txt=$('bridge-text');
 pill.className='bridge-pill '+(j.status==='connected'?'ok':j.status==='waiting'?'wait':j.status==='error'?'bad':'');
 txt.textContent=j.status==='connected'?`Teensy: ${j.port} live`:j.status==='waiting'?'Teensy: waiting':j.status==='error'?'Teensy: error':'Teensy: bridge off';
}
function devRender(j){
 updateBridgePill(j);
 const st=$('dev-status');st.className='sub mono '+(j.status==='connected'?'ok':j.status==='off'?'':'bad');
 st.textContent=j.status==='off'?'bridge off · run make sim DEVICE=auto':j.status==='connected'?`${j.port} · live${j.bench_log?` · log ${j.bench_log} (make bench-report)`:''}`:j.status==='waiting'?`waiting for a Teensy (${j.error||'none on USB'})`:`error · ${j.error}`;
 const s=j.state;dev.state=s;
 if(s){
  const flags=['running','calibrated','active','tuning','calibrating','claimed','codec','pitch_valid'].filter(k=>s[k]).join(' ')||'idle';
  $('dev-state').innerHTML=`stage ${s.stage} · <b>${s.anim_name}</b>/${s.frame} · level ${s.level} · fragments ${s.fragments} · session ${Math.floor(s.session_ms/1000)} s · total ${Math.floor(s.active_ms/1000)} s · goal ${s.goal_minutes} min · slot ${s.saved_slot}<br>`+
   `flush ${s.flush_us} µs (max ${s.flush_max_us}) · loop max ${s.loop_max_us} µs · tx ${s.tx_bytes} B · yin ${s.yin_us} µs (max ${s.yin_max_us}) · audio cpu ${s.cpu.toFixed(1)} % · cadence ${s.cadence_mul}× · ${flags}`;
  $('dev-frame-info').textContent=j.frame_complete?`device ${fmtClock(s.now_ms)}`:'waiting for a full frame (press Link)';
 }else{$('dev-state').textContent=j.status==='connected'?'telemetry only so far; press Link to stream the panel':'';$('dev-frame-info').textContent='';}
 const rows=j.telemetry||[],last=rows[rows.length-1];
 $('dev-knobs').textContent=last&&last.spi_hz!==undefined?`spi ${(last.spi_hz/1e6).toFixed(0)} MHz · line-in ${last.in_level} · test ${last.test?'on':'off'} · cadence ${last.cadence_ms} ms · diff ${last.diff?'on':'off'}${last.full?' · full':''}`:'';
 spark('sp-tx',rows,'tx_bytes',v=>`${v} B`);spark('sp-flush',rows,'flush_us',v=>`${v} µs`);spark('sp-yin',rows,'yin_us',v=>`${v} µs`);spark('sp-cpu',rows,'cpu',v=>`${Number(v).toFixed(1)} %`);
 if(j.log)$('dev-log').textContent=j.log.join('\n');
 if(M&&image){dev.wctx.putImageData(image,0,0);devDiff();}
}
function devPoll(){
 devCanvas();
 const loop=()=>fetch(`/device?gen=${dev.gen}`).then(r=>r.json()).then(async j=>{
  dev.status=j.status;dev.gen=j.generation===undefined?-1:j.generation;
  if(j.frame_generation!==undefined&&j.frame_generation!==dev.frameGen)await devFrame();
  devRender(j);
  setTimeout(loop,j.status==='off'?5000:j.status==='connected'?0:1000);
 }).catch(()=>{devRender({status:'off'});setTimeout(loop,5000);});
 loop();
}
function devSend(c){
 fetch(`/device/send?c=${encodeURIComponent(c)}`).then(r=>r.json()).then(j=>{note(`device ← '${c}' ${j.sent?'sent':'not sent (no device)'}`);}).catch(()=>{});
 const taps={m:['mode',100],q:['quest',100],k:['foot',100],M:['mode',1300],Q:['quest',1300],K:['foot',1300]};
 if(taps[c]&&$('dev-mirror-taps').checked){const [name,ms]=taps[c];pressSync(name,ms);}
 if('cspr'.includes(c)&&$('dev-mirror-taps').checked){M._vex_serial(c.charCodeAt(0),sim.now);sim.dirty=true;}
}
function devAdopt(){
 const s=dev.state;if(!s){note('adopt: no device state yet');return;}
 const b=storeBytes.subarray(0,64),dv=new DataView(b.buffer,b.byteOffset,64);b.fill(0);
 dv.setUint32(0,0x56455831,true);dv.setUint32(4,1,true);dv.setUint32(8,s.sequence+1,true);dv.setUint32(12,s.fragments,true);
 dv.setUint32(16,{5:0,10:1,20:2}[s.goal_minutes]||0,true);dv.setUint32(20,s.claimed?1:0,true);
 dv.setBigUint64(24,BigInt(s.active_ms),true);dv.setBigUint64(32,BigInt(s.session_ms),true);dv.setUint32(40,crc32(b,40),true);
 storeBytes.fill(0,64);M._vex_store_load();persist();
 recordComment(`adopted Teensy record: fragments ${s.fragments}, total ${Math.floor(s.active_ms/1000)} s`);
 boot(false);lastStoreSig='';
}

function selectInspectTab(tabName){
 document.querySelectorAll('.tabs button').forEach(x=>x.classList.toggle('sel',x.dataset.tab===tabName));
 document.querySelectorAll('.tab').forEach(x=>x.classList.toggle('sel',x.id==='tab-'+tabName));
 sim.recDirty=true;lastStoreSig='';updatePanels(true);
}

// Guitar drills panel ("Play it for me" performances via shared protocol.h) -----------------------
const DRILL_FAMILIES=[
 {prefix:'pulse',label:'Timing',walk:'pulse',
  plays:{steady:'Clean 4-bar take (+4 BPM)',rushing:'Rushing (−9 ms/note)',uneven:'Uneven (±46 ms jitter)',wrong:'Off-grid notes'},
  hint:'Anchors on your first pick. Keep within ±30 ms for 4 bars to climb +4 BPM; amber LED flashes the beat (except PULSE FREE).'},
 {prefix:'hunt',label:'Note finder',walk:'hunt',
  plays:{steady:'Find all 10 notes',rushing:'Fast 4-note streak',uneven:'Slow / hesitant finds',wrong:'Wrong string / note'},
  hint:'HUNT STR checks exact string + pitch register (e.g. F#2 on E6 vs F#3 on D); HUNT ANY accepts any octave. Slow notes repeat sooner.'},
 {prefix:'bend',label:'Bend & vib',walk:'bend',
  plays:{steady:'8 clean target bends',rushing:'Steady E4 vibrato',uneven:'Over-bent (+28 c sharp)',wrong:'Under-bent (−32 c flat)'},
  hint:'Fret a base note (≥ D3) then bend up +100 c (1/2) or +200 c (full) and hold the top. Holding steady wobble reports vibrato Hz & cents.'},
 {prefix:'run',label:'Scale runs',walk:'run',
  plays:{steady:'Clean 16-note scale run',rushing:'Brisk 128 BPM run',uneven:'Uneven spacing run',wrong:'Out-of-key notes (red)'},
  hint:'First note of each 16-note phrase sets your key root. In-scale notes light gold; outside notes flash red and protect your clean BPM best.'},
 {prefix:'touch',label:'Dynamics',walk:'touch',
  plays:{steady:'Clean take (hits accents)',rushing:'Fast even picking',uneven:'Uneven volume (±6 dB)',wrong:'Accent on wrong beat'},
  hint:'Pitch is gated off. Bars plot each pick’s dB level vs your running median; ACC3 / ACC4 require ≥ +4 dB on beat 1 of each group.'}
];
function familyForActivity(act){
 return DRILL_FAMILIES.find(f=>act&&act.startsWith(f.prefix))||DRILL_FAMILIES[0];
}
function selectDrillBySlug(slug){
 setPaused(false);setInputMode('quiet');
 if(sim.stage!==4){sim.stage=4;$('stage').value='4';boot(false);}
 let st=readState();
 if(!st.calibrated)execSim('calibrate');
 execSim(`drill ${slug}`);
 recordCommand(`drill ${slug}`);
 renderScreenNow();updatePanels(true);
}
function execSim(line){
 window.VexScenario.execLine(M,line);
 if(M._vex_store_dirty())persist();
 sim.now=M._vex_now();sim.audioClock=sim.now;
 sampleStrip();
}
function initDrillsPanel(){
 const wrap=$('drill-presets');if(!wrap)return;
 wrap.innerHTML='';
 const buckets=new Map(DRILL_FAMILIES.map(f=>[f.prefix,[]]));
 const n=M._vex_preset_count();
 for(let i=0;i<n;i++){
  const slug=M.UTF8ToString(M._vex_preset_slug(i));
  const title=M.UTF8ToString(M._vex_preset_title(i));
  const fam=familyForActivity(slug);
  buckets.get(fam.prefix).push({slug,title});
 }
 for(const fam of DRILL_FAMILIES){
  const row=document.createElement('div');
  row.className='drill-fam';row.dataset.fam=fam.prefix;
  const lbl=document.createElement('span');
  lbl.className='drill-fam-lbl';lbl.textContent=fam.label;
  lbl.style.cursor='pointer';lbl.title=`Open ${fam.label} step-by-step walkthrough`;
  lbl.onclick=()=>{if(sessionCtrl)sessionCtrl.setTrack(fam.walk);};
  const pills=document.createElement('div');pills.className='drill-fam-pills';
  for(const item of buckets.get(fam.prefix)){
   const b=document.createElement('button');
   b.type='button';b.className='pill';b.dataset.drill=item.slug;b.textContent=item.title;
   b.onclick=()=>selectDrillBySlug(item.slug);
   pills.appendChild(b);
  }
  row.appendChild(lbl);row.appendChild(pills);wrap.appendChild(row);
 }
 for(const b of document.querySelectorAll('[data-drill-play]')){
  b.onclick=()=>playDrillPerformance(b.dataset.drillPlay);
 }
}
function updateDrillsPanel(s){
 for(const b of document.querySelectorAll('[data-drill]')){
  b.classList.toggle('sel',s.activity===b.dataset.drill);
 }
 const fam=familyForActivity(s.activity);
 const onDrill=s.activity!=='pet'&&s.activity!=='tuner';
 for(const row of document.querySelectorAll('.drill-fam')){
  row.classList.toggle('active',onDrill&&row.dataset.fam===fam.prefix);
 }
 for(const b of document.querySelectorAll('[data-drill-play]')){
  const k=b.dataset.drillPlay;
  if(fam.plays[k])b.textContent=fam.plays[k];
 }
 const hintEl=$('drill-hint');
 if(hintEl){
  hintEl.textContent=onDrill?fam.hint:'On the pedal: tap MODE twice (Pet → Tuner → Drill), tap QUEST while paused to cycle presets, and tap FOOTSWITCH to start/pause.';
 }
 const el=$('drill-status');if(!el)return;
 const fmt=window.VexSession&&window.VexSession.fmtBest?window.VexSession.fmtBest:(_,v)=>String(v);
 if(!onDrill){
  el.textContent=`currently on ${s.activity.toUpperCase()} · click any preset below or tap MODE twice`;
 }else{
  el.textContent=`${s.activity} · ${s.running?'running':'paused'}${s.bpm?` · ${s.bpm} BPM`:''} · best ${fmt(s.activity,s.drill_best)} · rounds ${s.drill_rounds}`;
 }
}
function playDrillPerformance(kind){
 setPaused(false);setInputMode('quiet');
 if(sim.stage!==4){sim.stage=4;$('stage').value='4';boot(false);}
 let s=readState();
 if(!s.calibrated){execSim('calibrate');s=readState();}
 if(s.activity==='pet'||s.activity==='tuner'){execSim('drill pulse-8th');s=readState();}
 if(!s.running){execSim('press foot 100');s=readState();}
 const act=s.activity;
 const PC=['C','C#','D','D#','E','F','F#','G','G#','A','Bb','B'];
 if(act.startsWith('pulse')){
  const is8=act==='pulse-8th',bpm=s.bpm||96,rate=is8?`${bpm}x2`:`${bpm}`;
  const notes16='D3 D3 D3 D3 G3 G3 G3 G3 B3 B3 B3 B3 E4 E4 E4 E4';
  const notes=is8?`${notes16} ${notes16}`:notes16;
  if(kind==='steady')execSim(`notes ${rate} 2 0 -18 ${notes}`);
  else if(kind==='rushing')execSim(`notes ${rate} 6 -9 -18 ${notes}`);
  else if(kind==='uneven')execSim(`notes ${rate} 46 0 -18 ${notes}`);
  else execSim(`notes ${rate} 55 11 -18 ${notes}`);
 }else if(act.startsWith('hunt')){
  const midiName=m=>`${PC[m%12]}${Math.floor(m/12)-1}`;
  if(kind==='wrong'){
   const wrongMidi=s.hunt_midi>0?(s.hunt_midi===40?41:40):((s.hunt_pc+1)%12+48);
   execSim(`notes 140 0 0 -18 ${midiName(wrongMidi)}`);
  }else{
   const count=kind==='steady'?10:4;
   for(let i=0;i<count;i++){
    s=readState();
    if(kind==='uneven'&&i%2===0)execSim('wait 1400');
    const tok=s.hunt_midi>0?midiName(s.hunt_midi):`${PC[s.hunt_pc]}3`;
    execSim(`notes ${kind==='rushing'?200:140} 0 0 -18 ${tok}`);
   }
  }
 }else if(act.startsWith('bend')){
  const tgt=act==='bend-whole'?200:100;
  if(kind==='steady'){for(let i=0;i<8;i++)execSim(`bend 900 A3 ${tgt} 340 -16`);}
  else if(kind==='rushing')execSim(`vib 1200 E4 5.6 42 -16`);
  else if(kind==='uneven')execSim(`bend 900 D4 ${tgt+28} 340 -16`);
  else execSim(`bend 900 A3 ${tgt-32} 340 -16`);
 }else if(act.startsWith('run')){
  const clean=act==='run-major'?'C3 D3 E3 F3 G3 A3 B3 C4 B3 A3 G3 F3 E3 D3 C3 G3':
              act==='run-minor'?'E3 F#3 G3 A3 B3 C4 D4 E4 D4 C4 B3 A3 G3 F#3 E3 B3':
              'A2 C3 D3 E3 G3 A3 G3 E3 D3 C3 A2 C3 D3 E3 G3 A3';
  if(kind==='steady')execSim(`notes 112x2 2 0 -18 ${clean}`);
  else if(kind==='rushing')execSim(`notes 128x2 4 -4 -18 ${clean}`);
  else if(kind==='uneven')execSim(`notes 108x2 52 0 -18 ${clean}`);
  else execSim('notes 112x2 3 0 -18 A2 Bb2 B2 C3 C#3 D3 D#3 E3 F3 F#3 G3 G#3 A3 Bb3 B3 C4');
 }else if(act.startsWith('touch')){
  const acc3=act==='touch-acc3',acc4=act==='touch-acc4';
  if(kind==='steady'){
   if(acc3)execSim('notes 110x2 2 0 -20 D3@+6 D3 D3 G3@+6 G3 G3 B3@+6 B3 B3 D3@+6 D3 D3');
   else if(acc4)execSim('notes 110x2 2 0 -20 D3@+6 D3 D3 D3 G3@+6 G3 G3 G3 B3@+6 B3 B3 B3');
   else execSim('notes 110x2 2 0 -18 D3 D3 D3 D3 G3 G3 G3 G3 B3 B3 B3 B3');
  }else if(kind==='rushing')execSim('notes 132x2 4 0 -18 D3 D3 D3 D3 G3 G3 G3 G3 B3 B3 B3 B3');
  else if(kind==='uneven')execSim('notes 110x2 5 0 -20 D3@-6 D3@+5 D3@-4 D3@+7 G3@-5 G3@+4 G3@-7 G3@+6 B3@-5 B3@+6 B3@-4 B3@+5');
  else execSim('notes 110x2 2 0 -18 D3 D3@+6 D3 G3 G3@+6 G3 B3 B3@+6 B3 D3 D3@+6 D3');
 }
 renderScreenNow();updatePanels(true);
}

// Wiring ------------------------------------------------------------------------------------------
function wire(){
 $('pause').onclick=togglePause;
 $('speed').onchange=e=>{sim.speed=Number(e.target.value);localStorage.setItem('vex.speed',e.target.value);};
 $('step1').onclick=()=>{setPaused(false);run(1000);renderScreenNow();updatePanels(true);};
 $('adv5').onclick=()=>{run(300000);note('advanced 5 minutes');renderScreenNow();updatePanels(true);};
 $('stage').onchange=e=>{sim.stage=Number(e.target.value);recordCommand(`stage ${sim.stage}`);boot(false);renderScreenNow();updatePanels(true);};
 $('reboot').onclick=()=>{recordCommand('reboot');boot(true);renderScreenNow();updatePanels(true);};
 $('reset').onclick=()=>{if(confirm('Zero both EEPROM slots in this browser and reboot?')){recordCommand('reset');factoryReset();renderScreenNow();updatePanels(true);}};
 $('truesize').onchange=e=>$('bezel').classList.toggle('true',e.target.checked);
 $('cadence').onchange=e=>{localStorage.setItem('vex.cadence',e.target.checked?'1':'0');sim.dirty=true;};
 $('testpat').onchange=e=>setTestPattern(e.target.checked);

 for(const b of document.querySelectorAll('[data-input]'))b.onclick=()=>setInputMode(b.dataset.input);
 for(const id of ['inj-rms','inj-peak','inj-hz','inj-prob'])$(id).oninput=()=>{if(sim.inputMode==='custom')applyCustomInject();};
 $('syn-cents').oninput=e=>$('syn-cents-t').textContent=`${Number(e.target.value)>=0?'+':''}${e.target.value} c`;
 $('syn-db').oninput=e=>$('syn-db-t').textContent=`${e.target.value} dBFS`;
 $('wav-play').onclick=async()=>{try{$('wav-play').disabled=true;const pcm=await loadWav();setSource(wavSource(pcm));note('playing test recording');}catch(e){note('WAV failed: '+e.message);}finally{$('wav-play').disabled=false;}};
 $('mic-start').onclick=async()=>{if(mic){stopSource('stopped');return;}try{setSource(await micSource());$('mic-start').textContent='Stop Mac input';note('Mac input started');}catch(e){$('mic-info').textContent='Microphone unavailable: '+e.message;}};
 $('clip-rec').onclick=toggleClipRecording;

 const drawer=$('inspect-drawer');
 if(localStorage.getItem('vex.inspect')==='1')drawer.open=true;
 drawer.addEventListener('toggle',()=>{try{localStorage.setItem('vex.inspect',drawer.open?'1':'0');}catch(_){}if(drawer.open)updatePanels(true);});
 $('bridge-pill').onclick=()=>{drawer.open=true;selectInspectTab('device');drawer.scrollIntoView({behavior:'smooth',block:'nearest'});};
 for(const b of document.querySelectorAll('.tabs button'))b.onclick=()=>selectInspectTab(b.dataset.tab);

 $('save').onclick=()=>{recordCommand('save');M._vex_serial(115,sim.now);sim.dirty=true;renderScreenNow();updatePanels(true);};
 $('corrupt').onclick=()=>{const s=readState();if(s.saved_slot<0)return;recordCommand('corrupt');storeBytes[64*s.saved_slot+40]^=1;M._vex_store_load();lastStoreSig='';updatePanels(true);};
 $('cut-arm').onclick=()=>{const n=Number($('cut-n').value);recordCommand(`powercut ${n}`);M._vex_powercut(n);note(`power cut armed after ${n} byte writes`);};
 $('rec-copy').onclick=async()=>{const t=scenarioText();try{await navigator.clipboard.writeText(t);$('rec-count').textContent='copied';}catch(_){$('rec-out').textContent=t;}};
 $('rec-clear').onclick=()=>{sim.record=[];sim.span=null;closeSpan();sim.recDirty=true;updatePanels(true);};
 $('scn-run').onclick=async()=>{
  $('scn-out').textContent='running…';
  const res=await executeScenarioOnPage($('scn-in').value,{
   fresh:$('scn-fresh').checked,label:'editor',
   onGolden:async(name,line,rgba)=>window.VexScenario.diffRgba(rgba,await galleryCtrl.loadGoldenRgba(name))
  });
  const lines=[...res.lines,res.failures?`${res.failures} failure(s)`:'all expectations met',JSON.stringify(res.state)];
  $('scn-out').textContent=lines.join('\n');
 };
 for(const b of document.querySelectorAll('[data-send]'))b.onclick=()=>devSend(b.dataset.send);
 $('dev-adopt').onclick=devAdopt;
 $('session-reset').onclick=()=>{factoryReset();setInputMode('quiet');setPaused(false);renderScreenNow();updatePanels(true);};
}

function liveReload(){
 let gen=-1;const el=$('reload');
 const poll=()=>fetch(`/reload?gen=${gen}`).then(r=>r.json()).then(j=>{
  el.className='';
  if(j.error){el.className='err';el.textContent='build error: see terminal';}
  else if(gen>=0&&j.generation!==gen){el.textContent='rebuilt, reloading…';location.reload();return;}
  else el.textContent='live reload armed';
  gen=j.generation;poll();
 }).catch(()=>{el.textContent='live reload offline';});
 poll();
}

VexModule().then(mod=>{
 M=mod;audioPtr=M._vex_audio_ptr();
 const sp=M._vex_store_ptr(),ss=M._vex_store_size();storeBytes=M.HEAPU8.subarray(sp,sp+ss);
 const fp=M._vex_frame_ptr();image=new ImageData(new Uint8ClampedArray(M.HEAPU8.buffer,fp,240*240*4),240*1,240);
 stripIdx={
  gate:window.VexScenario.fieldIndex(M,'gate'),
  rms:window.VexScenario.fieldIndex(M,'rms'),
  peak:window.VexScenario.fieldIndex(M,'peak'),
  active:window.VexScenario.fieldIndex(M,'active'),
  env:window.VexScenario.fieldIndex(M,'env'),
  hold_ms:window.VexScenario.fieldIndex(M,'hold_ms')
 };
 ctx=$('screen').getContext('2d');
 stripCtx=$('activity-strip').getContext('2d');
 sim.displayMs=M._vex_display_ms();$('cadence-ms').textContent=String(sim.displayMs);
 sim.speed=Number(localStorage.getItem('vex.speed')||'1');$('speed').value=String(sim.speed);
 if(localStorage.getItem('vex.cadence')==='0')$('cadence').checked=false;

 restoreStore();storeSig=-1;
 boot(false);
 setInputMode('quiet');

 sessionCtrl=window.VexSession.createSession($('session-steps'),$('session-status'),{
  getState:()=>readState(),
  getInputMode:()=>sim.inputMode,
  setInputMode:(m,o)=>{setPaused(false);setInputMode(m,o);},
  ensureStage4:()=>{if(sim.stage!==4){sim.stage=4;$('stage').value='4';boot(false);}},
  exitTestPattern:()=>{if(sim.testPattern)setTestPattern(false);},
  press:(name,ms)=>{setPaused(false);pressSync(name,ms);renderScreenNow();updatePanels(true);},
  advance:ms=>{setPaused(false);run(ms);renderScreenNow();updatePanels(true);},
  selectDrill:slug=>selectDrillBySlug(slug),
  playDrill:kind=>playDrillPerformance(kind),
  reboot:()=>{recordCommand('reboot');boot(true);renderScreenNow();updatePanels(true);}
 });
 sessionCtrl.resetProgress();

 galleryCtrl=window.VexGallery.createGallery({
  gridEl:$('gallery-grid'),
  filtersEl:$('gallery-filters'),
  statusEl:$('gallery-status'),
  scenarioSelectEl:$('scn-pick'),
  api:{
   replayToLine:(text,line,label)=>executeScenarioOnPage(text,{fresh:true,upToLine:line,label}),
   currentFrameRgba:()=>new Uint8Array(image.data),
   setScenarioEditor:text=>{$('scn-in').value=text;}
  }
 });
 galleryCtrl.init();

 initDrillsPanel();
 wire();
 renderScreenNow();
 updatePanels(true);
 closeSpan();
 $('loading').hidden=true;$('main').hidden=false;
 if(!/noreload/.test(location.search))liveReload();
 devPoll();
 requestAnimationFrame(frame);
}).catch(e=>{$('loading').textContent='vex.wasm failed to load: '+e.message;});
