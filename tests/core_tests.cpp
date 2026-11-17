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
 // Uncertain band: a candidate between the thresholds is flagged, silence and confident notes clear it,
 // and the last confident reading is kept with its time so the display can fade it.
 assert(app.last_pitch.valid&&app.last_pitch.midi==70&&app.last_pitch_at==11&&!app.uncertain);
 app.observePitch(440,.75f,13);assert(app.uncertain&&!app.pitchNow.valid&&app.last_pitch.midi==70);
 app.observePitch(440,.5f,14);assert(!app.uncertain);
 app.observePitch(30,.95f,15);assert(!app.uncertain&&!app.pitchNow.valid);
 app.observePitch(440,.95f,16);assert(!app.uncertain&&app.pitchNow.valid&&app.last_pitch_at==16);
 vex::View tv=app.view();assert(tv.last_pitch.midi==69&&!tv.uncertain);
}
#include "../firmware/VEX_Pocket_Rival/flush.h"
#include "../simulator/canvas.h"
// Display pipeline: animation frames follow the cadence exactly and the band diff sends only what changed.
static void displayTests(){
 using namespace vex;
 View v;v.stage=4;v.progress.calibrated=true;v.now=101000;v.active_at=v.now;v.fragment_at=v.levelup_at=v.summary_at=v.now-100000;v.notice_at=v.now-100000;
 // Every manifest period is a multiple of DISPLAY_MS, so one cadence step never repeats or skips a strum frame.
 v.progress.running=true;v.progress.active=true;int last=-1,steps=0;
 for(uint32_t t=0;t<4000;t+=DISPLAY_MS){View w=v;w.now=v.now+t;AnimFrame f=chooseAnim(quantised(w));assert(f.anim==ANIM_STRUM);if(last>=0){assert(f.frame==(last+1)%VEX_ANIMS[ANIM_STRUM].frames);steps++;}last=f.frame;}
 assert(steps==4000/int(DISPLAY_MS)-1);
 // Idle breathes on a one-second loop and blinks as a one-shot inside each four-second window.
 v.progress.active=false;v.progress.running=false;
 assert(chooseAnim(v).anim==ANIM_IDLE);View b=v;b.now=v.now+(UI_BLINK_EVERY_MS-v.now%UI_BLINK_EVERY_MS)%UI_BLINK_EVERY_MS;
 assert(chooseAnim(b).anim==ANIM_BLINK&&chooseAnim(b).frame==0);b.now+=VEX_ANIMS[ANIM_BLINK].frame_ms*VEX_ANIMS[ANIM_BLINK].frames;assert(chooseAnim(b).anim==ANIM_IDLE);
 // Time inside the current cadence window is pulled back to its start; "long ago" sentinels stay long ago.
 View q=v;q.now=101150;q.fragment_at=101140;q=quantised(q);assert(q.now==101100&&q.fragment_at==101100&&q.now-q.levelup_at>=UI_CELEBRATE_MS);
 // Band diff: first frame is sent whole by the adapter; an unchanged screen produces no bands; the
 // second blink frame touches only the eye rows; a new caption adds a band near the bottom only.
 static DesktopCanvas a,c;drawScreen(a,v);drawScreen(c,v);Band out[FLUSH_MAX_BANDS];
 assert(bands(a.pixels.data(),c.pixels.data(),out)==0);
 b.now-=VEX_ANIMS[ANIM_BLINK].frame_ms*VEX_ANIMS[ANIM_BLINK].frames;assert(chooseAnim(b).anim==ANIM_BLINK);
 drawScreen(a,b);b.now+=DISPLAY_MS;drawScreen(c,b);int n=bands(a.pixels.data(),c.pixels.data(),out);assert(n>=1);uint32_t rows=bandBytes(out,n)/(FRAME_W*2);assert(rows>0&&rows<FRAME_H*15/100);
 drawScreen(a,v);
 View tick=v;tick.notice="Saved - safe to unplug";tick.notice_at=v.now;drawScreen(c,tick);n=bands(a.pixels.data(),c.pixels.data(),out);assert(n>=1&&out[0].y>=180);
 // Cap: many scattered rows collapse into FLUSH_MAX_BANDS bands that still cover every change.
 static uint16_t p0[FRAME_W*FRAME_H],p1[FRAME_W*FRAME_H];memset(p0,0,sizeof p0);memset(p1,0,sizeof p1);
 for(int y=0;y<FRAME_H;y+=20){p1[y*FRAME_W]=1;}
 n=bands(p0,p1,out);assert(n==FLUSH_MAX_BANDS);
 assert(out[n-1].y+out[n-1].h==FRAME_H-19);for(int i=1;i<n;i++)assert(out[i].y>=out[i-1].y+out[i-1].h);
}
int main(){
 appTests();analyzerTests();displayTests();
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
 puts("PASS: gating, clipping, stale input, quest exactly-once, goals, pitch math, CRC corruption, record restore, sequence rollover, button debounce and hold, app calibration/notice/save, YIN analyzer on tones/harmonics/noise, level accumulation, tuner smoothing, cadence-locked animation, dirty-band flush.");
}
