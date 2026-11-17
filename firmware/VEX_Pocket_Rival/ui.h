#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "core.h"
#include "sprites.h"
#include "tinyfont.h"

namespace vex {
constexpr uint16_t UI_BG=0x1925,UI_INK=0xEF37,UI_GOLD=0xCD46,UI_RED=0x99C7;
struct View {
 Progress progress;
 Pitch pitch;
 int stage=4;
 bool tuning=false,codec_ok=true,calibrating=false,pitch_fresh=false;
 bool mode_pressed=false,quest_pressed=false,foot_pressed=false;
 float rms=0,peak=0;
 uint32_t now=0,active_at=0,notice_at=0;
 const char* notice="Hold QUEST: calibrate";
};
template<class Canvas> void centered(Canvas& c,const char* s,int y,int scale=1,uint16_t color=UI_INK){
 int x=120-(int(strlen(s))*6-1)*scale/2;
 for(;*s;++s,x+=6*scale){
  unsigned ch=(unsigned char)*s;if(ch<32||ch>126)ch='?';
  for(int col=0;col<5;col++)for(int row=0;row<7;row++)
   if(VEX_FONT[ch-32][col]&(1<<row))c.fillRect(x+col*scale,y+row*scale,scale,scale,color);
 }
}
template<class Canvas> void pet(Canvas& c,int pose,int x=40,int y=35,int scale=5){
 for(int row=0;row<32;row++)for(int col=0;col<32;col++){
  uint8_t v=VEX_SPRITES[pose][row*32+col];
  if(v)c.fillRect(x+col*scale,y+row*scale,scale,scale,VEX_PALETTE[v]);
 }
}
template<class Canvas> void drawScreen(Canvas& c,const View& v){
 c.fillScreen(UI_BG);char s[64];const auto&p=v.progress;
 if(v.stage==2){
  centered(c,"SCREEN / INPUT",28,2);pet(c,0,56,55,4);
  snprintf(s,sizeof(s),"M:%d Q:%d F:%d",v.mode_pressed,v.quest_pressed,v.foot_pressed);
  centered(c,s,184,2);centered(c,"Press each control",208);
 }else if(v.stage==3){
  centered(c,v.codec_ok?"AUDIO CHECK":"CODEC ERROR",38,2);
  snprintf(s,sizeof(s),"RMS %.4f",double(v.rms));centered(c,s,84,2);
  snprintf(s,sizeof(s),"PEAK %.3f",double(v.peak));centered(c,s,116,2);
  int width=int(v.peak*160);if(width<0)width=0;if(width>160)width=160;
  c.fillRect(40,151,160,9,0x2949);c.fillRect(40,151,width,9,v.peak>=.98?UI_RED:UI_GOLD);
  centered(c,v.peak>=.98?"CLIP - TURN DOWN":"Watch serial monitor",180,1,v.peak>=.98?UI_RED:UI_INK);
 }else if(v.tuning){
  centered(c,"TUNE / A440",28,2);
  if(v.pitch.valid&&v.pitch_fresh&&v.rms>p.gate&&v.peak<.98){
   const char* names[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
   centered(c,names[v.pitch.midi%12],62,8,UI_GOLD);
   snprintf(s,sizeof(s),"%+.0f cents",double(v.pitch.cents));centered(c,s,139,2);
   c.drawFastHLine(40,173,160,UI_INK);c.drawFastVLine(120,166,15,UI_GOLD);
   float cents=v.pitch.cents;if(cents>50)cents=50;if(cents< -50)cents=-50;
   c.fillCircle(120+int(cents*1.6f),173,4,UI_RED);
  }else centered(c,"Play one note",99,2);
  centered(c,"DISPLAY ONLY",204);
 }else{
  centered(c,"VEX",18,2,UI_GOLD);pet(c,p.active?1:(v.now-v.active_at>30000?2:0));
  snprintf(s,sizeof(s),"LV%lu  %lu XP",(unsigned long)p.level(),(unsigned long)p.xp());centered(c,s,196);
  snprintf(s,sizeof(s),"%lu:%02lu / %lum",(unsigned long)(p.session_ms/60000),(unsigned long)(p.session_ms/1000%60),(unsigned long)(p.goalMs()/60000));centered(c,s,209);
  centered(c,p.claimed?"FRAGMENT!":p.running?"LISTENING":"PAUSED",222,1,UI_GOLD);
 }
 if(v.stage==4&&(v.now-v.notice_at<3000||!p.calibrated)){
  c.fillRect(20,193,200,11,UI_BG);
  centered(c,v.calibrating?"MUTE STRINGS...":v.notice,195,1,UI_GOLD);
 }
}
}
