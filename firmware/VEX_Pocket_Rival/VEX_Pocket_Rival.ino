/* VEX Pocket Rival R1, Teensy4.1 + Audio Adapter D/D2 + Adafruit6178.
 * USB-powered sensing companion; no audio through-path or output/mute relay.
 * Host core tests supplied; target compile and hardware acceptance still required.
 */
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include "config.h"
#include "core.h"
#include "ui.h"
static_assert(VEX_STAGE>=1&&VEX_STAGE<=4,"VEX_STAGE must be 1..4");
AudioInputI2S input;
AudioOutputI2S clockOutput;
AudioAnalyzeRMS rmsNode;
AudioAnalyzePeak peakNode;
AudioAnalyzeNoteFrequency noteNode;
AudioConnection c1(input,0,rmsNode,0),c2(input,0,peakNode,0),c3(input,0,noteNode,0);
AudioControlSGTL5000 codec;
Adafruit_GC9A01A lcd(TFT_CS,TFT_DC,TFT_RST);
GFXcanvas16 frame(240,240);
vex::Progress progress;
vex::Button bMode,bQuest,bFoot;
vex::Pitch pitchNow;
bool codecOK=false,tuning=false,calibrating=false;
float rms=0,peak=0,noise[300];
uint16_t noiseCount=0;
uint32_t rmsAt=0,peakAt=0,pitchAt=0,tickAt=0,drawAt=0,printAt=0,saveAt=0,activeAt=0,calAt=0,sampleAt=0,noticeAt=0,sequence=0;
int savedSlot=-1;
uint32_t savedHash=0;
const char*notice="Hold QUEST: calibrate";
constexpr uint16_t BG=0x1925,INK=0xEF37,GOLD=0xCD46,RED=0x99C7;
void message(const char*s){notice=s;noticeAt=millis();}
void load(){vex::Record a{},b{};EEPROM.get(0,a);EEPROM.get(64,b);bool av=vex::valid(a),bv=vex::valid(b);if(av||bv){savedSlot=(bv&&(!av||vex::newer(b.sequence,a.sequence)))?1:0;auto&r=savedSlot?b:a;vex::restore(progress,r);sequence=r.sequence;savedHash=vex::record(progress,0).checksum;}}
void save(){
 auto fingerprint=vex::record(progress,0).checksum;
 if(fingerprint==savedHash){message("Already saved");return;}
 int target=savedSlot==0?1:0;int address=64*target;auto r=vex::record(progress,sequence+1);
 uint32_t invalid=0;EEPROM.put(address,invalid);
 const uint8_t*p=(const uint8_t*)&r;for(size_t i=4;i<sizeof(r);i++)EEPROM.update(address+i,p[i]);EEPROM.put(address,r.magic);
 vex::Record check{};EEPROM.get(address,check);
 if(vex::valid(check)){savedSlot=target;sequence=r.sequence;savedHash=fingerprint;saveAt=millis();message("Saved - safe to unplug");}else message("SAVE FAILED");
}
void calibrate(){if(!codecOK){message("Check audio board");return;}progress.running=false;progress.calibrated=false;progress.active=false;calibrating=true;noiseCount=0;calAt=millis();sampleAt=0;message("Mute strings 3 seconds");}
void finishCalibration(){
 calibrating=false;if(noiseCount<50){message("No audio samples");return;}
 for(uint16_t i=1;i<noiseCount;i++){float v=noise[i];int j=i-1;while(j>=0&&noise[j]>v){noise[j+1]=noise[j];j--;}noise[j+1]=v;}
 float floor=noise[noiseCount/2];if(!isfinite(floor)||floor>.08f){message("Too loud: try again");return;}
 progress.gate=fmaxf(.001f,floor*4);progress.calibrated=true;message("Ready: tap footswitch");
}
void draw(uint32_t now){
 vex::View v;v.progress=progress;v.pitch=pitchNow;v.stage=VEX_STAGE;v.tuning=tuning;v.codec_ok=codecOK;v.calibrating=calibrating;
 v.rms=rms;v.peak=peak;v.now=now;v.active_at=activeAt;v.notice_at=noticeAt;v.notice=notice;
 v.pitch_fresh=now-pitchAt<350&&now-rmsAt<100&&now-peakAt<100;
 v.mode_pressed=digitalRead(PIN_MODE)==LOW;v.quest_pressed=digitalRead(PIN_QUEST)==LOW;v.foot_pressed=digitalRead(PIN_FOOT)==LOW;
 vex::drawScreen(frame,v);lcd.drawRGBBitmap(0,0,frame.getBuffer(),240,240);
}
void setup(){
 Serial.begin(115200);pinMode(PIN_LED,OUTPUT);const int controlPins[]={PIN_MODE,PIN_QUEST,PIN_FOOT};for(int p:controlPins)pinMode(p,INPUT_PULLUP);
 pinMode(6,OUTPUT);digitalWrite(6,HIGH);pinMode(10,OUTPUT);digitalWrite(10,HIGH);
 if(VEX_STAGE==1){pinMode(13,OUTPUT);return;}
 lcd.begin(TFT_HZ);lcd.setRotation(0);lcd.setTextWrap(false);
 if(VEX_STAGE>=3){AudioMemory(80);codecOK=codec.enable();if(codecOK){codec.inputSelect(AUDIO_INPUT_LINEIN);codec.lineInLevel(INPUT_LEVEL);codec.muteHeadphone();codec.muteLineout();}noteNode.begin(.15f);}
 if(VEX_STAGE==4)load();tickAt=millis();saveAt=millis();activeAt=millis();message("Hold QUEST: calibrate");
}
void loop(){
 uint32_t now=millis();
 if(VEX_STAGE==1){digitalWrite(13,(now/500)%2);if(now-printAt>=1000){printAt=now;Serial.println("VEX USB OK");}return;}
 if(VEX_STAGE>=3){if(rmsNode.available()){rms=rmsNode.read();rmsAt=now;}if(peakNode.available()){peak=peakNode.read();peakAt=now;}if(noteNode.available()){pitchNow=vex::pitch(noteNode.read(),noteNode.probability());pitchAt=now;}}
 int m=bMode.update(digitalRead(PIN_MODE)==LOW,now),q=bQuest.update(digitalRead(PIN_QUEST)==LOW,now),f=bFoot.update(digitalRead(PIN_FOOT)==LOW,now);
 if(VEX_STAGE==4){
  if(m==1){tuning=!tuning;progress.running=false;progress.active=false;message(tuning?"G3S handles muting":"Tap footswitch to start");}
  if(m==2)save();
  if(q==1){if(progress.setGoal((progress.goal_index+1)%3))message("Goal set; session reset");else message("Pause to change goal");}
  if(q==2)calibrate();
  if(f==1&&!tuning&&!calibrating){if(progress.calibrated)progress.running=!progress.running;else message("Hold QUEST: calibrate");}
  if(f==2){progress.newSession();message("New session / paused");}
  if(Serial.available()){char c=Serial.read();if(c=='c')calibrate();if(c=='s')save();if(c=='p')progress.running=false;if(c=='r')progress.newSession();}
  if(calibrating&&now-sampleAt>=10&&now-rmsAt<50){if(noiseCount<300)noise[noiseCount++]=rms;sampleAt=now;}
  if(calibrating&&now-calAt>=3000)finishCalibration();
  if(now-tickAt>=10){uint32_t dt=now-tickAt;tickAt=now;progress.tick(dt,rms,peak,codecOK&&now-rmsAt<100&&now-peakAt<100&&!tuning&&!calibrating);if(progress.active)activeAt=now;}
  if(now-saveAt>=60000&&now-activeAt>=3000&&!calibrating){save();saveAt=now;}
 }
 digitalWrite(PIN_LED,(VEX_STAGE==2)?(digitalRead(PIN_FOOT)==LOW):progress.active);
 if(now-drawAt>=200){drawAt=now;draw(now);}
 if(VEX_STAGE>=3&&now-printAt>=1000){printAt=now;Serial.printf("stage=%d codec=%d rms=%.5f peak=%.3f gate=%.5f active=%d running=%d session_s=%lu total_s=%lu fragments=%lu cpu=%.1f memory=%u\n",VEX_STAGE,codecOK,rms,peak,progress.gate,progress.active,progress.running,(unsigned long)(progress.session_ms/1000),(unsigned long)(progress.active_ms/1000),(unsigned long)progress.fragments,AudioProcessorUsageMax(),AudioMemoryUsageMax());}
}
