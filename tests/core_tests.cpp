#include "../firmware/VEX_Pocket_Rival/core.h"
#include "../firmware/VEX_Pocket_Rival/app.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
struct MemStore:vex::Store{uint8_t b[128]{};uint8_t read(int a)override{return b[a];}void update(int a,uint8_t v)override{b[a]=v;}};
static void appTests(){
 MemStore store;vex::App app(store,4);app.begin(0,true);
 assert(!app.progress.running&&!app.progress.calibrated&&std::strcmp(app.notice,"Hold QUEST: calibrate")==0);
 // Gate derives from the median of fresh calibration samples: max(0.001, 4 x median).
 app.command('c',100);assert(app.calibrating);
 for(uint32_t t=101;t<=3300;t++){app.observe(t<1500?.009f:.003f,.006f,t);app.tick(t);}
 assert(!app.calibrating&&app.progress.calibrated&&fabsf(app.progress.gate-.012f)<1e-6f);
 // Notices expire after three seconds in the view, but persist while uncalibrated.
 app.message("hello");assert(app.notice_at==3300);
 // Stage 2 and 3 never act on controls or credit time; a missing codec refuses calibration.
 vex::App bench(store,2);bench.begin(0,false);for(uint32_t t=1;t<400;t++)bench.controls(false,t<200,false,t);
 assert(!bench.progress.calibrated&&!bench.calibrating&&bench.raw_quest==false);
 vex::App nocodec(store,4);nocodec.begin(0,false);nocodec.command('c',5);assert(!nocodec.calibrating&&std::strcmp(nocodec.notice,"Check audio board")==0);
 // Saving writes 52 bytes in order and a second identical save is skipped.
 app.progress.active_ms=6000;assert(app.save()&&app.saved_slot==0&&app.sequence==1);assert(!app.save()&&std::strcmp(app.notice,"Already saved")==0);
 vex::App again(store,4);again.begin(0,true);assert(again.progress.active_ms==6000&&again.sequence==1&&!again.progress.running);
}
#include "../firmware/VEX_Pocket_Rival/analyzer.h"
#include <cstdlib>
#include <vector>
// Feed a generated signal through the analyzer in 128-sample blocks and return the last estimate.
template<class F> static void feed(vex::Analyzer& a,int samples,F signal,float& hz,float& probability,float* rms=nullptr,float* peak=nullptr){
 int16_t block[vex::Analyzer::BLOCK];float r=0,p=0;
 for(int i=0;i<samples;i+=vex::Analyzer::BLOCK){
  for(int j=0;j<vex::Analyzer::BLOCK;j++){float v=signal(i+j);if(v>1)v=1;if(v<-1)v=-1;block[j]=int16_t(v*32767);}
  a.push(block,vex::Analyzer::BLOCK);float br,bp;if(a.takeLevel(br,bp)){r=br;p=bp;}if(a.analyze()){hz=a.hz;probability=a.probability;}
 }
 if(rms){*rms=r;}
 if(peak){*peak=p;}
}
static void analyzerTests(){
 const float SR=44100;auto a=std::make_unique<vex::Analyzer>();float hz=0,p=0,rms=0,peak=0;
 // Pure tones across the guitar range, with amplitude and tuning offsets.
 for(float f:{82.41f,110.f,146.83f,196.f,246.94f,329.63f,440.f,659.26f,1318.5f}){
  feed(*a,SR*0.3f,[&](int n){return .1f*sinf(2*float(M_PI)*f*n/SR);},hz,p);
  assert(p>.95f&&fabsf(1200*log2f(hz/f))<3);
 }
 // Harmonic-rich plucked-string-like tone with a second harmonic stronger than the fundamental: no octave error.
 float f0=110.f;
 feed(*a,SR*0.3f,[&](int n){float t=n/SR;return .25f*(.6f*sinf(2*float(M_PI)*f0*t)+.9f*sinf(2*float(M_PI)*2*f0*t)+.4f*sinf(2*float(M_PI)*3*f0*t)+.2f*sinf(2*float(M_PI)*4*f0*t));},hz,p);
 assert(p>.9f&&fabsf(1200*log2f(hz/f0))<5);
 // Decaying sawtooth at low E stays locked while it fades.
 feed(*a,SR*0.5f,[&](int n){float t=n/SR,ph=fmodf(82.41f*t,1.f);return .3f*expf(-3*t)*(2*ph-1);},hz,p);
 assert(p>.9f&&fabsf(1200*log2f(hz/82.41f))<5);
 // Noise is not a note; silence yields no estimate; a DC-free square wave still reports its fundamental.
 srand(7);feed(*a,SR*0.3f,[&](int){return .2f*(rand()/float(RAND_MAX)*2-1);},hz,p);assert(p<.9f);
 feed(*a,SR*0.2f,[&](int){return 0.f;},hz,p,&rms,&peak);assert(hz==0&&p==0&&rms==0&&peak==0);
 feed(*a,SR*0.3f,[&](int n){return fmodf(196.f*n/SR,1.f)<.5f?.1f:-.1f;},hz,p,&rms,&peak);
 assert(fabsf(1200*log2f(hz/196.f))<5&&fabsf(rms-.1f)<.002f&&fabsf(peak-.1f)<.002f);
 // Level accumulates across blocks until taken, so a clipped block during a slow loop is still seen.
 int16_t quiet[vex::Analyzer::BLOCK]={},loud[vex::Analyzer::BLOCK];for(auto& s:loud)s=32767;
 a->takeLevel(rms,peak);a->push(loud,vex::Analyzer::BLOCK);for(int i=0;i<20;i++)a->push(quiet,vex::Analyzer::BLOCK);
 assert(a->takeLevel(rms,peak)&&peak>.99f&&rms<.3f&&!a->takeLevel(rms,peak));
 // App tuner smoothing: a single glitch is ignored and a note holds until the estimate leaves +-60 cents.
 MemStore store;vex::App app(store,4);app.begin(0,true);
 app.observePitch(440,1,1);app.observePitch(440,1,2);app.observePitch(880,1,3);assert(app.pitchNow.midi==69);
 app.observePitch(440,1,4);app.observePitch(440,1,5);
 for(int i=0;i<3;i++){app.observePitch(454.f,1,6+i);}
 assert(app.pitchNow.midi==69&&app.pitchNow.cents>50);
 for(int i=0;i<3;i++){app.observePitch(458.f,1,9+i);}
 assert(app.pitchNow.midi==70&&app.pitchNow.cents<0);
 app.observePitch(0,0,12);assert(!app.pitchNow.valid&&app.hz_count==0);
}
int main(){
 appTests();analyzerTests();
 vex::Progress p;p.tick(100,.1,.2,true);assert(p.active_ms==0);p.calibrated=true;p.running=true;
 p.tick(100,.1,.2,true);assert(p.active_ms==100);p.tick(100,0,0,true);assert(!p.active&&p.active_ms==100);
 p.tick(100,.2,.99,true);p.tick(101,.2,.2,true);p.tick(100,.2,.2,false);p.tick(100,NAN,.2,true);assert(p.active_ms==100);
 p.session_ms=p.goalMs()-100;p.tick(100,.2,.3,true);assert(p.fragments==1&&p.claimed);for(int i=0;i<20;i++)p.tick(100,.2,.3,true);assert(p.fragments==1);
 assert(!p.setGoal(1));p.running=false;assert(p.setGoal(1));assert(p.session_ms==0&&!p.claimed&&p.active_ms>0);assert(!p.setGoal(3));
 auto a=vex::pitch(440,.99f);assert(a.valid&&a.midi==69&&fabs(a.cents)<.001);assert(!vex::pitch(440,.2).valid);assert(!vex::pitch(0,.99).valid);assert(!vex::pitch(INFINITY,1).valid);auto e=vex::pitch(82.40689,.99);assert(e.valid&&e.midi==40&&fabs(e.cents)<.01);
 auto r=vex::record(p,7);assert(vex::valid(r));auto bad=r;bad.active_ms^=1;assert(!vex::valid(bad));
 for(size_t n=0;n<offsetof(vex::Record,checksum);n++){auto corrupt=r;((unsigned char*)&corrupt)[n]^=1;assert(!vex::valid(corrupt));}
 auto torn=r;torn.magic=0;assert(!vex::valid(torn));assert(vex::newer(0,0xffffffff));assert(!vex::newer(0xffffffff,0));
 vex::Progress restored;vex::restore(restored,r);assert(restored.active_ms==p.active_ms&&restored.fragments==p.fragments&&!restored.running&&!restored.calibrated);
 vex::Button b;assert(b.update(true,10)==0);assert(b.update(false,20)==0);assert(b.update(false,50)==0);assert(b.update(true,100)==0);assert(b.update(true,130)==0);assert(b.update(false,200)==0);assert(b.update(false,230)==1);
 assert(b.update(true,300)==0);assert(b.update(true,330)==0);assert(b.update(true,1530)==2);assert(b.update(false,1600)==0);assert(b.update(false,1630)==0);
 puts("PASS: gating, clipping, stale input, quest exactly-once, goals, pitch math, CRC corruption, record restore, sequence rollover, button debounce and hold, app calibration/notice/save, YIN analyzer on tones/harmonics/noise, level accumulation, tuner smoothing.");
}
