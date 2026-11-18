// VEX workbench page. The firmware runs in vex.wasm (simulator/web/vex_web.cpp); this file owns the clock,
// the controls, audio sources, inspection panels and the scenario recorder. One millisecond per step, in
// the sketch's loop order, so a recorded scenario replays identically in the native simulator.
'use strict';
const $=id=>document.getElementById(id);
const STATE_FIELDS=['active_ms','session_ms','xp','level','fragments','goal_minutes','running','calibrated','active','tuning','calibrating','gate','rms','peak','stage','saved_slot','sequence','power','pitch_midi','pitch_cents','pitch_valid','uncertain','hz','probability','now_ms'];
const BOOLS=new Set(['running','calibrated','active','tuning','calibrating','power','pitch_valid','uncertain']);
const POSES=['idle','blink','strum','cheer','levelup','listen','sleep'];   // order of art/vex/manifest.json animations
const BLOCK=128,RATE=44100,BLOCK_MS=1000*BLOCK/RATE;
const sim={now:0,stage:4,speed:1,paused:false,raw:{mode:0,quest:0,foot:0},inject:{rms:.1,peak:.3,hz:0,prob:.99,on:true},
 source:null,audioClock:0,carry:0,lastDraw:-1000,dirty:true,lastNotice:'',notices:[],telemetry:[],record:[],recordStart:0,span:null,
 lastState:null,analyses:0};
let M,audioPtr,storeBytes,image,ctx,stateView;

function fmtClock(ms){const s=Math.floor(ms/1000);return `${String(Math.floor(s/60)).padStart(2,'0')}:${String(s%60).padStart(2,'0')}.${String(ms%1000).padStart(3,'0')}`;}
function readState(){const p=M._vex_state()>>3;const v=M.HEAPF64.subarray(p,p+STATE_FIELDS.length);const o={};STATE_FIELDS.forEach((k,i)=>o[k]=BOOLS.has(k)?v[i]!==0:v[i]);o.notice=M.UTF8ToString(M._vex_notice());o.analyses=sim.analyses;return o;}

// Device lifecycle -------------------------------------------------------------------------------
function boot(){sim.now=0;sim.audioClock=0;M._vex_boot(sim.stage,0);sim.dirty=true;sim.lastNotice='';note(`boot stage ${sim.stage}`);}
function factoryReset(){storeBytes.fill(0);M._vex_store_load();persist();boot();}
function note(line){sim.telemetry.push(`[${fmtClock(sim.now)}] ${line}`);if(sim.telemetry.length>400)sim.telemetry.splice(0,sim.telemetry.length-400);sim.logDirty=true;}
function persist(){try{localStorage.setItem('vex.store',btoa(String.fromCharCode(...storeBytes)));}catch(e){}}
function restoreStore(){try{const s=localStorage.getItem('vex.store');if(!s)return;const b=atob(s);if(b.length!==128)return;for(let i=0;i<128;i++)storeBytes[i]=b.charCodeAt(i);M._vex_store_load();}catch(e){}}

// One simulated millisecond -----------------------------------------------------------------------
const blockView=new Int16Array(BLOCK);
function step(){
 const now=++sim.now;
 if(sim.source){
  if(sim.source.kind==='mic'){let b;while((b=sim.source.next()))pushBlock(b,now);}
  else while(sim.audioClock<=now){const b=sim.source.next();if(!b){stopSource('finished');break;}pushBlock(b,now);sim.audioClock+=BLOCK_MS;}
 }else if(sim.inject.on){M._vex_observe(sim.inject.rms,sim.inject.peak,now);if(sim.inject.hz>0)M._vex_pitch(sim.inject.hz,sim.inject.prob,now);}
 M._vex_controls(sim.raw.mode,sim.raw.quest,sim.raw.foot,now);
 M._vex_tick(now);
 if(now%1000===0)telemetryLine();
}
function pushBlock(block,now){M.HEAP16.set(block,audioPtr>>1);if(M._vex_audio_push(BLOCK,now))sim.analyses++;}
function run(ms){for(let i=0;i<ms;i++)step();sim.dirty=true;}
function telemetryLine(){
 const s=readState();
 note(`stage=${s.stage} rms=${s.rms.toFixed(5)} peak=${s.peak.toFixed(3)} gate=${s.gate.toFixed(5)} active=${+s.active} running=${+s.running} session_s=${Math.floor(s.session_ms/1000)} total_s=${Math.floor(s.active_ms/1000)} fragments=${s.fragments} pitch=yin hz=${s.hz.toFixed(2)} prob=${s.probability.toFixed(2)} yin_us=${M._vex_last_analysis_us()}`);
}

// Audio sources: each returns Int16Array(128) blocks or null when finished ----------------------------
function synthSource(){
 const buf=new Int16Array(BLOCK);let phase=0,t=0;
 return {kind:'synth',label:'synth note',next(){
  const hz=Number($('syn-note').value)*Math.pow(2,Number($('syn-cents').value)/1200);let amp=Math.pow(10,Number($('syn-db').value)/20);
  const decay=$('syn-decay').checked;
  for(let i=0;i<BLOCK;i++){const env=decay?Math.exp(-((t%2)*2.2)):1;t+=1/RATE;phase+=2*Math.PI*hz/RATE;if(phase>2*Math.PI)phase-=2*Math.PI;
   const v=amp*env*(Math.sin(phase)+.45*Math.sin(2*phase)+.15*Math.sin(3*phase))/1.6+(Math.random()-.5)*3e-4;buf[i]=Math.max(-32768,Math.min(32767,Math.round(v*32767)));}
  return buf;}};
}
let wavPcm=null,wavPromise=null;
function loadWav(){
 if(!wavPromise)wavPromise=fetch('guitar_notes_left_only.wav').then(r=>r.arrayBuffer()).then(async ab=>{
  const ac=new OfflineAudioContext(1,RATE,RATE);const decoded=await ac.decodeAudioData(ab);
  if(Math.round(decoded.sampleRate)!==RATE)throw Error(`test recording decoded at ${decoded.sampleRate} Hz`);
  const f=decoded.getChannelData(0);const pcm=new Int16Array(f.length);for(let i=0;i<f.length;i++)pcm[i]=Math.max(-32768,Math.min(32767,Math.round(f[i]*32767)));wavPcm=pcm;return pcm;});
 return wavPromise;
}
function wavSource(pcm){
 const buf=new Int16Array(BLOCK);let pos=0;
 return {kind:'wav',label:'test recording',get pos(){return pos;},total:pcm.length,next(){if(pos>=pcm.length)return null;buf.fill(0);const n=Math.min(BLOCK,pcm.length-pos);buf.set(pcm.subarray(pos,pos+n));pos+=n;return buf;}};
}
let mic=null;
async function micSource(){
 const stream=await navigator.mediaDevices.getUserMedia({audio:{echoCancellation:false,noiseSuppression:false,autoGainControl:false,channelCount:1}});
 const ctx=new AudioContext({sampleRate:RATE,latencyHint:'interactive'});await ctx.audioWorklet.addModule('worklet.js');
 const node=new AudioWorkletNode(ctx,'vex-capture',{numberOfInputs:1,numberOfOutputs:0});
 const queue=[];node.port.onmessage=e=>{if(queue.length<256)queue.push(e.data);};
 ctx.createMediaStreamSource(stream).connect(node);
 const label=stream.getAudioTracks()[0].label||'input';
 mic={stream,ctx,node};$('mic-info').textContent=`${label} · ${ctx.sampleRate} Hz · base latency ${(ctx.baseLatency*1000).toFixed(1)} ms`;
 if(ctx.sampleRate!==RATE)$('mic-info').textContent+=' · WARNING: not 44.1 kHz, pitch will be scaled';
 return {kind:'mic',label:`Mac input (${label})`,next(){return queue.shift()||null;}};
}
function stopMic(){if(!mic)return;mic.stream.getTracks().forEach(t=>t.stop());mic.ctx.close();mic=null;$('mic-info').textContent='';$('mic-start').textContent='Start Mac input';}
function setSource(src){if(sim.source&&sim.source.kind==='mic'&&(!src||src.kind!=='mic'))stopMic();if(sim.source)closeSpan();sim.source=src;sim.audioClock=sim.now;
 $('audio-status').textContent=src?src.label:(sim.inject.on?'synthetic observations':'no observations (stale)');recordComment(src?`live audio: ${src.label} (not reproducible; replace with audio lines)`:null);}
function stopSource(reason){if(!sim.source)return;const k=sim.source.kind;setSource(null);note(`audio source ${k} ${reason||'stopped'}`);if(k==='wav')$('wav-pos').textContent='';}

// Controls ---------------------------------------------------------------------------------------
const heldSince={};
function setControl(name,down){if(sim.raw[name]===(down?1:0))return;sim.raw[name]=down?1:0;const el=document.querySelector(`[data-control=${name}]`);el.classList.toggle('down',down);if(down)heldSince[name]=sim.now;closeSpan();}
for(const el of document.querySelectorAll('[data-control]')){const name=el.dataset.control;
 el.addEventListener('pointerdown',e=>{e.preventDefault();el.setPointerCapture(e.pointerId);setControl(name,true);});
 for(const ev of ['pointerup','pointercancel','lostpointercapture'])el.addEventListener(ev,()=>setControl(name,false));
 el.addEventListener('keydown',e=>{if(e.key==='Enter'){e.preventDefault();setControl(name,true);}});el.addEventListener('keyup',e=>{if(e.key==='Enter')setControl(name,false);});}
const KEYS={m:'mode',q:'quest',' ':'foot'};
function typing(e){return /^(INPUT|TEXTAREA|SELECT)$/.test(e.target.tagName)||e.target.isContentEditable;}
document.addEventListener('keydown',e=>{if(typing(e)||e.metaKey||e.ctrlKey||e.altKey)return;const k=e.key.toLowerCase();if(KEYS[k]){e.preventDefault();if(!e.repeat)setControl(KEYS[k],true);}else if(k==='p'){togglePause();}});
document.addEventListener('keyup',e=>{const k=e.key.toLowerCase();if(KEYS[k]&&!typing(e))setControl(KEYS[k],false);});
window.addEventListener('blur',()=>{for(const n in sim.raw)setControl(n,false);});

// Scenario recorder: spans of (pressed control, injected observation) become press/audio/wait lines ----
function spanKey(){const c=sim.raw.mode?'mode':sim.raw.quest?'quest':sim.raw.foot?'foot':'';const inj=!sim.source&&sim.inject.on?`${sim.inject.rms} ${sim.inject.peak}${sim.inject.hz>0?` ${sim.inject.hz} ${sim.inject.prob}`:''}`:'';return c+'|'+inj;}
function closeSpan(){const key=spanKey();if(sim.span&&sim.span.key===key)return;if(sim.span){const ms=sim.now-sim.span.at;if(ms>0)sim.record.push({at:sim.span.at,ms,key:sim.span.key});}sim.span={at:sim.now,key};sim.recDirty=true;}
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

// Scenario runner (same protocol as simulator/main.cpp, plus expect) --------------------------------
function parseValue(v){if(v==='true'||v==='false')return v==='true';const n=Number(v);return v!==''&&!isNaN(n)?n:v.replace(/_/g,' ');}
function pressSync(name,ms){sim.raw[name]=1;run(ms);sim.raw[name]=0;run(30);}
function audioSync(ms,r,p,hz,prob){const saved={...sim.inject},src=sim.source;sim.source=null;sim.inject={rms:r,peak:p,hz:hz||0,prob:prob===undefined?1:prob,on:true};run(ms);sim.inject=saved;sim.source=src;}
async function runScenario(text,fresh){
 const out=[];let failures=0;const wasPaused=sim.paused;sim.paused=true;const wasRecording=sim.record.length;
 // Isolate the replay from the page's own inputs so it matches the native simulator step for step.
 const savedInject=sim.inject,savedSource=sim.source,savedRaw={...sim.raw};sim.inject={...sim.inject,on:false};sim.source=null;sim.raw={mode:0,quest:0,foot:0};
 if(fresh)factoryReset();
 let last=readState();
 for(const [i,rawLine] of text.split('\n').entries()){
  const line=rawLine.trim();if(!line||line.startsWith('#'))continue;const a=line.split(/\s+/),cmd=a[0];const where=`line ${i+1}`;
  try{
   if(cmd==='expect'){for(const pair of a.slice(1)){let [k,v]=pair.split('=');const approx=k.endsWith('~');k=k.replace(/~$/,'');const want=parseValue(v),got=last[k];
     let ok;if(approx)ok=typeof got==='number'&&Math.abs(got-want)<=200;else if(typeof want==='number')ok=typeof got==='number'&&Math.abs(got-want)<=1e-6;else ok=got===want;
     if(!ok){failures++;out.push(`FAIL ${where}: ${k} expected ${JSON.stringify(want)}, got ${JSON.stringify(got)}`);}}continue;}
   if(cmd==='golden'){out.push(`note ${where}: golden ${a[1]} is checked by make test, not here`);continue;}
   if(cmd==='press')pressSync(a[1],Number(a[2]||100));
   else if(cmd==='audio')audioSync(Number(a[1]),Number(a[2]),Number(a[3]),a[4]!==undefined?Number(a[4]):0,a[5]!==undefined?Number(a[5]):1);
   else if(cmd==='wait')run(Number(a[1]));
   else if(cmd==='serial')M._vex_serial(a[1].charCodeAt(0),sim.now);
   else if(cmd==='save')M._vex_serial(115,sim.now);
   else if(cmd==='calibrate'){const r=a[1]!==undefined?Number(a[1]):.001;M._vex_serial(99,sim.now);audioSync(3100,r,r*2,0,0);}
   else if(cmd==='stage'){sim.stage=Number(a[1]);$('stage').value=String(sim.stage);boot();}
   else if(cmd==='powercut')M._vex_powercut(Number(a[1]));
   else if(cmd==='reboot')boot();
   else if(cmd==='corrupt'){const s=readState();if(s.saved_slot>=0){storeBytes[64*s.saved_slot+40]^=1;M._vex_store_load();}}
   else if(cmd==='reset')factoryReset();
   else if(cmd==='wav'||cmd==='raw'){if(!/guitar_notes_left_only\.wav$/.test(a[1]))throw Error('only the bundled test recording is available in the browser');
    const pcm=await loadWav();const start=Math.min(pcm.length,Math.floor(Number(a[2]||0)*44.1)),len=a[3]!==undefined?Number(a[3]):-1;const end=len<0?pcm.length:Math.min(pcm.length,start+Math.floor(len*44.1));
    const src=wavSource(pcm.subarray(start,end));const saved=sim.source;sim.source=src;sim.audioClock=sim.now;while(sim.source===src)step();sim.source=saved;}
   else if(cmd!=='state')throw Error(`unknown command ${cmd}`);
   last=readState();
  }catch(e){failures++;out.push(`FAIL ${where}: ${e.message}`);}
 }
 sim.inject=savedInject;sim.source=savedSource;sim.raw=savedRaw;sim.audioClock=sim.now;
 sim.record.length=wasRecording;sim.span=null;closeSpan();recordComment(`scenario replayed here (${failures?failures+' failures':'ok'})`);
 out.push(failures?`${failures} failure(s)`:'all expectations met');out.push(JSON.stringify(last));
 sim.paused=wasPaused;sim.dirty=true;return out.join('\n');
}

// Panels ------------------------------------------------------------------------------------------
function crc32(bytes,n){let c=0xffffffff;for(let i=0;i<n;i++){c^=bytes[i];for(let j=0;j<8;j++)c=(c>>>1)^(0xedb88320&-(c&1));}return (~c)>>>0;}
function slotHtml(i,s){
 const b=storeBytes.subarray(64*i,64*i+48),dv=new DataView(b.buffer,b.byteOffset,48);
 const magic=dv.getUint32(0,true),version=dv.getUint32(4,true),seq=dv.getUint32(8,true),frag=dv.getUint32(12,true),goal=dv.getUint32(16,true),claimed=dv.getUint32(20,true);
 const active=Number(dv.getBigUint64(24,true)),session=Number(dv.getBigUint64(32,true)),sum=dv.getUint32(40,true);
 const valid=magic===0x56455831&&version===1&&goal<3&&claimed<2&&sum===crc32(b,40);
 let hex='';for(let r=0;r<4;r++){hex+=Array.from(storeBytes.subarray(64*i+r*16,64*i+r*16+16),x=>x.toString(16).padStart(2,'0')).join(' ')+'\n';}
 return `<h3>slot ${i} ${s.saved_slot===i?'· loaded':''} · ${valid?'valid':'invalid'}</h3><div class="mono">seq ${seq} · frag ${frag} · goal ${goal} · claimed ${claimed}<br>active ${active} ms · session ${session} ms</div><pre style="max-height:none;margin:6px 0 0">${hex}</pre>`;
}
let lastStoreSig='';
function updatePanels(){
 const s=readState();sim.lastState=s;
 $('clock').textContent=fmtClock(sim.now);
 $('led').classList.toggle('on',s.stage===2?!!sim.raw.foot:s.active);
 $('pose').textContent=s.stage===4?`pose ${POSES[M._vex_pose()]||'?'}`:'';
 $('notice').textContent=s.calibrating?'MUTE STRINGS...':s.notice;
 if(s.notice!==sim.lastNotice){sim.lastNotice=s.notice;sim.notices.push(`[${fmtClock(sim.now)}] ${s.notice}`);if(sim.notices.length>60)sim.notices.shift();$('notices').textContent=sim.notices.join('\n');}
 for(const el of document.querySelectorAll('.ctl')){const n=el.dataset.control;if(sim.raw[n])el.querySelector('.held').textContent=`held ${sim.now-heldSince[n]} ms`;}
 const banner=$('banner');banner.classList.toggle('show',s.stage!==4);
 banner.textContent=s.stage===2?'Stage 2 · screen and input check: the display only mirrors control levels; practice, saves and the tuner are inert by design.':s.stage===3?'Stage 3 · audio check: level meters only; controls are inert by design.':'';
 const rms=$('m-rms'),gate=Math.min(1,s.gate/.25);rms.style.width=`${Math.min(100,s.rms/.25*100)}%`;$('m-gate').style.left=`${gate*100}%`;$('t-rms').textContent=s.rms.toFixed(4);
 const peak=$('m-peak');peak.style.width=`${Math.min(100,s.peak*100)}%`;peak.classList.toggle('clip',s.peak>=.98);$('t-peak').textContent=s.peak.toFixed(3);
 $('t-pitch').textContent=`yin hz=${s.hz.toFixed(2)} prob=${s.probability.toFixed(2)} · midi ${s.pitch_midi} ${s.pitch_cents>=0?'+':''}${s.pitch_cents.toFixed(1)} c ${s.pitch_valid?'valid':''} · ${sim.analyses} windows`;
 $('t-yin').textContent=`analyze ${M._vex_last_analysis_us()} µs (this machine)`;
 if(sim.source&&sim.source.kind==='wav')$('wav-pos').textContent=`${(sim.source.pos/RATE).toFixed(1)} / ${(sim.source.total/RATE).toFixed(1)} s`;
 $('power').textContent=s.power?'power on':'POWER CUT · reboot to continue';
 if($('tab-state').classList.contains('sel'))$('state').innerHTML=STATE_FIELDS.map(k=>`<tr><td>${k}</td><td>${typeof s[k]==='number'?(Number.isInteger(s[k])?s[k]:s[k].toFixed(4)):s[k]}</td></tr>`).join('')+`<tr><td>notice</td><td>${s.notice}</td></tr>`;
 if($('tab-eeprom').classList.contains('sel')){const sig=Array.from(storeBytes).join(',')+s.saved_slot;if(sig!==lastStoreSig){lastStoreSig=sig;$('slot0').innerHTML=slotHtml(0,s);$('slot1').innerHTML=slotHtml(1,s);$('slot0').classList.toggle('live',s.saved_slot===0);$('slot1').classList.toggle('live',s.saved_slot===1);}}
 if(sim.logDirty){sim.logDirty=false;const pre=$('telemetry');pre.textContent=sim.telemetry.slice(-200).join('\n');pre.scrollTop=pre.scrollHeight;}
 if(sim.recDirty&&$('tab-scenario').classList.contains('sel')){sim.recDirty=false;$('rec-out').textContent=scenarioText();$('rec-count').textContent=`${sim.record.length} segments`;}
 if(dev.wctx&&$('tab-device').classList.contains('sel')){dev.wctx.putImageData(image,0,0);devDiff();}
}
let storeSig=0;
function persistIfChanged(){let h=0;for(let i=0;i<128;i++)h=(h*31+storeBytes[i])>>>0;if(h!==storeSig){storeSig=h;persist();}}

// Main loop -----------------------------------------------------------------------------------------
let lastT=performance.now();
function frame(t){
 const real=Math.min(t-lastT,250);lastT=t;
 if(!sim.paused){sim.carry+=real*sim.speed;let n=Math.floor(sim.carry);sim.carry-=n;if(n>30000)n=30000;for(let i=0;i<n;i++)step();if(n)sim.dirty=true;}
 if(sim.dirty&&(!$('cadence').checked||sim.now-sim.lastDraw>=sim.displayMs||sim.now<sim.lastDraw)){sim.lastDraw=sim.now;sim.dirty=false;M._vex_render(sim.now);ctx.putImageData(image,0,0);}
 updatePanels();persistIfChanged();requestAnimationFrame(frame);
}
function togglePause(){sim.paused=!sim.paused;$('pause').textContent=sim.paused?'Run':'Pause';$('pause').classList.toggle('on',sim.paused);}

// Wiring --------------------------------------------------------------------------------------------
function wire(){
 $('pause').onclick=togglePause;$('speed').onchange=e=>{sim.speed=Number(e.target.value);localStorage.setItem('vex.speed',e.target.value);};
 $('step1').onclick=()=>{run(1000);};$('adv5').onclick=()=>{run(300000);note('advanced 5 minutes');};
 $('stage').onchange=e=>{sim.stage=Number(e.target.value);recordCommand(`stage ${sim.stage}`);boot();};
 $('reboot').onclick=()=>{recordCommand('reboot');boot();};$('reset').onclick=()=>{if(confirm('Zero both EEPROM slots in this browser and reboot?')){recordCommand('reset');factoryReset();}};
 $('truesize').onchange=e=>$('bezel').classList.toggle('true',e.target.checked);
 $('cadence').onchange=e=>{localStorage.setItem('vex.cadence',e.target.checked?'1':'0');sim.dirty=true;};
 for(const b of document.querySelectorAll('.tabs button'))b.onclick=()=>{document.querySelectorAll('.tabs button').forEach(x=>x.classList.toggle('sel',x===b));document.querySelectorAll('.tab').forEach(x=>x.classList.toggle('sel',x.id==='tab-'+b.dataset.tab));sim.recDirty=true;lastStoreSig='';};
 const presets={play:[.1,.3],quiet:[0,0],clip:[.1,1]};
 const applyInject=()=>{const p=$('inj-preset').value;if(presets[p]){[$('inj-rms').value,$('inj-peak').value]=presets[p];}sim.inject={rms:Number($('inj-rms').value),peak:Number($('inj-peak').value),hz:Number($('inj-hz').value),prob:Number($('inj-prob').value),on:p!=='stale'};if(!sim.source)$('audio-status').textContent=sim.inject.on?'synthetic observations':'no observations (stale)';closeSpan();};
 $('inj-preset').onchange=applyInject;for(const id of ['inj-rms','inj-peak','inj-hz','inj-prob'])$(id).oninput=()=>{$('inj-preset').value='custom';applyInject();};
 $('source').onchange=async e=>{const v=e.target.value;for(const k of ['inject','synth','wav','mic'])$('src-'+k).hidden=k!==v;
  if(v==='inject')stopSource('switched');else if(v==='synth')setSource(synthSource());else if(v==='wav')stopSource('switched');else if(v==='mic')stopSource('switched');};
 $('audio-stop').onclick=()=>{stopSource('stopped');if($('source').value==='synth'){$('source').value='inject';$('source').onchange({target:$('source')});}};
 $('syn-cents').oninput=e=>$('syn-cents-t').textContent=`${Number(e.target.value)>=0?'+':''}${e.target.value} c`;$('syn-db').oninput=e=>$('syn-db-t').textContent=`${e.target.value} dBFS`;
 $('wav-play').onclick=async()=>{try{$('wav-play').disabled=true;const pcm=await loadWav();setSource(wavSource(pcm));note('playing test recording');}catch(e){note('WAV failed: '+e.message);}finally{$('wav-play').disabled=false;}};
 $('mic-start').onclick=async()=>{if(mic){stopSource('stopped');return;}try{setSource(await micSource());$('mic-start').textContent='Stop Mac input';note('Mac input started');}catch(e){$('mic-info').textContent='Microphone unavailable: '+e.message;}};
 $('save').onclick=()=>{recordCommand('save');M._vex_serial(115,sim.now);sim.dirty=true;};
 $('corrupt').onclick=()=>{const s=readState();if(s.saved_slot<0)return;recordCommand('corrupt');storeBytes[64*s.saved_slot+40]^=1;M._vex_store_load();lastStoreSig='';};
 $('cut-arm').onclick=()=>{const n=Number($('cut-n').value);recordCommand(`powercut ${n}`);M._vex_powercut(n);note(`power cut armed after ${n} byte writes`);};
 $('rec-copy').onclick=async()=>{const t=scenarioText();try{await navigator.clipboard.writeText(t);$('rec-count').textContent='copied';}catch(e){$('rec-out').textContent=t;}};
 $('rec-clear').onclick=()=>{sim.record=[];sim.span=null;closeSpan();sim.recDirty=true;};
 $('scn-run').onclick=async()=>{$('scn-out').textContent='running…';$('scn-out').textContent=await runScenario($('scn-in').value,$('scn-fresh').checked);};
 for(const b of document.querySelectorAll('[data-send]'))b.onclick=()=>devSend(b.dataset.send);
 $('dev-adopt').onclick=devAdopt;
}
// Device tab: mirror of a plugged-in Teensy through the tools/web.py bridge ------------------------------
// The page long-polls /device for state + telemetry, fetches the raw RGB565 frame when a band arrived,
// draws it next to this page's frame and counts differing pixels. Buttons send one serial character.
const dev={gen:-1,frameGen:-1,status:'off',state:null,telemetry:[],image:null,ctx:null,wctx:null,rgb:null,backoff:0,busy:false};
function devCanvas(){if(dev.ctx)return;dev.ctx=$('dev-screen').getContext('2d');dev.wctx=$('dev-wasm').getContext('2d');dev.image=dev.ctx.createImageData(240,240);}
function devRgb565(bytes){const d=dev.image.data;for(let i=0;i<240*240;i++){const v=bytes[2*i]|bytes[2*i+1]<<8;d[4*i]=((v>>11)&31)*255/31|0;d[4*i+1]=((v>>5)&63)*255/63|0;d[4*i+2]=(v&31)*255/31|0;d[4*i+3]=255;}}
function devDiff(){if(!dev.image||dev.frameGen<0)return;const a=dev.image.data,b=image.data;let n=0;for(let i=0;i<240*240*4;i+=4){if(a[i]!==b[i]||a[i+1]!==b[i+1]||a[i+2]!==b[i+2])n++;}
 $('dev-diff').textContent=n?`${n} px differ from the real panel`:'identical to the real panel';return n;}
async function devFrame(){const r=await fetch('/device/frame');const buf=new Uint8Array(await r.arrayBuffer());if(buf.length!==240*240*2)return;dev.frameGen=Number(r.headers.get('X-Frame-Generation'));devRgb565(buf);dev.ctx.putImageData(dev.image,0,0);}
function spark(id,rows,key,fmt){const c=$(id),g=c.getContext('2d');g.clearRect(0,0,c.width,c.height);const v=rows.map(r=>Number(r[key])||0);if(!v.length){$(id+'-v').textContent='';return;}
 const max=Math.max(1,...v);g.strokeStyle='#c9a24a';g.lineWidth=1.5;g.beginPath();v.forEach((x,i)=>{const px=i/(Math.max(1,v.length-1))*(c.width-2)+1,py=c.height-1-(x/max)*(c.height-4);i?g.lineTo(px,py):g.moveTo(px,py);});g.stroke();
 g.fillStyle='#9a9199';g.font='10px ui-monospace,monospace';g.fillText(fmt(max),2,10);$(id+'-v').textContent=fmt(v[v.length-1]);}
function devRender(j){
 const st=$('dev-status');st.className='sub mono '+(j.status==='connected'?'ok':j.status==='off'?'':'bad');
 st.textContent=j.status==='off'?'bridge off · run make sim DEVICE=auto':j.status==='connected'?`${j.port} · live${j.bench_log?` · log ${j.bench_log} (make bench-report)`:''}`:j.status==='waiting'?`waiting for a Teensy (${j.error||'none on USB'})`:`error · ${j.error}`;
 const s=j.state;dev.state=s;
 if(s){const flags=['running','calibrated','active','tuning','calibrating','claimed','codec','pitch_valid'].filter(k=>s[k]).join(' ')||'idle';
  $('dev-state').innerHTML=`stage ${s.stage} · <b>${s.anim_name}</b>/${s.frame} · level ${s.level} · fragments ${s.fragments} · session ${Math.floor(s.session_ms/1000)} s · total ${Math.floor(s.active_ms/1000)} s · goal ${s.goal_minutes} min · slot ${s.saved_slot}<br>`+
   `flush ${s.flush_us} µs (max ${s.flush_max_us}) · loop max ${s.loop_max_us} µs · tx ${s.tx_bytes} B · yin ${s.yin_us} µs (max ${s.yin_max_us}) · audio cpu ${s.cpu.toFixed(1)} % · cadence ${s.cadence_mul}× · ${flags}`;
  $('dev-frame-info').textContent=j.frame_complete?`device ${fmtClock(s.now_ms)}`:'waiting for a full frame (press Link)';}
 else{$('dev-state').textContent=j.status==='connected'?'telemetry only so far; press Link to stream the panel':'';$('dev-frame-info').textContent='';}
 const rows=j.telemetry||[],last=rows[rows.length-1];
 $('dev-knobs').textContent=last&&last.spi_hz!==undefined?`spi ${(last.spi_hz/1e6).toFixed(0)} MHz · line-in ${last.in_level} · test ${last.test?'on':'off'} · cadence ${last.cadence_ms} ms · diff ${last.diff?'on':'off'}${last.full?' · full':''}`:'';
 spark('sp-tx',rows,'tx_bytes',v=>`${v} B`);spark('sp-flush',rows,'flush_us',v=>`${v} µs`);spark('sp-yin',rows,'yin_us',v=>`${v} µs`);spark('sp-cpu',rows,'cpu',v=>`${Number(v).toFixed(1)} %`);
 if(j.log)$('dev-log').textContent=j.log.join('\n');
 if(M&&image){dev.wctx.putImageData(image,0,0);devDiff();}
}
function devPoll(){
 devCanvas();
 const loop=()=>fetch(`/device?gen=${dev.gen}`).then(r=>r.json()).then(async j=>{dev.status=j.status;dev.gen=j.generation===undefined?-1:j.generation;
  if(j.frame_generation!==undefined&&j.frame_generation!==dev.frameGen)await devFrame();devRender(j);
  setTimeout(loop,j.status==='off'?5000:j.status==='connected'?0:1000);}).catch(()=>{devRender({status:'error',error:'bridge unreachable (static page?)'});setTimeout(loop,5000);});
 loop();
}
function devSend(c){fetch(`/device/send?c=${encodeURIComponent(c)}`).then(r=>r.json()).then(j=>{note(`device ← '${c}' ${j.sent?'sent':'not sent (no device)'}`);}).catch(()=>{});
 const taps={m:['mode',100],q:['quest',100],k:['foot',100],M:['mode',1300],Q:['quest',1300],K:['foot',1300]};
 if(taps[c]&&$('dev-mirror-taps').checked){const [name,ms]=taps[c];pressSync(name,ms);}
 if('cspr'.includes(c)&&$('dev-mirror-taps').checked){M._vex_serial(c.charCodeAt(0),sim.now);sim.dirty=true;}}
function devAdopt(){
 const s=dev.state;if(!s){note('adopt: no device state yet');return;}
 // Record v1 (core.h): magic, version, sequence, fragments, goal_index, claimed, active_ms, session_ms, checksum, padding; slot 0.
 const b=storeBytes.subarray(0,64),dv=new DataView(b.buffer,b.byteOffset,64);b.fill(0);
 dv.setUint32(0,0x56455831,true);dv.setUint32(4,1,true);dv.setUint32(8,s.sequence+1,true);dv.setUint32(12,s.fragments,true);dv.setUint32(16,{5:0,10:1,20:2}[s.goal_minutes]||0,true);dv.setUint32(20,s.claimed?1:0,true);
 dv.setBigUint64(24,BigInt(s.active_ms),true);dv.setBigUint64(32,BigInt(s.session_ms),true);dv.setUint32(40,crc32(b,40),true);
 storeBytes.fill(0,64);M._vex_store_load();persist();recordComment(`adopted Teensy record: fragments ${s.fragments}, total ${Math.floor(s.active_ms/1000)} s`);boot();lastStoreSig='';
}
function liveReload(){
 let gen=-1;const el=$('reload');
 const poll=()=>fetch(`/reload?gen=${gen}`).then(r=>r.json()).then(j=>{el.className='';if(j.error){el.className='err';el.textContent='build error: see terminal';}
  else if(gen>=0&&j.generation!==gen){el.textContent='rebuilt, reloading…';location.reload();return;}else el.textContent='live reload armed';gen=j.generation;poll();}).catch(()=>{el.textContent='live reload offline (static page)';});
 poll();
}

VexModule().then(mod=>{
 M=mod;audioPtr=M._vex_audio_ptr();const sp=M._vex_store_ptr();storeBytes=M.HEAPU8.subarray(sp,sp+128);
 const fp=M._vex_frame_ptr();image=new ImageData(new Uint8ClampedArray(M.HEAPU8.buffer,fp,240*240*4),240,240);
 ctx=$('screen').getContext('2d');
 sim.displayMs=M._vex_display_ms();$('cadence-ms').textContent=String(sim.displayMs);
 sim.speed=Number(localStorage.getItem('vex.speed')||'1');$('speed').value=String(sim.speed);
 if(localStorage.getItem('vex.cadence')==='0')$('cadence').checked=false;
 restoreStore();storeSig=-1;boot();wire();closeSpan();
 $('loading').hidden=true;$('main').hidden=false;if(!/noreload/.test(location.search))liveReload();devPoll();requestAnimationFrame(frame);
}).catch(e=>{$('loading').textContent='vex.wasm failed to load: '+e.message;});
