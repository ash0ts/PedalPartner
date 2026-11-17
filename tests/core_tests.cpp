#include "../firmware/VEX_Pocket_Rival/core.h"
#include <cassert>
#include <cstdio>
#include <limits>
int main(){
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
 puts("PASS: gating, clipping, stale input, quest exactly-once, goals, pitch math, CRC corruption, record restore, sequence rollover, button debounce and hold.");
}
