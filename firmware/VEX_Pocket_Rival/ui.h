#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "core.h"
#include "drill_view.h"
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
constexpr uint32_t UI_CELEBRATE_MS=3000,UI_SUMMARY_MS=4000,UI_SLEEP_MS=30000,UI_TUNER_FADE_MS=1500,UI_BLINK_EVERY_MS=4000,UI_PULSE_MS=250;
struct View {
 Progress progress;
 Pitch pitch;
 int stage=4;
 Activity activity=ACT_PET;
 bool codec_ok=true,calibrating=false,pitch_fresh=false,uncertain=false;
 bool tuning()const{return activity==ACT_TUNER;}
 Pitch last_pitch;uint32_t last_pitch_at=0;
 bool mode_pressed=false,quest_pressed=false,foot_pressed=false;
 float rms=0,peak=0;
 uint32_t now=0,active_at=0,notice_at=0;
 uint32_t fragment_at=0,levelup_at=0,summary_at=0,summary_ms=0,summary_xp=0;
 DrillView drill;
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
// `frame` is a step into the animation's play order (manifest `sequence`); spriteIndex maps it to a drawn frame.
struct AnimFrame{int anim,frame;};
inline int loopFrame(int anim,uint32_t now){const auto& a=VEX_ANIMS[anim];return int((now/a.frame_ms)%a.steps);}
// One-shot step since `since`; -1 once the animation has finished.
inline int shotFrame(int anim,uint32_t since){const auto& a=VEX_ANIMS[anim];uint32_t f=since/a.frame_ms;return f<a.steps?int(f):-1;}
inline AnimFrame chooseAnim(const View& v){
 const auto& p=v.progress;
 if(v.now-v.levelup_at<UI_CELEBRATE_MS)return{ANIM_LEVELUP,loopFrame(ANIM_LEVELUP,v.now-v.levelup_at)};
 if(v.now-v.fragment_at<UI_CELEBRATE_MS)return{ANIM_CHEER,loopFrame(ANIM_CHEER,v.now-v.fragment_at)};
 if(v.activity==ACT_DRILL){
  if(v.drill.new_best)return{ANIM_CHEER,loopFrame(ANIM_CHEER,v.now)};
  if(p.running)return{ANIM_LISTEN,loopFrame(ANIM_LISTEN,v.now)};
 }
 if(v.activity==ACT_TUNER)return{ANIM_LISTEN,loopFrame(ANIM_LISTEN,v.now)};
 if(p.active)return{ANIM_STRUM,loopFrame(ANIM_STRUM,v.now)};
 if(v.now-v.active_at>UI_SLEEP_MS&&!p.running)return{ANIM_SLEEP,loopFrame(ANIM_SLEEP,v.now)};
 int blink=shotFrame(ANIM_BLINK,v.now%UI_BLINK_EVERY_MS);
 if(blink>=0)return{ANIM_BLINK,blink};
 return{ANIM_IDLE,loopFrame(ANIM_IDLE,v.now)};
}
inline int spriteIndex(AnimFrame f){const auto& a=VEX_ANIMS[f.anim];return a.first+VEX_ANIM_STEPS[a.first_step+f.frame];}
// Lead-square pulse while time is being earned: gold > fade > ink > fade, one step per UI_PULSE_MS, so the
// brightness eases instead of flipping. All theme colours; the square is 4x4 so each step is one tiny band.
inline uint16_t pulseColor(uint32_t now){static const uint16_t steps[4]={UI_GOLD,UI_FADE,UI_INK,UI_FADE};return steps[(now/UI_PULSE_MS)%4];}
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
 int listener=spriteIndex({ANIM_LISTEN,loopFrame(ANIM_LISTEN,v.now)});   // ear-tilt loop while tuning
 if(clipping){
  pet(c,listener,88,58,2);
  title(c,"CLIP - TURN DOWN",131,1,UI_RED);
  c.drawFastHLine(40,166,160,UI_DIM);c.drawFastVLine(120,158,17,UI_DIM);
  centered(c,"lower the guitar volume",184,1,UI_RED);
 }else if(sounding){
  tunerReading(c,v.pitch,true);
 }else if(stale){
  tunerReading(c,v.last_pitch,false);
 }else{
  pet(c,listener,88,58,2);
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
 // The leading square pulses through pulseColor while time is being earned.
 ring(c,filled,p.claimed||p.running?UI_GOLD:UI_PLUM,UI_DIM,p.running&&!p.claimed,pulseColor(v.now));
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
template<class Canvas> void drawDrill(Canvas& c,const View& v){
 const auto& d=v.drill;const auto& p=v.progress;
 ring(c,d.ring_lit,d.new_best?UI_GOLD:UI_RED);
 title(c,d.title,24,1,UI_GOLD);
 centered(c,d.subtitle,41,1,UI_FADE);
 // Small pixel-bat companion perched above the drill stage (uses listen / cheer / idle, no new art).
 AnimFrame af=chooseAnim(v);
 pet(c,spriteIndex(af),104,52,1);

 if(d.viz==VIZ_TIMING){
  // Centre beat line at x=120, +-30 ms pass brackets at x=84..156, and early/late note dots.
  c.drawFastHLine(48,112,144,UI_DIM);
  c.drawFastVLine(84,104,17,UI_FADE);
  c.drawFastVLine(156,104,17,UI_FADE);
  c.drawFastVLine(120,96,33,UI_GOLD);
  text(c,"EARLY",48,92,1,UI_FADE);
  text(c,"LATE",168,92,1,UI_FADE);
  for(uint8_t i=0;i<d.dot_count;i++){
   int ms=d.dots[i];
   int px=120+(ms*60)/50;
   if(px<48)px=48;
   if(px>192)px=192;
   bool ok=ms>=-d.pass_ms&&ms<=d.pass_ms;
   bool newest=(i+1==d.dot_count);
   int py=108+((i%3)-1)*4;
   c.fillCircle(px,py,newest?4:2,ok?(newest?UI_GOLD:UI_INK):UI_RED);
  }
  title(c,d.mid,136,1,UI_INK);
 }else if(d.viz==VIZ_PROMPT){
  centered(c,d.big,92,3,d.wrong_flash?UI_RED:UI_INK);
  title(c,d.mid,124,1,UI_GOLD);
  if(p.calibrated&&p.active)c.fillRect(118,144,4,4,pulseColor(v.now));
 }else if(d.viz==VIZ_BEND){
  // Target horizontal bar at y=100, start line at y=132, live pitch bar + last-8 bends along bottom.
  c.drawFastHLine(52,132,136,UI_DIM);
  c.drawFastHLine(52,100,136,UI_GOLD);
  int span=d.target_cents>0?d.target_cents:100;
  int dy=(int(d.live_cents)*32)/span;
  if(dy<-8)dy=-8;
  if(dy>44)dy=44;
  int py=132-dy;
  uint16_t lc=(d.live_cents>=span-12&&d.live_cents<=span+12)?UI_GOLD:UI_INK;
  c.fillRect(68,py-2,104,4,lc);
  for(uint8_t i=0;i<d.bend_count&&i<8;i++){
   int err=d.bend_hist[i];
   uint16_t hc=(err>=-12&&err<=12)?UI_GOLD:UI_RED;
   int hy=100-(err*16)/50;
   if(hy<88)hy=88;
   if(hy>134)hy=134;
   c.fillCircle(64+i*16,hy,3,hc);
  }
  title(c,d.mid,140,1,UI_INK);
 }else if(d.viz==VIZ_SCALE){
  // 12 chromatic degree dots in two rows of 6; in-scale degrees outlined/lit, played degree highlighted.
  for(int i=0;i<12;i++){
   int col=i%6,row=i/6;
   int cx=70+col*20,cy=98+row*18;
   bool inScale=((d.scale_mask>>i)&1)!=0;
   bool activeDeg=(d.last_rel_pc==i);
   uint16_t dc=activeDeg?(d.last_in_scale?UI_GOLD:UI_RED):(inScale?UI_INK:UI_DIM);
   c.fillCircle(cx,cy,activeDeg?5:(inScale?3:2),dc);
  }
  title(c,d.mid,136,1,d.wrong_flash?UI_RED:UI_INK);
 }else if(d.viz==VIZ_BARS){
  // Median reference line at y=114 and up to 12 per-note dB bars.
  c.drawFastHLine(48,114,144,UI_GOLD);
  for(uint8_t i=0;i<d.bar_count&&i<12;i++){
   int bx=52+i*11;
   int h2=d.db_bars[i]; // half-dB units relative to median
   int bh=h2;
   if(bh<-18)bh=-18;
   if(bh>18)bh=18;
   bool acc=((d.accent_mask>>i)&1)!=0;
   uint16_t bc=acc?UI_GOLD:UI_INK;
   if(bh>=0)c.fillRect(bx,114-bh,8,bh+2,bc);
   else c.fillRect(bx,114,8,(-bh)+2,UI_DIM);
  }
  title(c,d.mid,138,1,UI_INK);
 }

 if(d.new_best){
  c.fillRect(44,166,152,15,UI_GOLD);
  centered(c,"NEW PERSONAL BEST",170,1,UI_BG);
 }else if(!p.calibrated){
  centered(c,"HOLD QUEST: CALIBRATE",170,1,UI_GOLD);
 }else if(!p.running){
  centered(c,"QUEST: PRESET  FOOT: GO",170,1,UI_GOLD);
 }else{
  centered(c,d.detail,170,1,d.wrong_flash?UI_RED:UI_INK);
 }
 if(v.now-v.notice_at<2200&&p.calibrated&&!d.new_best){
  centered(c,v.notice,188,1,UI_FADE);
 }
}
template<class Canvas> void drawActivity(Canvas& c,const View& v){
 switch(v.activity){
  case ACT_TUNER:drawTuner(c,v);break;
  case ACT_DRILL:drawDrill(c,v);break;
  case ACT_PET:default:drawPet(c,v);break;
 }
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
 }else drawActivity(c,v);
 bool summary=v.now-v.summary_at<UI_SUMMARY_MS&&!p.running&&p.calibrated;
 if(v.stage==4&&v.activity==ACT_PET&&!summary&&(v.now-v.notice_at<3000||!p.calibrated)){
  c.fillRect(20,193,200,11,UI_BG);
  centered(c,v.calibrating?"MUTE STRINGS...":v.notice,195,1,UI_GOLD);
 }
}
// Bench test pattern (serial `t`), independent of App state, for the first minutes with a panel on the
// desk: a 1 px ink border and four differently coloured corner marks (gold top-left, red top-right, plum
// bottom-left, ink bottom-right) expose a mirrored or rotated panel; "TOP" labels the orientation; seven
// theme-colour bars show colour order and bit depth; a ruler tick at every 16th row on both edges makes a
// missing or shifted band visible; the frame counter and a marker that walks one cell per display cadence
// show whether the flush is keeping up. Theme colours only; deterministic in `now`.
template<class Canvas> void drawTestPattern(Canvas& c,uint32_t now){
 now=quantise(now);char s[32];
 c.fillScreen(UI_BG);
 c.drawFastHLine(0,0,240,UI_INK);c.drawFastHLine(0,239,240,UI_INK);c.drawFastVLine(0,0,240,UI_INK);c.drawFastVLine(239,0,240,UI_INK);
 c.fillRect(2,2,10,10,UI_GOLD);c.fillRect(228,2,10,10,UI_RED);c.fillRect(2,228,10,10,UI_PLUM);c.fillRect(228,228,10,10,UI_INK);
 for(int y=16;y<240;y+=16){c.drawFastHLine(2,y,6,UI_FADE);c.drawFastHLine(232,y,6,UI_FADE);}
 title(c,"TOP",18,1,UI_GOLD);
 static const uint16_t bars[7]={UI_INK,UI_GOLD,UI_RED,UI_PLUM,UI_DIM,UI_FADE,UI_BG};
 c.fillRect(21,55,198,50,UI_FADE);for(int i=0;i<7;i++)c.fillRect(22+i*28,56,28,48,bars[i]);
 c.drawFastHLine(100,120,40,UI_INK);c.drawFastVLine(120,100,40,UI_INK);
 snprintf(s,sizeof(s),"FRAME %lu",(unsigned long)(now/DISPLAY_MS%10000));title(c,s,146);
 c.drawFastHLine(24,186,192,UI_DIM);c.fillRect(24+int(now/DISPLAY_MS%24)*8,176,8,8,UI_GOLD);
 centered(c,"SPI TEST PATTERN",200,1,UI_FADE);centered(c,"serial t returns to VEX",212,1,UI_FADE);
}
}
