// Thin scenario runner, WAV reader and schema-driven state decoder for both the browser
// workbench and `tools/web.py check` (Node). Command parsing and tone synthesis live once
// in simulator/protocol.h (exposed by WASM as vex_command); state field order lives once
// in simulator/state.h (exposed by WASM as vex_state_schema).
(function(root,factory){
 if(typeof module!=='undefined'&&module.exports)module.exports=factory();
 else root.VexScenario=factory();
})(typeof globalThis!=='undefined'?globalThis:this,function(){
 'use strict';
 const BLOCK=128,RATE=44100,BLOCK_MS=1000*BLOCK/RATE;
 const NOTE_NAMES=['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'];
 const schemaCache=new WeakMap();
 const utf8Out=typeof TextEncoder!=='undefined'?new TextEncoder():null;

 function midiNoteName(midi){
  if(!midi||midi<=0||midi>127)return '';
  return NOTE_NAMES[midi%12]+(Math.floor(midi/12)-1);
 }

 function encodeWav16Mono(samples,rate=RATE){
  const pcm=samples instanceof Int16Array?samples:Int16Array.from(samples);
  const dataBytes=pcm.length*2;
  const out=new Uint8Array(44+dataBytes);
  const dv=new DataView(out.buffer);
  const tag=(o,s)=>{for(let i=0;i<4;i++)out[o+i]=s.charCodeAt(i);};
  tag(0,'RIFF');dv.setUint32(4,36+dataBytes,true);tag(8,'WAVE');
  tag(12,'fmt ');dv.setUint32(16,16,true);dv.setUint16(20,1,true);dv.setUint16(22,1,true);
  dv.setUint32(24,rate,true);dv.setUint32(28,rate*2,true);dv.setUint16(32,2,true);dv.setUint16(34,16,true);
  tag(36,'data');dv.setUint32(40,dataBytes,true);
  for(let i=0;i<pcm.length;i++)dv.setInt16(44+i*2,pcm[i],true);
  return out;
 }

 function readRecentEvents(M){
  if(!M._vex_event_count)return [];
  const total=M._vex_event_count()>>>0;
  const first=total>64?total-64+1:1;
  const out=[];
  for(let seq=first;seq<=total;seq++){
   const p=M._vex_event(seq)>>3;
   const flags=M.HEAPF64[p+7]|0,midi=M.HEAPF64[p+3]|0;
   out.push({
    seq:M.HEAPF64[p]|0,
    onsetMs:M.HEAPF64[p+1],
    exactMs:M.HEAPF64[p+2],
    midi,
    note:midiNoteName(midi),
    cents:M.HEAPF64[p+4],
    bendCents:M.HEAPF64[p+5],
    peak:M.HEAPF64[p+6],
    pitched:(flags&1)!==0,
    legato:(flags&2)!==0
   });
  }
  return out;
 }

 function parseValue(v){
  if(v==='true'||v==='false')return v==='true';
  const n=Number(v);
  return v!==''&&!Number.isNaN(n)?n:v.replace(/_/g,' ');
 }

 function getSchema(M){
  let s=schemaCache.get(M);
  if(s)return s;
  const raw=M.UTF8ToString(M._vex_state_schema());
  const fields=[],names=[],numIdx={};
  let ni=0,si=0;
  for(const part of raw.split(',')){
   if(!part)continue;
   const [name,kind]=part.split(':');
   names.push(name);
   if(kind==='s')fields.push({name,kind,index:si++});
   else{numIdx[name]=ni;fields.push({name,kind,index:ni++});}
  }
  s={fields,names,numCount:ni,numIdx};
  schemaCache.set(M,s);
  return s;
 }

 function stateNames(M){return getSchema(M).names;}
 function fieldIndex(M,name){return getSchema(M).numIdx[name];}

 function readModuleState(M){
  const sc=getSchema(M);
  const p=M._vex_state()>>3,v=M.HEAPF64.subarray(p,p+sc.numCount),o={};
  for(const f of sc.fields){
   if(f.kind==='s')o[f.name]=M.UTF8ToString(M._vex_state_str(f.index));
   else if(f.kind==='b')o[f.name]=v[f.index]!==0;
   else o[f.name]=v[f.index];
  }
  return o;
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

 // Live block stream for the workbench "Strumming" input pill.
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

 function execLine(M,line){
  const ptr=M._vex_cmd_buf(),cap=M._vex_cmd_cap();
  const dst=M.HEAPU8.subarray(ptr,ptr+cap-1);
  let written=0;
  if(utf8Out){written=utf8Out.encodeInto(line,dst).written;}
  else{for(let i=0;i<line.length&&i<dst.length;i++)dst[written++]=line.charCodeAt(i)&0x7f;}
  M.HEAPU8[ptr+written]=0;
  return M._vex_command();
 }

 function feedPcm(M,pcm){
  const audioPtr=M._vex_audio_ptr(),cap=M._vex_audio_capacity();
  M._vex_feed_begin();
  for(let i=0;i<pcm.length;i+=cap){
   const chunk=pcm.subarray(i,Math.min(pcm.length,i+cap));
   M.HEAP16.set(chunk,audioPtr>>1);
   M._vex_feed_samples(chunk.length);
  }
 }

 function renderRgba(M){
  const ptr=M._vex_render(M._vex_now());
  return M.HEAPU8.subarray(ptr,ptr+240*240*4);
 }

 function diffRgba(a,b){
  let n=0;
  for(let i=0;i<240*240*4;i+=4){if(a[i]!==b[i]||a[i+1]!==b[i+1]||a[i+2]!==b[i+2])n++;}
  return n;
 }

 async function runScenario(M,text,opts){
  const o=Object.assign({fresh:true,upToLine:Infinity,checkExpect:true,label:''},opts);
  if(o.fresh)M._vex_reset();
  let last=readModuleState(M);
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
     const name=a[1],rgba=new Uint8Array(renderRgba(M));
     let diff=null;
     if(o.onGolden)diff=await o.onGolden(name,lineNo,rgba,where);
     if(typeof diff==='number'&&diff>0){failures++;out.push(`FAIL ${where}: golden ${name} differs in ${diff} pixels`);}
     else if(typeof diff==='number')out.push(`PASS ${where}: golden ${name} (0 px diff)`);
     goldens.push({name,line:lineNo,diff});
     continue;
    }
    if(cmd==='wav'||cmd==='raw'){
     if(!o.loadWav)throw Error('no WAV loader configured');
     const pcm=await o.loadWav(a[1],cmd);
     const start=Math.min(pcm.length,Math.floor(Number(a[2]||0)*44.1));
     const len=a[3]!==undefined?Number(a[3]):-1;
     const end=len<0?pcm.length:Math.min(pcm.length,start+Math.floor(len*44.1));
     feedPcm(M,pcm.subarray(start,end));
    }else{
     const rc=execLine(M,line);
     if(rc!==1&&rc!==0)throw Error(`command error (${rc}) on "${line}"`);
    }
    if(M._vex_store_dirty()&&o.onPersist)o.onPersist();
    renderRgba(M);
    last=readModuleState(M);
   }catch(e){failures++;out.push(`FAIL ${where}: ${e.message}`);}
  }
  return {failures,lines:out,state:last,goldens};
 }

 return {
  BLOCK,RATE,BLOCK_MS,
  midiNoteName,encodeWav16Mono,readRecentEvents,
  parseValue,stateNames,fieldIndex,readModuleState,
  parseWavLeft16,createStrumSource,
  execLine,feedPcm,renderRgba,diffRgba,runScenario
 };
});
