#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "core.h"
#include "theme.h"
#include "sprites.h"
#include "anim.h"
#include "tinyfont.h"
#include "displayfont.h"

// Shared 240 x 240 renderer for the Teensy framebuffer and the desktop simulator. Only fillRect,
// fillScreen, drawFastHLine, drawFastVLine and fillCircle are required from the canvas. Everything
// here is deterministic in View fields so simulator golden frames stay stable. Colours, sprites,
// animation timing and fonts are generated from art/ (see docs/ART.md); drawScreen quantises
// View::now to DISPLAY_MS so both targets pick the same animation frame for the same instant.
namespace vex {
constexpr uint32_t UI_CELEBRATE_MS=3000,UI_SUMMARY_MS=4000,UI_SLEEP_MS=30000,UI_TUNER_FADE_MS=1500,UI_BLINK_EVERY_MS=4000,UI_PULSE_MS=500;
struct View {
 Progress progress;
 Pitch pitch;
 int stage=4;
 bool tuning=false,codec_ok=true,calibrating=false,pitch_fresh=false,uncertain=false;
 Pitch last_pitch;uint32_t last_pitch_at=0;
 bool mode_pressed=false,quest_pressed=false,foot_pressed=false;
 float rms=0,peak=0;
 uint32_t now=0,active_at=0,notice_at=0;
 uint32_t fragment_at=0,levelup_at=0,summary_at=0,summary_ms=0,summary_xp=0;
 const char* notice="Hold QUEST: calibrate";
};
// Tiny 5x7 font (captions). Glyphs are column bytes; advance is 6 px per scale step.
inline int tinyWidth(const char* s,int scale=1){return (int(strlen(s))*6-1)*scale;}
template<class Canvas> void text(Canvas& c,const char* s,int x,int y,int scale=1,uint16_t color=UI_INK){
 for(;*s;++s,x+=6*scale){
  unsigned ch=(unsigned char)*s;if(ch<32||ch>126)ch='?';
  for(int col=0;col<5;col++)for(int row=0;row<7;row++)
   if(VEX_FONT[ch-32][col]&(1<<row))c.fillRect(x+col*scale,y+row*scale,scale,scale,color);
 }
}
template<class Canvas> void centered(Canvas& c,const char* s,int y,int scale=1,uint16_t color=UI_INK){text(c,s,120-tinyWidth(s,scale)/2,y,scale,color);}
// Display 8x12 proportional font (titles, note names, banners): left-aligned column bitmaps with a
// per-glyph width table; one pixel of tracking between glyphs.
constexpr int DISPLAY_H=12;
inline unsigned displayGlyph(char ch){unsigned u=(unsigned char)ch;return (u<32||u>126)?'?'-32:u-32;}
inline int displayWidth(const char* s,int scale=1){int w=0;for(;*s;++s)w+=VEX_DISPLAY_FONT_WIDTH[displayGlyph(*s)]+1;return (w-1)*scale;}
template<class Canvas> void display(Canvas& c,const char* s,int x,int y,int scale=1,uint16_t color=UI_INK){
 for(;*s;++s){
  unsigned g=displayGlyph(*s);
  for(int col=0;col<8;col++)for(int row=0;row<DISPLAY_H;row++)
   if(VEX_DISPLAY_FONT[g][col]&(1<<row))c.fillRect(x+col*scale,y+row*scale,scale,scale,color);
  x+=(VEX_DISPLAY_FONT_WIDTH[g]+1)*scale;
 }
}
template<class Canvas> void title(Canvas& c,const char* s,int y,int scale=1,uint16_t color=UI_INK){display(c,s,120-displayWidth(s,scale)/2,y,scale,color);}
// One 32x32 sprite frame by index into VEX_SPRITES (see anim.h for the per-animation ranges).
template<class Canvas> void pet(Canvas& c,int sprite,int x=40,int y=36,int scale=5){
 if(sprite<0||sprite>=VEX_SPRITE_COUNT)sprite=0;
 for(int row=0;row<VEX_SPRITE_SIZE;row++)for(int col=0;col<VEX_SPRITE_SIZE;col++){
  uint8_t v=VEX_SPRITES[sprite][row*VEX_SPRITE_SIZE+col];
  if(v)c.fillRect(x+col*scale,y+row*scale,scale,scale,VEX_PALETTE[v]);
 }
}
// 112 * sin(2*pi*i/60) for the ring; cos is the same table a quarter turn on. No trigonometry at
// run time, so the target and the host produce identical pixels.
static const int8_t RING_SIN[60]={0,12,23,35,46,56,66,75,83,91,97,102,107,110,111,112,111,110,107,102,97,91,83,75,66,56,46,35,23,12,0,-12,-23,-35,-46,-56,-66,-75,-83,-91,-97,-102,-107,-110,-111,-112,-111,-110,-107,-102,-97,-91,-83,-75,-66,-56,-46,-35,-23,-12};
inline void ringPoint(int i,int& x,int& y){i%=60;x=120+RING_SIN[i];y=120-RING_SIN[(i+15)%60];}
// Progress ring at the display edge: dim 3 px ticks every five segments on the unfilled part, a
// continuous 4 px arc clockwise from the top for the filled part (four 4x4 squares per segment,
// linearly interpolated between the table points) and a brighter leading square when `lead` is set.
template<class Canvas> void ring(Canvas& c,int filled,uint16_t on,uint16_t off=UI_DIM,bool lead=false,uint16_t lead_color=UI_INK){
 if(filled<0){filled=0;}
 if(filled>60){filled=60;}
 for(int i=0;i<60;i+=5)if(i>=filled){int x,y;ringPoint(i,x,y);c.fillRect(x-1,y-1,3,3,off);}
 for(int i=0;i<filled;i++){
  int x0,y0,x1,y1;ringPoint(i,x0,y0);ringPoint(i+1,x1,y1);
  for(int k=0;k<4;k++)c.fillRect(x0+(x1-x0)*k/4-2,y0+(y1-y0)*k/4-2,4,4,on);
 }
 if(filled>0&&filled<60&&lead){int x,y;ringPoint(filled,x,y);c.fillRect(x-2,y-2,4,4,lead_color);}
}
// Animation selection. Priority: level-up > fragment cheer > tuner listen > active strum > sleep >
// blink (one-shot every UI_BLINK_EVERY_MS) > idle breathe. Frame timing comes from anim.h; `now`
// has already been quantised to DISPLAY_MS by drawScreen, and every frame_ms is a multiple of it,
// so loops advance exactly one step per cadence boundary and never repeat or skip a frame.
struct AnimFrame{int anim,frame;};
inline int loopFrame(int anim,uint32_t now){const auto& a=VEX_ANIMS[anim];return int((now/a.frame_ms)%a.frames);}
// One-shot frame since `since`; -1 once the animation has finished.
inline int shotFrame(int anim,uint32_t since){const auto& a=VEX_ANIMS[anim];uint32_t f=since/a.frame_ms;return f<a.frames?int(f):-1;}
inline AnimFrame chooseAnim(const View& v){
 const auto& p=v.progress;
 if(v.now-v.levelup_at<UI_CELEBRATE_MS)return{ANIM_LEVELUP,loopFrame(ANIM_LEVELUP,v.now-v.levelup_at)};
 if(v.now-v.fragment_at<UI_CELEBRATE_MS)return{ANIM_CHEER,loopFrame(ANIM_CHEER,v.now-v.fragment_at)};
 if(v.tuning)return{ANIM_LISTEN,0};
 if(p.active)return{ANIM_STRUM,loopFrame(ANIM_STRUM,v.now)};
 if(v.now-v.active_at>UI_SLEEP_MS&&!p.running)return{ANIM_SLEEP,loopFrame(ANIM_SLEEP,v.now)};
 int blink=shotFrame(ANIM_BLINK,v.now%UI_BLINK_EVERY_MS);
 if(blink>=0)return{ANIM_BLINK,blink};
 return{ANIM_IDLE,loopFrame(ANIM_IDLE,v.now)};
}
inline int spriteIndex(AnimFrame f){return VEX_ANIMS[f.anim].first+f.frame;}
inline uint32_t quantise(uint32_t now){return now-now%DISPLAY_MS;}
inline const char* stringName(int midi){
 // Nearest standard-tuning open string for a quick visual anchor; other notes show a dash.
 switch(midi){case 40:return "E2 string";case 45:return "A2 string";case 50:return "D3 string";case 55:return "G3 string";case 59:return "B3 string";case 64:return "E4 string";default:return "-";}
}
// Note name, octave digit, cents and the needle. `live` draws the confident reading; otherwise the same
// layout is drawn in dim ink so a decaying string fades instead of vanishing.
template<class Canvas> void tunerReading(Canvas& c,const Pitch& pitch,bool live){
 static const char* names[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};char s[32];
 float cents=pitch.cents;bool inTune=cents>-5&&cents<5;
 uint16_t ink=live?UI_INK:UI_FADE,accent=live?(inTune?UI_GOLD:UI_INK):UI_FADE,warn=live?UI_RED:UI_FADE;
 const char* name=names[pitch.midi%12];
 title(c,name,58,4,accent);
 snprintf(s,sizeof(s),"%d",pitch.midi/12-1);display(c,s,120+displayWidth(name,4)/2+6,94,1,live?UI_FADE:UI_DIM);
 snprintf(s,sizeof(s),"%+.0f cents",double(cents));title(c,s,129,1,live&&inTune?UI_GOLD:ink);
 // Needle: centre mark, +-50 cent scale, red needle outside the green zone, gold when in tune.
 c.drawFastHLine(40,166,160,ink);c.fillRect(112,160,16,13,live?UI_PLUM:UI_DIM);c.drawFastVLine(120,158,17,live?UI_GOLD:UI_DIM);
 for(int i=-50;i<=50;i+=25)c.drawFastVLine(120+int(i*1.6f),163,7,UI_DIM);
 if(cents>50){cents=50;}
 if(cents<-50){cents=-50;}
 c.fillCircle(120+int(cents*1.6f),166,4,live?(inTune?UI_GOLD:UI_RED):UI_FADE);
 if(!live)centered(c,"fading - play again",184,1,UI_FADE);
 else if(cents<=-5)centered(c,"FLAT  < tune up",184,1,warn);else if(cents>=5)centered(c,"SHARP  tune down >",184,1,warn);else centered(c,"IN TUNE",184,1,UI_GOLD);
 centered(c,stringName(pitch.midi),198,1,live?UI_FADE:UI_DIM);
}
// Tuner states, each with a label so colour never carries the meaning alone:
// clipping, sounding (note shown), uncertain (periodic candidate below confidence), stale (last note
// fading for UI_TUNER_FADE_MS), listening (signal above the gate, no note yet) and waiting (no signal).
template<class Canvas> void drawTuner(Canvas& c,const View& v){
 const auto& p=v.progress;
 title(c,"TUNE / A440",24);
 bool fresh=v.pitch_fresh,clipping=fresh&&v.peak>=.98f,signal=fresh&&v.rms>p.gate;
 bool sounding=v.pitch.valid&&signal&&!clipping;
 bool stale=!sounding&&!clipping&&v.last_pitch.valid&&v.now-v.last_pitch_at<UI_TUNER_FADE_MS;
 // Signal meter under the title: level relative to eight times the gate, gate mark at one eighth.
 float level=p.gate>0?v.rms/(p.gate*8):0;if(level>1){level=1;}if(level<0||!fresh){level=0;}
 c.fillRect(70,42,100,4,UI_DIM);c.fillRect(70,42,int(level*100),4,clipping?UI_RED:signal?UI_GOLD:UI_PLUM);c.fillRect(82,40,1,8,UI_INK);
 if(clipping){
  pet(c,VEX_ANIMS[ANIM_LISTEN].first,88,58,2);
  title(c,"CLIP - TURN DOWN",131,1,UI_RED);
  c.drawFastHLine(40,166,160,UI_DIM);c.drawFastVLine(120,158,17,UI_DIM);
  centered(c,"lower the guitar volume",184,1,UI_RED);
 }else if(sounding){
  tunerReading(c,v.pitch,true);
 }else if(stale){
  tunerReading(c,v.last_pitch,false);
 }else{
  pet(c,VEX_ANIMS[ANIM_LISTEN].first,88,58,2);
  bool uncertain=v.uncertain&&signal;
  title(c,uncertain?"Hmm...":signal?"Listening":"Play one note",131,1,UI_INK);
  c.drawFastHLine(40,166,160,UI_DIM);c.drawFastVLine(120,158,17,UI_DIM);
  centered(c,uncertain?"one string, let it ring":signal?"waiting for a steady note":"pluck a single open string",184,1,UI_FADE);
 }
 centered(c,"DISPLAY ONLY",214);
}
template<class Canvas> void drawPet(Canvas& c,const View& v){
 const auto& p=v.progress;char s[64];
 // Quest ring: filled ticks follow session progress toward the goal; gold while running, dim when paused.
 uint32_t goal=p.goalMs();int filled=int((p.session_ms>=goal?goal:p.session_ms)*60/goal);
 // The leading square pulses ink/gold every UI_PULSE_MS while time is being earned.
 ring(c,filled,p.claimed||p.running?UI_GOLD:UI_PLUM,UI_DIM,p.running&&!p.claimed,(v.now/UI_PULSE_MS)%2?UI_INK:UI_GOLD);
 title(c,"VEX",16,1,UI_GOLD);
 pet(c,spriteIndex(chooseAnim(v)));
 bool celebrate=v.now-v.fragment_at<UI_CELEBRATE_MS,levelup=v.now-v.levelup_at<UI_CELEBRATE_MS,summary=v.now-v.summary_at<UI_SUMMARY_MS&&!p.running;
 if(levelup){
  c.fillRect(36,166,168,22,UI_GOLD);snprintf(s,sizeof(s),"LEVEL %lu!",(unsigned long)p.level());title(c,s,171,1,UI_BG);
 }else if(celebrate){
  c.fillRect(36,166,168,22,UI_RED);title(c,"FRAGMENT!",171,1,UI_INK);
 }
 if(summary){
  c.fillRect(24,194,192,1,UI_PLUM);
  snprintf(s,sizeof(s),"SESSION %lu:%02lu  +%lu XP",(unsigned long)(v.summary_ms/60000),(unsigned long)(v.summary_ms/1000%60),(unsigned long)v.summary_xp);centered(c,s,199,1,UI_GOLD);
  snprintf(s,sizeof(s),"LV%lu  %lu XP total",(unsigned long)p.level(),(unsigned long)p.xp());centered(c,s,211);
  centered(c,p.claimed?"Fragment earned":"Tap foot to resume",222,1,UI_INK);
 }else{
  snprintf(s,sizeof(s),"LV%lu  %lu XP",(unsigned long)p.level(),(unsigned long)p.xp());centered(c,s,196);
  snprintf(s,sizeof(s),"%lu:%02lu / %lum",(unsigned long)(p.session_ms/60000),(unsigned long)(p.session_ms/1000%60),(unsigned long)(goal/60000));centered(c,s,209);
  centered(c,p.claimed?"FRAGMENT!":p.running?"LISTENING":"PAUSED",222,1,UI_GOLD);
 }
}
// Timestamps inside the current cadence window are pulled back to its start. App marks "long ago"
// with now-100000, which wraps below zero at boot; that stays untouched because it is outside the window.
inline void clampTo(uint32_t& t,uint32_t now){if(t>now&&t-now<DISPLAY_MS)t=now;}
// Time is quantised to the display cadence before anything is drawn. Event timestamps newer than the
// quantised instant are pulled back to it so a fragment earned 30 ms ago still celebrates this frame.
inline View quantised(const View& v){
 View q=v;q.now=quantise(v.now);
 clampTo(q.active_at,q.now);clampTo(q.notice_at,q.now);clampTo(q.fragment_at,q.now);clampTo(q.levelup_at,q.now);clampTo(q.summary_at,q.now);clampTo(q.last_pitch_at,q.now);
 return q;
}
template<class Canvas> void drawScreen(Canvas& c,const View& raw){
 const View v=quantised(raw);
 c.fillScreen(UI_BG);char s[64];const auto&p=v.progress;
 if(v.stage==2){
  title(c,"SCREEN / INPUT",28);pet(c,0,56,55,4);
  snprintf(s,sizeof(s),"M:%d Q:%d F:%d",v.mode_pressed,v.quest_pressed,v.foot_pressed);
  title(c,s,184);centered(c,"Press each control",208);
 }else if(v.stage==3){
  title(c,v.codec_ok?"AUDIO CHECK":"CODEC ERROR",38);
  snprintf(s,sizeof(s),"RMS %.4f",double(v.rms));title(c,s,84);
  snprintf(s,sizeof(s),"PEAK %.3f",double(v.peak));title(c,s,116);
  int width=int(v.peak*160);if(width<0)width=0;if(width>160)width=160;
  c.fillRect(40,151,160,9,UI_DIM);c.fillRect(40,151,width,9,v.peak>=.98?UI_RED:UI_GOLD);
  centered(c,v.peak>=.98?"CLIP - TURN DOWN":"Watch serial monitor",180,1,v.peak>=.98?UI_RED:UI_INK);
 }else if(v.tuning)drawTuner(c,v);
 else drawPet(c,v);
 bool summary=v.now-v.summary_at<UI_SUMMARY_MS&&!p.running&&p.calibrated;
 if(v.stage==4&&!v.tuning&&!summary&&(v.now-v.notice_at<3000||!p.calibrated)){
  c.fillRect(20,193,200,11,UI_BG);
  centered(c,v.calibrating?"MUTE STRINGS...":v.notice,195,1,UI_GOLD);
 }
}
}
