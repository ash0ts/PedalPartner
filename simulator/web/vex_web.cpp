// Browser hardware adapter for the shared firmware. Compiled with Emscripten by tools/web.py.
// JavaScript owns the clock and the loop order (observe, controls, tick), exactly as the sketch does.
// No allocation after start; all buffers are static so WASM memory never grows and JS views stay valid.
#include <stdint.h>
#include <string.h>
#include <new>
#include <emscripten/emscripten.h>
#include "../../firmware/VEX_Pocket_Rival/app.h"
#include "../../firmware/VEX_Pocket_Rival/analyzer.h"

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
struct MemStore:vex::Store{
 uint8_t bytes[128];long cut=-1;bool powered=true;
 uint8_t read(int a)override{return a>=0&&a<128?bytes[a]:0xff;}
 void update(int a,uint8_t v)override{if(!powered||a<0||a>=128)return;if(cut==0){powered=false;return;}if(cut>0)cut--;bytes[a]=v;}
};
WasmCanvas canvas;MemStore store;vex::Analyzer analyzer;
alignas(8) uint8_t appStorage[sizeof(vex::App)];vex::App* app=nullptr;
uint8_t rgba[240*240*4];
int16_t audio[vex::Analyzer::BLOCK*64];   // up to 64 blocks per push (about 186 ms)
uint32_t lastAnalysisUs=0;
// Numeric state snapshot read by JS as a Float64 array; see STATE_FIELDS in app.js.
double state[32];
}

extern "C" {
EMSCRIPTEN_KEEPALIVE void vex_boot(int stage,uint32_t now){
 store.powered=true;store.cut=-1;analyzer=vex::Analyzer();
 if(app)app->~App();app=new(appStorage) vex::App(store,stage);app->begin(now,true);
}
EMSCRIPTEN_KEEPALIVE void vex_controls(int m,int q,int f,uint32_t now){app->controls(m,q,f,now);}
EMSCRIPTEN_KEEPALIVE void vex_tick(uint32_t now){app->tick(now);}
EMSCRIPTEN_KEEPALIVE void vex_observe(float rms,float peak,uint32_t now){app->observe(rms,peak,now);}
EMSCRIPTEN_KEEPALIVE void vex_pitch(float hz,float prob,uint32_t now){app->observePitch(hz,prob,now);}
EMSCRIPTEN_KEEPALIVE void vex_serial(int c,uint32_t now){app->command(char(c),now);}
EMSCRIPTEN_KEEPALIVE int16_t* vex_audio_ptr(){return audio;}
EMSCRIPTEN_KEEPALIVE int vex_audio_capacity(){return int(sizeof(audio)/sizeof(audio[0]));}
// Push n samples (multiple of 128) already written at vex_audio_ptr(), then run the foreground steps the sketch
// runs each loop: take accumulated level, analyze a completed window. Returns 1 when a pitch window was analyzed.
EMSCRIPTEN_KEEPALIVE int vex_audio_push(int n,uint32_t now){
 for(int i=0;i+vex::Analyzer::BLOCK<=n;i+=vex::Analyzer::BLOCK)analyzer.push(audio+i,vex::Analyzer::BLOCK);
 float r,p;if(analyzer.takeLevel(r,p))app->observe(r,p,now);
 double started=emscripten_get_now();
 int analyzed=analyzer.analyze();
 if(analyzed){lastAnalysisUs=uint32_t((emscripten_get_now()-started)*1000);app->observePitch(analyzer.hz,analyzer.probability,now);}
 return analyzed;
}
EMSCRIPTEN_KEEPALIVE uint32_t vex_last_analysis_us(){return lastAnalysisUs;}
bool testPattern=false;
// Serial `t` on the bench: render ui.h drawTestPattern instead of the app screen.
EMSCRIPTEN_KEEPALIVE void vex_test(int on){testPattern=on!=0;}
EMSCRIPTEN_KEEPALIVE uint8_t* vex_render(uint32_t now){
 if(testPattern)vex::drawTestPattern(canvas,now);else vex::drawScreen(canvas,app->view());
 for(int i=0;i<240*240;i++){uint16_t p=canvas.px[i];uint8_t r=(p>>11)&31,g=(p>>5)&63,b=p&31;
  // Same integer scaling as simulator/canvas.h so WASM frames can be compared with native goldens.
  rgba[i*4]=uint8_t(r*255/31);rgba[i*4+1]=uint8_t(g*255/63);rgba[i*4+2]=uint8_t(b*255/31);rgba[i*4+3]=255;}
 return rgba;
}
EMSCRIPTEN_KEEPALIVE uint8_t* vex_frame_ptr(){return rgba;}
EMSCRIPTEN_KEEPALIVE uint8_t* vex_store_ptr(){return store.bytes;}
// After JS copies bytes into the store (e.g. from localStorage), reload the device from them.
EMSCRIPTEN_KEEPALIVE void vex_store_load(){store.powered=true;store.cut=-1;}
EMSCRIPTEN_KEEPALIVE void vex_powercut(long n){store.cut=n;}
EMSCRIPTEN_KEEPALIVE int vex_pose(){return vex::chooseAnim(vex::quantised(app->view())).anim;}
EMSCRIPTEN_KEEPALIVE int vex_display_ms(){return int(DISPLAY_MS);}
EMSCRIPTEN_KEEPALIVE const char* vex_notice(){return app->notice;}
EMSCRIPTEN_KEEPALIVE double* vex_state_ptr(){return state;}
EMSCRIPTEN_KEEPALIVE double* vex_state(){
 const auto& p=app->progress;int i=0;
 state[i++]=double(p.active_ms);state[i++]=double(p.session_ms);state[i++]=double(p.xp());state[i++]=p.level();state[i++]=p.fragments;
 state[i++]=p.goalMs()/60000;state[i++]=p.running;state[i++]=p.calibrated;state[i++]=p.active;state[i++]=app->tuning;state[i++]=app->calibrating;
 state[i++]=p.gate;state[i++]=app->rms;state[i++]=app->peak;state[i++]=app->stage;state[i++]=app->saved_slot;state[i++]=app->sequence;
 state[i++]=store.powered;state[i++]=app->pitchNow.midi;state[i++]=app->pitchNow.cents;state[i++]=app->pitchNow.valid;state[i++]=app->uncertain;
 state[i++]=analyzer.hz;state[i++]=analyzer.probability;state[i++]=app->now;
 return state;
}
}
