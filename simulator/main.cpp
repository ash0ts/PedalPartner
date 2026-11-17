#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cstdlib>
#include "canvas.h"
#include "../firmware/VEX_Pocket_Rival/ui.h"

int main(int argc,char**argv){
 const char* frame=argc>1?argv[1]:"build/screen.ppm";
 const char* savepath=argc>2?argv[2]:"build/sim-progress.bin";
 vex::View view;DesktopCanvas canvas;vex::Record slots[2]{};uint32_t sequence=0;int saved=-1;
 auto load=[&](){
  std::ifstream in(savepath,std::ios::binary);if(in)in.read((char*)slots,sizeof(slots));
  bool a=vex::valid(slots[0]),b=vex::valid(slots[1]);
  view.progress=vex::Progress{};saved=-1;sequence=0;
  if(a||b){saved=b&&(!a||vex::newer(slots[1].sequence,slots[0].sequence))?1:0;vex::restore(view.progress,slots[saved]);sequence=slots[saved].sequence;}
 };
 load();std::string line;
 while(std::getline(std::cin,line)){
  std::istringstream in(line);std::string cmd;in>>cmd;view.notice_at=view.now;
  if(cmd=="calibrate"){view.progress.calibrated=true;view.progress.running=false;view.progress.gate=.004f;view.notice="Simulated quiet baseline";}
  else if(cmd=="foot"){int event=1;in>>event;if(event==2)view.progress.newSession();else if(!view.tuning&&view.progress.calibrated)view.progress.running=!view.progress.running;view.notice="Practice control";}
  else if(cmd=="mode"){view.tuning=!view.tuning;view.progress.running=false;view.progress.active=false;view.notice="Mode changed";}
  else if(cmd=="quest"){view.progress.setGoal((view.progress.goal_index+1)%3);view.notice="Goal selection";}
  else if(cmd=="stage"){int stage=4;in>>stage;if(stage>=2&&stage<=4)view.stage=stage;}
  else if(cmd=="step"){
   int duration=0,fresh=1;float hz=440,confidence=1;in>>duration>>view.rms>>view.peak>>hz>>confidence>>fresh;
   if(duration<0||duration>3600000||!std::isfinite(view.rms)||!std::isfinite(view.peak))return 2;
   view.pitch=vex::pitch(hz,confidence);view.pitch_fresh=fresh;
   for(int elapsed=0;elapsed<duration;){uint32_t dt=std::min(10,duration-elapsed);elapsed+=dt;view.now+=dt;
    view.progress.tick(dt,view.rms,view.peak,fresh&&!view.tuning);if(view.progress.active)view.active_at=view.now;
   }
  }else if(cmd=="save"){
   int target=saved==0?1:0;slots[target]=vex::record(view.progress,++sequence);saved=target;
   std::ofstream out(savepath,std::ios::binary);out.write((const char*)slots,sizeof(slots));view.notice="Saved in simulator";
  }else if(cmd=="reboot"){load();view.tuning=false;view.notice="Hold QUEST: calibrate";}
  else if(cmd=="corrupt"){
   if(saved>=0){slots[saved].checksum^=1;std::ofstream out(savepath,std::ios::binary);out.write((const char*)slots,sizeof(slots));}view.notice="Latest save corrupted";
  }else if(cmd=="reset"){view=vex::View{};view.notice="New unsaved device";}
  else if(cmd!="state")return 3;
  vex::drawScreen(canvas,view);canvas.write(frame);
  auto&p=view.progress;
  std::cout<<"{\"active_ms\":"<<p.active_ms<<",\"session_ms\":"<<p.session_ms<<",\"xp\":"<<p.xp()<<",\"level\":"<<p.level()<<",\"fragments\":"<<p.fragments<<",\"goal_minutes\":"<<p.goalMs()/60000<<",\"running\":"<<(p.running?"true":"false")<<",\"calibrated\":"<<(p.calibrated?"true":"false")<<",\"active\":"<<(p.active?"true":"false")<<",\"tuning\":"<<(view.tuning?"true":"false")<<",\"now_ms\":"<<view.now<<"}"<<std::endl;
 }
}
