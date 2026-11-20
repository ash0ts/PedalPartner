// Desktop host for the shared firmware state machine. Reads one command per line, advances a
// millisecond clock through vex::App exactly as the sketch does, writes the frame and prints state.
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <memory>
#include <cstring>
#include <cmath>
#include <vector>
#include <algorithm>
#include "canvas.h"
#include "wav.h"
#include "../firmware/VEX_Pocket_Rival/app.h"
#include "../firmware/VEX_Pocket_Rival/analyzer.h"

// Two 64-byte slots persisted to a file. `cut` simulates power loss after N more byte writes.
struct FileStore:vex::Store{
 std::string path;uint8_t bytes[128]{};long cut=-1;bool powered=true,dirty=false;
 explicit FileStore(std::string p):path(std::move(p)){std::ifstream in(path,std::ios::binary);if(in)in.read((char*)bytes,sizeof bytes);}
 uint8_t read(int a)override{return a>=0&&a<128?bytes[a]:0xff;}
 void update(int a,uint8_t v)override{
  if(!powered||a<0||a>=128)return;
  if(cut==0){powered=false;return;}
  if(cut>0)cut--;
  if(bytes[a]!=v){bytes[a]=v;dirty=true;}
 }
 void flush(){if(!dirty)return;std::ofstream out(path,std::ios::binary);out.write((const char*)bytes,sizeof bytes);dirty=false;}
 void wipe(){std::memset(bytes,0,sizeof bytes);dirty=true;cut=-1;powered=true;flush();}
};

// Synthesise a guitar-like harmonic tone or repeated plucks at 44.1 kHz for `tone`.
static std::vector<int16_t> synthTone(uint32_t ms,float hz,float dbfs,float decay_ms,float gap_ms){
 constexpr double PI=3.14159265358979323846;
 size_t n=size_t(uint64_t(ms)*vex::Analyzer::RATE/1000);std::vector<int16_t> pcm(n,0);
 if(hz<=0||!n)return pcm;
 float amp=std::pow(10.f,dbfs/20.f),ring_ms=decay_ms>0?decay_ms:(gap_ms>0?200.f:0.f),period_ms=gap_ms>0?ring_ms+gap_ms:0.f;
 double phase=0,step=2.0*PI*hz/vex::Analyzer::RATE;
 for(size_t i=0;i<n;i++){
  float t_ms=float(i)*1000.f/vex::Analyzer::RATE,in_ms=period_ms>0?std::fmod(t_ms,period_ms):t_ms;
  if(gap_ms>0&&in_ms>=ring_ms){phase=0;continue;}
  float env=decay_ms>0?std::exp(-(gap_ms>0?1.5f:1.f)*in_ms/decay_ms):1.f;
  phase+=step;if(phase>2.0*PI)phase-=2.0*PI;
  float v=std::clamp(amp*env*(std::sin(float(phase))+.45f*std::sin(float(2*phase))+.15f*std::sin(float(3*phase)))/1.6f,-1.f,1.f);
  pcm[i]=int16_t(std::lround(v*32767.f));
 }
 return pcm;
}

struct Sim{
 FileStore store;int stage=4;std::unique_ptr<vex::App> app;std::unique_ptr<vex::Analyzer> analyzer=std::make_unique<vex::Analyzer>();DesktopCanvas canvas;
 bool raw_mode=false,raw_quest=false,raw_foot=false,test=false;   // `test on` renders ui.h drawTestPattern as the sketch does for serial `t`
 // Injected observations active during `audio`; absent during `wait` so samples go stale.
 bool inject=false;float rms=0,peak=0,hz=0,probability=0;
 double sample_ms=0;uint32_t analyses=0;
 explicit Sim(std::string save):store(std::move(save)){boot();}
 void boot(){store.powered=true;store.cut=-1;app=std::make_unique<vex::App>(store,stage);app->begin(0,true);analyzer=std::make_unique<vex::Analyzer>();sample_ms=0;}
 void poll(){// foreground audio handling, as the sketch's loop() does it
  float r,p;if(analyzer->takeLevel(r,p))app->observe(r,p,app->now);
  if(app->wantsPitch()){if(analyzer->analyze()){analyses++;app->observePitch(analyzer->hz,analyzer->probability,app->now);}}
  else analyzer->skip();
 }
 void step(){// one millisecond, in the sketch's loop order
  uint32_t now=app->now+1;
  if(inject){app->observe(rms,peak,now);if(hz>0)app->observePitch(hz,probability,now);}
  app->controls(raw_mode,raw_quest,raw_foot,now);
  app->tick(now);
 }
 void run(uint32_t ms){for(uint32_t i=0;i<ms;i++)step();}
 // Feed PCM through the shared analyzer in 128-sample blocks, advancing the clock at 44.1 kHz.
 void feed(const std::vector<int16_t>& pcm){
  sample_ms=app->now;
  for(size_t i=0;i<pcm.size();i+=vex::Analyzer::BLOCK){
   int16_t block[vex::Analyzer::BLOCK]{};size_t n=std::min<size_t>(vex::Analyzer::BLOCK,pcm.size()-i);std::copy(pcm.begin()+i,pcm.begin()+i+n,block);
   analyzer->push(block,vex::Analyzer::BLOCK);
   sample_ms+=1000.0*vex::Analyzer::BLOCK/vex::Analyzer::RATE;
   poll();while(app->now+1<=uint32_t(sample_ms))step();
  }
 }
 bool& control(const std::string& name){return name=="mode"?raw_mode:name=="quest"?raw_quest:raw_foot;}
 bool press(const std::string& name,uint32_t ms){
  if(name!="mode"&&name!="quest"&&name!="foot")return false;
  control(name)=true;run(ms);control(name)=false;run(30);return true;
 }
 void audio(uint32_t ms,float r,float p,float f,float q){inject=true;rms=r;peak=p;hz=f;probability=q;run(ms);inject=false;}
 std::string json()const{
  const auto& p=app->progress;std::ostringstream o;
  o<<"{\"active_ms\":"<<p.active_ms<<",\"session_ms\":"<<p.session_ms<<",\"xp\":"<<p.xp()<<",\"level\":"<<p.level()<<",\"fragments\":"<<p.fragments
   <<",\"goal_minutes\":"<<p.goalMs()/60000<<",\"running\":"<<(p.running?"true":"false")<<",\"calibrated\":"<<(p.calibrated?"true":"false")
   <<",\"active\":"<<(p.active?"true":"false")<<",\"tuning\":"<<(app->tuning?"true":"false")<<",\"calibrating\":"<<(app->calibrating?"true":"false")
   <<",\"gate\":"<<p.gate<<",\"rms\":"<<app->rms<<",\"env\":"<<app->env<<",\"peak\":"<<app->peak<<",\"hold_ms\":"<<p.hold_ms<<",\"credit_drops\":"<<p.credit_drops
   <<",\"stage\":"<<app->stage<<",\"notice\":\""<<app->notice<<"\""
   <<",\"saved_slot\":"<<app->saved_slot<<",\"sequence\":"<<app->sequence<<",\"power\":"<<(store.powered?"true":"false")
   <<",\"pitch_midi\":"<<app->pitchNow.midi<<",\"pitch_cents\":"<<app->pitchNow.cents<<",\"pitch_valid\":"<<(app->pitchNow.valid?"true":"false")<<",\"uncertain\":"<<(app->uncertain?"true":"false")
   <<",\"hz\":"<<analyzer->hz<<",\"probability\":"<<analyzer->probability<<",\"analyses\":"<<analyses<<",\"now_ms\":"<<app->now;
  // Animation the renderer picks for this instant (index into art/vex/manifest.json order, frame within it).
  vex::AnimFrame f=vex::chooseAnim(vex::quantised(app->view()));o<<",\"anim\":"<<f.anim<<",\"frame\":"<<f.frame<<",\"test\":"<<(test?"true":"false")<<"}";
  return o.str();
 }
};

int main(int argc,char** argv){
 const char* frame=argc>1?argv[1]:"build/screen.ppm";
 const char* savepath=argc>2?argv[2]:"build/sim-progress.bin";
 Sim sim(savepath);std::string line;
 while(std::getline(std::cin,line)){
  std::istringstream in(line);std::string cmd;in>>cmd;
  if(cmd.empty()||cmd[0]=='#')continue;
  if(cmd=="press"){std::string name;uint32_t ms=100;in>>name>>ms;if(ms>60000||!sim.press(name,ms))return 3;}
  else if(cmd=="audio"){uint32_t ms=0;float r=0,p=0,f=0,q=1;in>>ms>>r>>p>>f>>q;if(ms>3600000||!std::isfinite(r)||!std::isfinite(p))return 2;sim.audio(ms,r,p,f,q);}
  else if(cmd=="wait"){uint32_t ms=0;in>>ms;if(ms>3600000)return 2;sim.run(ms);}
  else if(cmd=="wav"||cmd=="raw"){
   std::string path;double start=0,length=-1;in>>path>>start>>length;std::vector<int16_t> pcm;std::string error;
   if(!(cmd=="wav"?readWav(path,pcm,error):readRaw(path,pcm,error))){std::cerr<<error<<std::endl;return 4;}
   size_t a=std::min(pcm.size(),size_t(start*44.1)),b=length<0?pcm.size():std::min(pcm.size(),a+size_t(length*44.1));
   sim.feed(std::vector<int16_t>(pcm.begin()+a,pcm.begin()+b));
  }
  else if(cmd=="tone"){
   uint32_t ms=0;float hz=0,dbfs=-20,decay=0,gap=0;in>>ms>>hz>>dbfs>>decay>>gap;
   if(ms>3600000||!std::isfinite(hz)||!std::isfinite(dbfs)||!std::isfinite(decay)||!std::isfinite(gap)||decay<0||gap<0)return 2;
   sim.feed(synthTone(ms,hz,dbfs,decay,gap));
  }
  else if(cmd=="serial"){char c=0;in>>c;sim.app->command(c,sim.app->now);}
  else if(cmd=="save"){sim.app->command('s',sim.app->now);}
  else if(cmd=="calibrate"){float r=.001f;in>>r;sim.app->command('c',sim.app->now);sim.audio(3100,r,r*2,0,0);}
  else if(cmd=="stage"){int s=4;in>>s;if(s<2||s>4)return 2;sim.stage=s;sim.boot();}
  else if(cmd=="powercut"){long n=0;in>>n;sim.store.cut=n;}
  else if(cmd=="reboot"){sim.store.flush();sim.boot();}
  else if(cmd=="corrupt"){if(sim.app->saved_slot>=0){sim.store.powered=true;sim.store.bytes[64*sim.app->saved_slot+40]^=1;sim.store.dirty=true;}}
  else if(cmd=="reset"){sim.store.wipe();sim.boot();}
  else if(cmd=="test"){std::string v;in>>v;if(v!="on"&&v!="off")return 2;sim.test=v=="on";}
  else if(cmd!="state")return 3;
  sim.store.flush();
  if(sim.test)vex::drawTestPattern(sim.canvas,sim.app->now);else vex::drawScreen(sim.canvas,sim.app->view());
  sim.canvas.write(frame);
  std::cout<<sim.json()<<std::endl;
 }
}
