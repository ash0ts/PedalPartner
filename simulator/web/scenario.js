// Shared scenario protocol, WAV reader and guitar pluck synthesiser for both the browser
// workbench and `tools/web.py check` (Node). Mirrors simulator/main.cpp step for step.
(function(root,factory){
 if(typeof module!=='undefined'&&module.exports)module.exports=factory();
 else root.VexScenario=factory();
})(typeof globalThis!=='undefined'?globalThis:this,function(){
 'use strict';
 const BLOCK=128,RATE=44100,BLOCK_MS=1000*BLOCK/RATE;
 const STATE_FIELDS=[
  'active_ms','session_ms','xp','level','fragments','goal_minutes',
  'running','calibrated','active','tuning','calibrating',
  'gate','rms','peak','stage','saved_slot','sequence','power',
  'pitch_midi','pitch_cents','pitch_valid','uncertain','hz','probability','now_ms',
  'env','hold_ms','credit_drops','anim','frame','test'
 ];
 const BOOLS=new Set(['running','calibrated','active','tuning','calibrating','power','pitch_valid','uncertain','test']);

 function parseValue(v){
  if(v==='true'||v==='false')return v==='true';
  const n=Number(v);
  return v!==''&&!Number.isNaN(n)?n:v.replace(/_/g,' ');
 }

 // Exact port of simulator/wav.h readWav: 16-bit 44.1 kHz RIFF/WAVE, left channel, no WebAudio downmix.
 function parseWavLeft16(buffer){
  const u8=buffer instanceof Uint8Array?buffer:new Uint8Array(buffer);
  const dv=new DataView(u8.buffer,u8.byteOffset,u8.byteLength);
  const tag=o=>String.fromCharCode(u8[o],u8[o+1],u8[o+2],u8[o+3]);
  if(u8.byteLength<12||tag(0)!=='RIFF'||tag(8)!=='WAVE')throw Error('not a RIFF/WAVE file');
  let channels=0,bits=0,rate=0,pos=12,data=0,size=0;
  while(pos+8<=u8.byteLength){
   const id=tag(pos),len=dv.getUint32(pos+4,true),body=pos+8;
   if(id==='fmt '&&len>=16){
    if(dv.getUint16(body,true)!==1)throw Error('only PCM WAV is supported');
    channels=dv.getUint16(body+2,true);rate=dv.getUint32(body+4,true);bits=dv.getUint16(body+14,true);
   }else if(id==='data'){data=body;size=len;break;}
   pos=body+len+(len&1);
  }
  if(!data||!channels)throw Error('missing fmt or data chunk');
  if(rate!==RATE||bits!==16)throw Error('need 16-bit 44100 Hz PCM');
  if(data+size>u8.byteLength)size=u8.byteLength-data;
  const frames=Math.floor(size/(2*channels)),out=new Int16Array(frames);
  for(let i=0;i<frames;i++)out[i]=dv.getInt16(data+i*2*channels,true);
  return out;
 }

 function lround(x){return x>=0?Math.floor(x+0.5):-Math.floor(-x+0.5);}

 // Exact port of simulator/main.cpp synthTone: harmonic pluck or gapped strums at 44.1 kHz.
 function synthTone(ms,hz,dbfs,decayMs=0,gapMs=0){
  const n=Math.floor(ms*RATE/1000),pcm=new Int16Array(n);
  if(hz<=0||!n)return pcm;
  const amp=Math.pow(10,dbfs/20),ringMs=decayMs>0?decayMs:(gapMs>0?200:0),periodMs=gapMs>0?ringMs+gapMs:0;
  let phase=0;const step=2*Math.PI*hz/RATE;
  for(let i=0;i<n;i++){
   const tMs=i*1000/RATE,inMs=periodMs>0?tMs%periodMs:tMs;
   if(gapMs>0&&inMs>=ringMs){phase=0;continue;}
   const env=decayMs>0?Math.exp(-(gapMs>0?1.5:1)*inMs/decayMs):1;
   phase+=step;if(phase>2*Math.PI)phase-=2*Math.PI;
   const v=Math.max(-1,Math.min(1,amp*env*(Math.sin(phase)+0.45*Math.sin(2*phase)+0.15*Math.sin(3*phase))/1.6));
   pcm[i]=lround(v*32767);
  }
  return pcm;
 }

 // Continuous block stream using the same gapped-pluck model for the live Strumming input.
 function createStrumSource(opts){
  const cfg=Object.assign({hz:110,dbfs:-20,decayMs:280,gapMs:280},opts);
  const notes=[82.4069,110,146.832,195.998,146.832,110];
  const buf=new Int16Array(BLOCK);let sample=0,phase=0,lastCycle=-1;
  return {
   kind:'strum',
   label:'Strumming (plucks + 280 ms chord gaps)',
   next(){
    const periodMs=cfg.decayMs+cfg.gapMs,amp=Math.pow(10,cfg.dbfs/20);
    for(let i=0;i<BLOCK;i++,sample++){
     const tMs=sample*1000/RATE,cyc=Math.floor(tMs/periodMs),inMs=tMs-cyc*periodMs;
     if(cyc!==lastCycle){lastCycle=cyc;phase=0;}
     if(inMs>=cfg.decayMs){buf[i]=0;continue;}
     const hz=cfg.chordNotes?notes[cyc%notes.length]:cfg.hz;
     phase+=2*Math.PI*hz/RATE;if(phase>2*Math.PI)phase-=2*Math.PI;
     const env=Math.exp(-1.5*inMs/cfg.decayMs);
     const v=Math.max(-1,Math.min(1,amp*env*(Math.sin(phase)+0.45*Math.sin(2*phase)+0.15*Math.sin(3*phase))/1.6));
     buf[i]=lround(v*32767);
    }
    return buf;
   }
  };
 }

 function readModuleState(M,analyses){
  const p=M._vex_state()>>3,v=M.HEAPF64.subarray(p,p+STATE_FIELDS.length),o={};
  for(let i=0;i<STATE_FIELDS.length;i++){const k=STATE_FIELDS[i];o[k]=BOOLS.has(k)?v[i]!==0:v[i];}
  o.notice=M.UTF8ToString(M._vex_notice());
  o.analyses=analyses||0;
  return o;
 }

 // Standalone or page-backed driver around a loaded VexModule instance.
 function createDriver(M,hooks){
  const h=hooks||{};
  const audioPtr=M._vex_audio_ptr(),storePtr=M._vex_store_ptr();
  const storeBytes=M.HEAPU8.subarray(storePtr,storePtr+128);
  let stage=h.getStage?h.getStage():4;
  let now=h.getNow?h.getNow():0;
  let analyses=h.getAnalyses?h.getAnalyses():0;
  const raw={mode:0,quest:0,foot:0};
  let inject=null;
  const syncOut=()=>{if(h.setState)h.setState({stage,now,analyses});};
  const syncIn=()=>{if(h.getStage)stage=h.getStage();if(h.getNow)now=h.getNow();if(h.getAnalyses)analyses=h.getAnalyses();};
  function boot(){
   now=0;M._vex_boot(stage,0);syncOut();if(h.onBoot)h.onBoot(stage);
  }
  function reset(){
   storeBytes.fill(0);M._vex_store_load();M._vex_test(0);stage=4;analyses=0;boot();if(h.onReset)h.onReset();
  }
  function step(){
   now++;
   if(inject){M._vex_observe(inject.rms,inject.peak,now);if(inject.hz>0)M._vex_pitch(inject.hz,inject.prob,now);}
   M._vex_controls(raw.mode,raw.quest,raw.foot,now);
   M._vex_tick(now);
  }
  function run(ms){for(let i=0;i<ms;i++)step();syncOut();}
  function press(name,ms){
   if(name!=='mode'&&name!=='quest'&&name!=='foot')throw Error(`unknown control ${name}`);
   raw[name]=1;run(ms);raw[name]=0;run(30);
  }
  function audio(ms,r,p,hz=0,prob=1){inject={rms:r,peak:p,hz,prob};run(ms);inject=null;}
  function feed(pcm){
   let sampleMs=now;const blk=new Int16Array(BLOCK);
   for(let i=0;i<pcm.length;i+=BLOCK){
    blk.fill(0);const n=Math.min(BLOCK,pcm.length-i);blk.set(pcm.subarray(i,i+n));
    M.HEAP16.set(blk,audioPtr>>1);
    if(M._vex_audio_push(BLOCK,now))analyses++;
    sampleMs+=BLOCK_MS;
    while(now+1<=Math.floor(sampleMs))step();
   }
   syncOut();
  }
  return {
   get now(){return now;},
   get stage(){return stage;},
   get analyses(){return analyses;},
   syncIn,syncOut,boot,reset,run,press,audio,feed,
   setStage(s){if(s<2||s>4)throw Error(`bad stage ${s}`);stage=s;boot();},
   serial(ch){M._vex_serial(ch.charCodeAt(0),now);},
   powercut(n){M._vex_powercut(n);},
   corrupt(){const s=readModuleState(M,analyses);if(s.saved_slot>=0){storeBytes[64*s.saved_slot+40]^=1;M._vex_store_load();if(h.onPersist)h.onPersist();}},
   test(on){M._vex_test(on?1:0);},
   readState(){return readModuleState(M,analyses);},
   renderRgba(){const ptr=M._vex_render(now);return M.HEAPU8.subarray(ptr,ptr+240*240*4);}
  };
 }

 function diffRgba(a,b){
  let n=0;
  for(let i=0;i<240*240*4;i+=4){if(a[i]!==b[i]||a[i+1]!==b[i+1]||a[i+2]!==b[i+2])n++;}
  return n;
 }

 async function runScenario(driver,text,opts){
  const o=Object.assign({fresh:true,upToLine:Infinity,checkExpect:true,label:''},opts);
  driver.syncIn();
  if(o.fresh)driver.reset();
  let last=driver.readState();
  const out=[],goldens=[];let failures=0;
  const lines=text.split(/\r?\n/);
  for(let idx=0;idx<lines.length;idx++){
   const lineNo=idx+1;if(lineNo>o.upToLine)break;
   const line=lines[idx].trim();if(!line||line.startsWith('#'))continue;
   const a=line.split(/\s+/),cmd=a[0];
   const where=o.label?`${o.label}:${lineNo}`:`line ${lineNo}`;
   try{
    if(cmd==='expect'){
     if(o.checkExpect){
      for(const pair of a.slice(1)){
       const eq=pair.indexOf('=');let k=pair.slice(0,eq);const v=pair.slice(eq+1);
       const approx=k.endsWith('~');k=k.replace(/~$/,'');
       const want=parseValue(v),got=last[k];
       let ok;
       if(approx)ok=typeof got==='number'&&typeof want==='number'&&Math.abs(got-want)<=200;
       else if(typeof want==='number')ok=typeof got==='number'&&Math.abs(got-want)<=1e-6;
       else ok=got===want;
       if(!ok){failures++;out.push(`FAIL ${where}: ${k} expected ${JSON.stringify(want)}, got ${JSON.stringify(got)}`);}
      }
     }
     continue;
    }
    if(cmd==='golden'){
     const name=a[1],rgba=new Uint8Array(driver.renderRgba());
     let diff=null;
     if(o.onGolden)diff=await o.onGolden(name,lineNo,rgba,where);
     if(typeof diff==='number'&&diff>0){failures++;out.push(`FAIL ${where}: golden ${name} differs in ${diff} pixels`);}
     else if(typeof diff==='number')out.push(`PASS ${where}: golden ${name} (0 px diff)`);
     goldens.push({name,line:lineNo,diff});
     continue;
    }
    if(cmd==='press')driver.press(a[1],Number(a[2]||100));
    else if(cmd==='audio')driver.audio(Number(a[1]),Number(a[2]),Number(a[3]),a[4]!==undefined?Number(a[4]):0,a[5]!==undefined?Number(a[5]):1);
    else if(cmd==='wait')driver.run(Number(a[1]));
    else if(cmd==='tone')driver.feed(synthTone(Number(a[1]),Number(a[2]),Number(a[3]),a[4]!==undefined?Number(a[4]):0,a[5]!==undefined?Number(a[5]):0));
    else if(cmd==='wav'||cmd==='raw'){
     if(!o.loadWav)throw Error('no WAV loader configured');
     const pcm=await o.loadWav(a[1],cmd);
     const start=Math.min(pcm.length,Math.floor(Number(a[2]||0)*44.1));
     const len=a[3]!==undefined?Number(a[3]):-1;
     const end=len<0?pcm.length:Math.min(pcm.length,start+Math.floor(len*44.1));
     driver.feed(pcm.subarray(start,end));
    }
    else if(cmd==='serial')driver.serial(a[1]);
    else if(cmd==='save')driver.serial('s');
    else if(cmd==='calibrate'){const r=a[1]!==undefined?Number(a[1]):0.001;driver.serial('c');driver.audio(3100,r,r*2,0,0);}
    else if(cmd==='stage')driver.setStage(Number(a[1]));
    else if(cmd==='powercut')driver.powercut(Number(a[1]));
    else if(cmd==='reboot')driver.boot();
    else if(cmd==='corrupt')driver.corrupt();
    else if(cmd==='reset')driver.reset();
    else if(cmd==='test'){if(a[1]!=='on'&&a[1]!=='off')throw Error(`bad test mode ${a[1]}`);driver.test(a[1]==='on');}
    else if(cmd!=='state')throw Error(`unknown command ${cmd}`);
    driver.renderRgba();
    last=driver.readState();
   }catch(e){failures++;out.push(`FAIL ${where}: ${e.message}`);}
  }
  return {failures,lines:out,state:last,goldens};
 }

 return {
  BLOCK,RATE,BLOCK_MS,STATE_FIELDS,BOOLS,
  parseValue,parseWavLeft16,synthTone,createStrumSource,
  readModuleState,createDriver,diffRgba,runScenario
 };
});
