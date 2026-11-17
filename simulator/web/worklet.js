// AudioWorklet capture: converts the 128-frame render quantum to 16-bit samples and posts one block per
// quantum, the same shape the Teensy audio interrupt hands to vex::Analyzer::push. The pool keeps process()
// free of allocation; the main thread copies the block into WASM memory.
class VexCapture extends AudioWorkletProcessor{
 constructor(){super();this.pool=[];for(let i=0;i<32;i++)this.pool.push(new Int16Array(128));this.slot=0;this.fill=0;this.clipped=0;}
 process(inputs){
  const ch=inputs[0]&&inputs[0][0];if(!ch)return true;
  let buf=this.pool[this.slot];
  for(let i=0;i<ch.length;i++){
   let v=ch[i]*32767;if(v>32767){v=32767;this.clipped++;}else if(v<-32768){v=-32768;this.clipped++;}
   buf[this.fill++]=v|0;
   if(this.fill===128){this.port.postMessage(buf);this.fill=0;this.slot=(this.slot+1)&31;buf=this.pool[this.slot];}
  }
  return true;
 }
}
registerProcessor('vex-capture',VexCapture);
