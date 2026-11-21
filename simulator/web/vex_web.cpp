// Browser hardware adapter for the shared firmware and scenario interpreter (simulator/protocol.h).
// Compiled with Emscripten by tools/web.py. No allocation after start; all buffers are static so
// WASM memory never grows and JS views stay valid.
#include <stdint.h>
#include <string.h>
#include <emscripten/emscripten.h>
#include "../protocol.h"

namespace {
struct WasmCanvas {
 uint16_t px[240*240];
 void fillScreen(uint16_t c){for(auto& p:px)p=c;}
 void fillRect(int x,int y,int w,int h,uint16_t c){
  int x0=x<0?0:x,y0=y<0?0:y,x1=x+w>240?240:x+w,y1=y+h>240?240:y+h;
  for(int yy=y0;yy<y1;yy++)for(int xx=x0;xx<x1;xx++)px[yy*240+xx]=c;
 }
 void drawFastHLine(int x,int y,int w,uint16_t c){fillRect(x,y,w,1,c);}
 void drawFastVLine(int x,int y,int h,uint16_t c){fillRect(x,y,1,h,c);}
 void fillCircle(int x,int y,int r,uint16_t c){for(int yy=-r;yy<=r;yy++)for(int xx=-r;xx<=r;xx++)if(xx*xx+yy*yy<=r*r)fillRect(x+xx,y+yy,1,1,c);}
};

WasmCanvas canvas;
vexsim::SimStore store;
vexsim::SimHost sim(store);
uint8_t rgba[240*240*4];
int16_t audio[vex::Analyzer::BLOCK*64];   // up to 64 blocks per push (about 186 ms)
char cmdBuf[1024];
uint32_t lastAnalysisUs=0;

// Populated from simulator/state.h so numeric indices and field names have a single definition.
double state[64];
const char* stateStrings[16];
char schemaBuf[1024];
bool schemaBuilt=false;
}

extern "C" {
EMSCRIPTEN_KEEPALIVE void vex_boot(int stage,uint32_t now){
 sim.stage=stage;
 sim.boot(now);
}
EMSCRIPTEN_KEEPALIVE void vex_reset(){sim.reset();lastAnalysisUs=0;}
EMSCRIPTEN_KEEPALIVE uint32_t vex_now(){return sim.app().now;}
EMSCRIPTEN_KEEPALIVE void vex_controls(int m,int q,int f,uint32_t now){
 sim.raw_mode=m!=0;sim.raw_quest=q!=0;sim.raw_foot=f!=0;
 sim.app().controls(sim.raw_mode,sim.raw_quest,sim.raw_foot,now);
}
EMSCRIPTEN_KEEPALIVE void vex_tick(uint32_t now){sim.app().tick(now);}
EMSCRIPTEN_KEEPALIVE void vex_observe(float rms,float peak,uint32_t now){sim.app().observe(rms,peak,now);}
EMSCRIPTEN_KEEPALIVE void vex_pitch(float hz,float prob,uint32_t now){sim.app().observePitch(hz,prob,now);}
EMSCRIPTEN_KEEPALIVE void vex_serial(int c,uint32_t now){sim.app().command(char(c),now);}
EMSCRIPTEN_KEEPALIVE int16_t* vex_audio_ptr(){return audio;}
EMSCRIPTEN_KEEPALIVE int vex_audio_capacity(){return int(sizeof(audio)/sizeof(audio[0]));}

// Push n samples (multiple of 128) written at vex_audio_ptr(), then run foreground level, onsets + pitch.
EMSCRIPTEN_KEEPALIVE int vex_audio_push(int n,uint32_t now){
 sim.analyzer.setGate(sim.app().progress.gate);
 for(int i=0;i+vex::Analyzer::BLOCK<=n;i+=vex::Analyzer::BLOCK)sim.analyzer.push(audio+i,vex::Analyzer::BLOCK);
 float r,p;uint32_t sc=0;
 if(sim.analyzer.takeLevel(r,p,&sc))sim.app().observe(r,p,now,sc);
 vex::Onset ons;while(sim.analyzer.takeOnset(ons))sim.app().observeOnset(ons,now);
 vex::PitchMode pm=sim.app().pitchMode();
 if(pm==vex::PITCH_OFF){sim.analyzer.skip();lastAnalysisUs=0;return 0;}
 double started=emscripten_get_now();
 int analyzed=sim.analyzer.analyze(pm,sim.app().slow_run_hop);
 if(analyzed){
  lastAnalysisUs=uint32_t((emscripten_get_now()-started)*1000);
  sim.analyses++;
  sim.app().observePitch(sim.analyzer.hz,sim.analyzer.probability,now,sim.analyzer.pitch_sample,
                         vex::pitchProfile(pm,sim.app().slow_run_hop).window);
 }
 return analyzed;
}

double eventOut[8];
EMSCRIPTEN_KEEPALIVE uint32_t vex_event_count(){return sim.app().notes.total_events;}
EMSCRIPTEN_KEEPALIVE double* vex_event(uint32_t seq){
 const auto& nt=sim.app().notes;
 if(seq==0||seq>nt.total_events||nt.total_events-seq>=vex::NoteTracker::EVENT_CAP){
  for(double& v:eventOut)v=0;
  return eventOut;
 }
 const auto& e=nt.atSeq(seq);
 // Convert sample-based onset_ms_exact to current session clock domain for the 30 s strip.
 eventOut[0]=double(e.seq);
 eventOut[1]=double(e.onset_ms);
 eventOut[2]=double(e.onset_ms_exact);
 eventOut[3]=double(e.midi);
 eventOut[4]=double(e.cents);
 eventOut[5]=double(e.bend_cents);
 eventOut[6]=double(e.peak);
 eventOut[7]=double((e.pitched?1:0)|(e.legato?2:0));
 return eventOut;
}

// Scenario protocol execution (simulator/protocol.h) and streamed PCM feeding for `wav`/`raw`.
EMSCRIPTEN_KEEPALIVE char* vex_cmd_buf(){return cmdBuf;}
EMSCRIPTEN_KEEPALIVE int vex_cmd_cap(){return int(sizeof(cmdBuf));}
EMSCRIPTEN_KEEPALIVE int vex_command(){return int(sim.exec(cmdBuf));}
EMSCRIPTEN_KEEPALIVE void vex_feed_begin(){sim.beginFeed();}
EMSCRIPTEN_KEEPALIVE void vex_feed_samples(int n){if(n>0)sim.feedSamples(audio,size_t(n));}

EMSCRIPTEN_KEEPALIVE uint32_t vex_last_analysis_us(){return lastAnalysisUs;}
EMSCRIPTEN_KEEPALIVE void vex_test(int on){sim.test=on!=0;}
EMSCRIPTEN_KEEPALIVE uint8_t* vex_render(uint32_t now){
 if(sim.test)vex::drawTestPattern(canvas,now);else vex::drawScreen(canvas,sim.app().view());
 for(int i=0;i<240*240;i++){uint16_t p=canvas.px[i];uint8_t r=(p>>11)&31,g=(p>>5)&63,b=p&31;
  rgba[i*4]=uint8_t(r*255/31);rgba[i*4+1]=uint8_t(g*255/63);rgba[i*4+2]=uint8_t(b*255/31);rgba[i*4+3]=255;}
 return rgba;
}
EMSCRIPTEN_KEEPALIVE uint8_t* vex_frame_ptr(){return rgba;}
EMSCRIPTEN_KEEPALIVE uint8_t* vex_store_ptr(){return store.bytes;}
EMSCRIPTEN_KEEPALIVE int vex_store_size(){return vexsim::SimStore::CAPACITY;}
EMSCRIPTEN_KEEPALIVE void vex_store_load(){store.powered=true;store.cut=-1;store.dirty=false;}
EMSCRIPTEN_KEEPALIVE int vex_store_dirty(){int d=store.dirty?1:0;store.dirty=false;return d;}
EMSCRIPTEN_KEEPALIVE void vex_powercut(long n){store.cut=n;}
EMSCRIPTEN_KEEPALIVE int vex_pose(){return vex::chooseAnim(vex::quantised(sim.app().view())).anim;}
EMSCRIPTEN_KEEPALIVE int vex_display_ms(){return int(DISPLAY_MS);}
EMSCRIPTEN_KEEPALIVE const char* vex_notice(){return sim.app().notice;}

EMSCRIPTEN_KEEPALIVE const char* vex_state_schema(){
 if(!schemaBuilt){
  struct SchemaEmit{
   size_t pos=0;
   void add(const char* k,char kind){
    if(pos&&pos+1<sizeof schemaBuf)schemaBuf[pos++]=',';
    for(const char* p=k;*p&&pos+3<sizeof schemaBuf;++p)schemaBuf[pos++]=*p;
    schemaBuf[pos++]=':';schemaBuf[pos++]=kind;schemaBuf[pos]='\0';
   }
   void u(const char* k,uint64_t){add(k,'i');}
   void i(const char* k,int64_t){add(k,'i');}
   void f(const char* k,float){add(k,'f');}
   void b(const char* k,bool){add(k,'b');}
   void s(const char* k,const char*){add(k,'s');}
  } emit;
  vexsim::stateFields(sim.stateSource(),emit);
  schemaBuilt=true;
 }
 return schemaBuf;
}

EMSCRIPTEN_KEEPALIVE double* vex_state_ptr(){return state;}
EMSCRIPTEN_KEEPALIVE const char* vex_state_str(int idx){return (idx>=0&&idx<16&&stateStrings[idx])?stateStrings[idx]:"";}
EMSCRIPTEN_KEEPALIVE double* vex_state(){
 struct WasmEmit{
  int ni=0,si=0;
  void u(const char*,uint64_t v){state[ni++]=double(v);}
  void i(const char*,int64_t v){state[ni++]=double(v);}
  void f(const char*,float v){state[ni++]=double(v);}
  void b(const char*,bool v){state[ni++]=v?1.0:0.0;}
  void s(const char*,const char* v){if(si<16)stateStrings[si++]=v?v:"";}
 } emit;
 vexsim::stateFields(sim.stateSource(),emit);
 return state;
}
}
