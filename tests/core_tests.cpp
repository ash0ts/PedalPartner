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
static int audioLockDepth=0,audioLocks=0;
#define VEX_AUDIO_LOCK() do{assert(audioLockDepth==0);audioLockDepth++;audioLocks++;}while(0)
#define VEX_AUDIO_UNLOCK() do{assert(audioLockDepth==1);audioLockDepth--;}while(0)
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
 assert(!app.wantsPitch());app.tuning=true;assert(app.wantsPitch());app.tuning=false;
 vex::App s3(store,3);s3.begin(0,true);assert(s3.wantsPitch());
 // Smoothed level + activity hold through the real block analyzer: sustained low E2 near the gate
 // credits >= 95% of its duration, and 300 ms chord-change gaps never drop active mid-phrase.
 {vex::App holdApp(store,4);holdApp.begin(0,true);holdApp.progress.calibrated=true;holdApp.progress.gate=.004f;holdApp.setRunning(true);
  auto az=std::make_unique<vex::Analyzer>();int16_t blk[vex::Analyzer::BLOCK];double clk=0,ph=0;
  const float amp=powf(10.f,-41.f/20.f);
  for(int b=0;b<int(5.0f*SR/vex::Analyzer::BLOCK);b++){
   for(int j=0;j<vex::Analyzer::BLOCK;j++){ph+=2*M_PI*82.4069/SR;float v=amp*(sinf(float(ph))+.45f*sinf(float(2*ph))+.15f*sinf(float(3*ph)))/1.6f;blk[j]=int16_t(v*32767);}
   az->push(blk,vex::Analyzer::BLOCK);clk+=1000.0*vex::Analyzer::BLOCK/SR;
   float br,bp;if(az->takeLevel(br,bp))holdApp.observe(br,bp,holdApp.now);
   while(holdApp.now+1<=uint32_t(clk))holdApp.tick(holdApp.now+1);
  }
  assert(holdApp.progress.active_ms>=4750&&holdApp.progress.active);
  // Now 6 s of 300 ms decaying strums separated by 300 ms silent gaps: active never drops on any tick.
  uint64_t before=holdApp.progress.active_ms;double startClk=clk;ph=0;
  for(int b=0;b<int(6.0f*SR/vex::Analyzer::BLOCK);b++){
   for(int j=0;j<vex::Analyzer::BLOCK;j++){
    double t_ms=(clk-startClk)+j*1000.0/SR,in_ms=fmod(t_ms,600.0);
    float v=0;if(in_ms<300.0){ph+=2*M_PI*110.0/SR;v=.1f*expf(-1.5f*float(in_ms)/300.f)*(sinf(float(ph))+.45f*sinf(float(2*ph))+.15f*sinf(float(3*ph)))/1.6f;}else ph=0;
    blk[j]=int16_t(v*32767);
   }
   az->push(blk,vex::Analyzer::BLOCK);clk+=1000.0*vex::Analyzer::BLOCK/SR;
   float br,bp;if(az->takeLevel(br,bp))holdApp.observe(br,bp,holdApp.now);
   while(holdApp.now+1<=uint32_t(clk)){holdApp.tick(holdApp.now+1);assert(holdApp.progress.active);}
  }
  assert(holdApp.progress.active_ms-before>=5900);
  // And 2 s of silence credits at most ACTIVITY_HOLD_MS, then stops.
  uint64_t atStop=holdApp.progress.active_ms;uint32_t t0=holdApp.now;
  for(uint32_t d=1;d<=2000;d++){holdApp.observe(0,0,t0+d);holdApp.tick(t0+d);}
  assert(!holdApp.progress.active&&holdApp.progress.hold_ms==0&&holdApp.progress.active_ms-atStop<=vex::ACTIVITY_HOLD_MS);}
 a->skip();assert(a->hz==0&&a->probability==0&&!a->analyze()&&audioLockDepth==0&&audioLocks>0);
}
#include "../firmware/VEX_Pocket_Rival/flush.h"
#include "../simulator/canvas.h"
// Display pipeline: animation frames follow the cadence exactly and the band diff sends only what changed.
static void displayTests(){
 using namespace vex;
 View v;v.stage=4;v.progress.calibrated=true;v.now=101000;v.active_at=v.now;v.fragment_at=v.levelup_at=v.summary_at=v.now-100000;v.notice_at=v.now-100000;
 // Every manifest period is a multiple of DISPLAY_MS, so one cadence step never repeats or skips a strum frame.
 v.progress.running=true;v.progress.active=true;int last=-1,steps=0;
 for(uint32_t t=0;t<4000;t+=DISPLAY_MS){View w=v;w.now=v.now+t;AnimFrame f=chooseAnim(quantised(w));assert(f.anim==ANIM_STRUM);if(last>=0){assert(f.frame==(last+1)%VEX_ANIMS[ANIM_STRUM].steps);steps++;}last=f.frame;}
 assert(steps==4000/int(DISPLAY_MS)-1);
 // Idle breathes through its manifest sequence (a ping-pong over the drawn frames) and blinks as a one-shot
 // inside each four-second window. The step index is what the simulator and the bench link report.
 v.progress.active=false;v.progress.running=false;
 assert(chooseAnim(v).anim==ANIM_IDLE);
 {const auto& a=VEX_ANIMS[ANIM_IDLE];assert(a.steps>=a.frames);int seen[16]={0};
  uint32_t base=v.now-v.now%(a.frame_ms*a.steps)+1000;   // clear of the blink window at each 4 s boundary
  for(int s=0;s<a.steps;s++){View w=v;w.now=base+s*a.frame_ms;AnimFrame f=chooseAnim(w);assert(f.anim==ANIM_IDLE&&f.frame==int(w.now/a.frame_ms%a.steps));int sprite=spriteIndex(f);assert(sprite>=a.first&&sprite<a.first+a.frames);seen[sprite-a.first]++;}
  for(int f=0;f<a.frames;f++)assert(seen[f]>0);   // every drawn frame is used; the ping-pong revisits the middle one
  // Breath budget: each idle step may redraw at most 30 % of the panel (the wings and collar, not the whole bat).
  static DesktopCanvas f0,f1;Band out[FLUSH_MAX_BANDS];
  for(int s=0;s<a.steps;s++){View w0=v,w1=v;w0.now=base+s*a.frame_ms;w1.now=w0.now+a.frame_ms;drawScreen(f0,w0);drawScreen(f1,w1);
   int n=bands(f0.pixels.data(),f1.pixels.data(),out);assert(n>=1);assert(bandBytes(out,n)/(FRAME_W*2)<=FRAME_H*30/100);}}
 View b=v;b.now=v.now+(UI_BLINK_EVERY_MS-v.now%UI_BLINK_EVERY_MS)%UI_BLINK_EVERY_MS;
 assert(chooseAnim(b).anim==ANIM_BLINK&&chooseAnim(b).frame==0);b.now+=VEX_ANIMS[ANIM_BLINK].frame_ms*VEX_ANIMS[ANIM_BLINK].steps;assert(chooseAnim(b).anim==ANIM_IDLE);
 // Time inside the current cadence window is pulled back to its start; "long ago" sentinels stay long ago.
 View q=v;q.now=101150;q.fragment_at=101140;q=quantised(q);assert(q.now==101100&&q.fragment_at==101100&&q.now-q.levelup_at>=UI_CELEBRATE_MS);
 // Band diff: first frame is sent whole by the adapter; an unchanged screen produces no bands; the
 // second blink frame touches only the eye rows; a new caption adds a band near the bottom only.
 static DesktopCanvas a,c;drawScreen(a,v);drawScreen(c,v);Band out[FLUSH_MAX_BANDS];
 assert(bands(a.pixels.data(),c.pixels.data(),out)==0);
 b.now-=VEX_ANIMS[ANIM_BLINK].frame_ms*VEX_ANIMS[ANIM_BLINK].steps;assert(chooseAnim(b).anim==ANIM_BLINK);
 drawScreen(a,b);b.now+=DISPLAY_MS;drawScreen(c,b);int n=bands(a.pixels.data(),c.pixels.data(),out);assert(n>=1);uint32_t rows=bandBytes(out,n)/(FRAME_W*2);assert(rows>0&&rows<FRAME_H*15/100);
 drawScreen(a,v);
 View tick=v;tick.notice="Saved - safe to unplug";tick.notice_at=v.now;drawScreen(c,tick);n=bands(a.pixels.data(),c.pixels.data(),out);assert(n>=1&&out[0].y>=180);
 // Cap: many scattered rows collapse into FLUSH_MAX_BANDS bands that still cover every change.
 static uint16_t p0[FRAME_W*FRAME_H],p1[FRAME_W*FRAME_H];memset(p0,0,sizeof p0);memset(p1,0,sizeof p1);
 for(int y=0;y<FRAME_H;y+=20){p1[y*FRAME_W]=1;}
 n=bands(p0,p1,out);assert(n==FLUSH_MAX_BANDS);
 assert(out[n-1].y+out[n-1].h==FRAME_H-19);for(int i=1;i<n;i++)assert(out[i].y>=out[i-1].y+out[i-1].h);
 // Bench test pattern (serial `t`): deterministic in `now`; every 16-row stripe differs from a blank panel,
 // so a dropped or shifted band is visible; one cadence later only the counter and marker rows change.
 {static DesktopCanvas t0,t1;drawTestPattern(t0,123456);drawTestPattern(t1,123456);assert(bands(t0.pixels.data(),t1.pixels.data(),out)==0);
  for(int y=0;y<FRAME_H;y+=16){bool dirty=false;for(int r=y;r<y+16&&r<FRAME_H&&!dirty;r++)for(int x=0;x<FRAME_W&&!dirty;x++)dirty=t0.pixels[r*FRAME_W+x]!=UI_BG;assert(dirty);}
  drawTestPattern(t1,123456+DISPLAY_MS);n=bands(t0.pixels.data(),t1.pixels.data(),out);assert(n>=1);for(int i=0;i<n;i++)assert(out[i].y>=140&&out[i].y+out[i].h<=190);}
}
#include "../firmware/VEX_Pocket_Rival/link.h"
// Bench link: band and state packets round-trip through the host parser, corruption is rejected, the parser
// resynchronises past interleaved text, and a sample stream is written for tests/link_test.py (Python decoder).
struct ByteSink{std::vector<uint8_t> bytes;void write(const uint8_t* p,size_t n){bytes.insert(bytes.end(),p,p+n);}};
static void linkTests(){
 using namespace vex;
 struct MemStore:Store{uint8_t m[128]={};uint8_t read(int a)override{return m[a];}void update(int a,uint8_t v)override{m[a]=v;}} store;
 App app(store,4);app.begin(101000,true);app.progress.calibrated=true;app.progress.gate=.004f;app.setRunning(true);app.progress.active=true;
 for(int i=0;i<30;i++){app.observe(.05f,.2f,app.now);app.tick(app.now+10);}
 LinkTiming timing;timing.tx_bytes=31200;timing.flush_us=4321;timing.flush_max_us=9876;timing.yin_us=1500;timing.yin_max_us=2200;timing.loop_max_us=12000;timing.cpu=12.5f;timing.cadence_mul=1;
 LinkState s=linkState(app,timing);assert(s.stage==4&&(s.flags&LINK_RUNNING)&&(s.flags&LINK_ACTIVE)&&(s.flags&LINK_CALIBRATED)&&s.anim==ANIM_STRUM&&s.flush_us==4321&&s.now_ms==app.now);
 static DesktopCanvas canvas;drawScreen(canvas,app.view());
 ByteSink out;const char* text="{\"stage\":4,\"tx_bytes\":31200}\n";out.write((const uint8_t*)text,strlen(text));
 linkBand(out,36,100,canvas.pixels.data()+36*FRAME_W);linkState(out,s);linkBand(out,200,8,canvas.pixels.data()+200*FRAME_W);
 // Parse: skip text byte by byte, then each packet decodes with the right geometry and payload.
 size_t at=0;int bandsSeen=0,statesSeen=0;
 while(at<out.bytes.size()){LinkPacket p;long used=linkParse(out.bytes.data()+at,out.bytes.size()-at,p);if(used<=0){at++;continue;}
  if(p.type==LINK_BAND){assert(p.length==size_t(p.b)*FRAME_W*2);assert(memcmp(p.payload,canvas.pixels.data()+size_t(p.a)*FRAME_W,p.length)==0);bandsSeen++;}
  else{assert(p.length==sizeof(LinkState));LinkState back;memcpy(&back,p.payload,sizeof back);assert(back.flush_us==4321&&back.loop_max_us==12000&&back.anim==s.anim&&back.frame==s.frame);statesSeen++;}
  at+=size_t(used);}
 assert(bandsSeen==2&&statesSeen==1);
 // A flipped payload bit fails the CRC; a truncated stream asks for more bytes rather than guessing.
 std::vector<uint8_t> bad(out.bytes.begin()+strlen(text),out.bytes.end());bad[LINK_HEADER+5]^=1;LinkPacket p;assert(linkParse(bad.data(),bad.size(),p)==-1);
 assert(linkParse(out.bytes.data()+strlen(text),LINK_HEADER+10,p)==0);
 FILE* f=fopen("build/link_sample.bin","wb");if(f){fwrite(out.bytes.data(),1,out.bytes.size(),f);fclose(f);}
}
int main(){
 appTests();analyzerTests();displayTests();linkTests();
 vex::Progress p;p.tick(100,.1,.2,true);assert(p.active_ms==0);p.calibrated=true;p.running=true;
 p.tick(100,.1,.2,true);assert(p.active_ms==100&&p.hold_ms==vex::ACTIVITY_HOLD_MS);
 // Silence holds up to ACTIVITY_HOLD_MS (including partial final ticks), then stops.
 for(int i=0;i<4;i++)p.tick(100,0,0,true);
 assert(p.active&&p.hold_ms==100&&p.active_ms==500);
 p.tick(70,0,0,true);assert(p.active&&p.hold_ms==30&&p.active_ms==570);
 p.tick(100,0,0,true);assert(!p.active&&p.hold_ms==0&&p.active_ms==600);
 p.tick(100,0,0,true);assert(!p.active&&p.active_ms==600);
 // Clipping, >100 ms loop stalls (counted in credit_drops), stale and NaN cut an active hold immediately.
 p.tick(100,.1,.2,true);assert(p.hold_ms==vex::ACTIVITY_HOLD_MS&&p.active_ms==700);
 p.tick(100,.2,.99,true);assert(!p.active&&p.hold_ms==0&&p.active_ms==700);
 p.tick(100,.1,.2,true);p.tick(101,.2,.2,true);assert(!p.active&&p.hold_ms==0&&p.credit_drops==1&&p.active_ms==800);
 p.tick(100,.1,.2,true);p.tick(100,.2,.2,false);assert(!p.active&&p.hold_ms==0&&p.active_ms==900);
 p.tick(100,.1,.2,true);p.tick(100,NAN,.2,true);assert(!p.active&&p.hold_ms==0&&p.active_ms==1000);
 p.session_ms=p.goalMs()-100;p.tick(100,.2,.3,true);assert(p.fragments==1&&p.claimed);for(int i=0;i<20;i++)p.tick(100,.2,.3,true);assert(p.fragments==1);
 assert(!p.setGoal(1));p.running=false;assert(p.setGoal(1));assert(p.session_ms==0&&!p.claimed&&p.active_ms>0);assert(!p.setGoal(3));
 auto a=vex::pitch(440,.99f);assert(a.valid&&a.midi==69&&fabs(a.cents)<.001);assert(!vex::pitch(440,.2).valid);assert(!vex::pitch(0,.99).valid);assert(!vex::pitch(INFINITY,1).valid);auto e=vex::pitch(82.40689,.99);assert(e.valid&&e.midi==40&&fabs(e.cents)<.01);
 auto r=vex::record(p,7);assert(vex::valid(r));auto bad=r;bad.active_ms^=1;assert(!vex::valid(bad));
 for(size_t n=0;n<offsetof(vex::Record,checksum);n++){auto corrupt=r;((unsigned char*)&corrupt)[n]^=1;assert(!vex::valid(corrupt));}
 auto torn=r;torn.magic=0;assert(!vex::valid(torn));assert(vex::newer(0,0xffffffff));assert(!vex::newer(0xffffffff,0));
 vex::Progress restored;vex::restore(restored,r);assert(restored.active_ms==p.active_ms&&restored.fragments==p.fragments&&!restored.running&&!restored.calibrated);
 vex::Button b;assert(b.update(true,10)==0);assert(b.update(false,20)==0);assert(b.update(false,50)==0);assert(b.update(true,100)==0);assert(b.update(true,130)==0);assert(b.update(false,200)==0);assert(b.update(false,230)==1);
 assert(b.update(true,300)==0);assert(b.update(true,330)==0);assert(b.update(true,1530)==2);assert(b.update(false,1600)==0);assert(b.update(false,1630)==0);
 puts("PASS: gating, clipping, stale input, quest exactly-once, goals, pitch math, CRC corruption, record restore, sequence rollover, button debounce and hold, app calibration/notice/save, YIN analyzer on tones/harmonics/noise, level accumulation, tuner smoothing, cadence-locked animation, dirty-band flush, bench link framing.");
}
