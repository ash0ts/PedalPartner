/* VEX Pocket Rival R1, Teensy4.1 + Audio Adapter D/D2 + Adafruit6178.
 * USB-powered sensing companion; no audio through-path or output/mute relay.
 * Device behavior lives in app.h and is shared with the desktop simulator; this
 * sketch only adapts I2S audio, GPIO, EEPROM, the SPI display and USB serial.
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
#include "app.h"
#include "ui.h"
#include "analyzer.h"
static_assert(VEX_STAGE>=1&&VEX_STAGE<=4,"VEX_STAGE must be 1..4");
AudioInputI2S input;
AudioOutputI2S clockOutput;
#ifdef VEX_PITCH_PJRC
// Bench comparison path: PJRC analyzers instead of the shared analyzer.
AudioAnalyzeRMS rmsNode;
AudioAnalyzePeak peakNode;
AudioAnalyzeNoteFrequency noteNode;
AudioConnection c1(input,0,rmsNode,0),c2(input,0,peakNode,0),c3(input,0,noteNode,0);
#else
// The audio interrupt only hands each 128-sample block to Analyzer::push; YIN runs in loop().
vex::Analyzer analyzer; // about 20 KB of RAM1: two 2048-sample windows, float work and difference buffers
class VexCapture:public AudioStream{
 audio_block_t* queue[1];
public:
 VexCapture():AudioStream(1,queue){}
 void update() override{audio_block_t* block=receiveReadOnly(0);if(!block)return;analyzer.push(block->data,AUDIO_BLOCK_SAMPLES);release(block);}
} capture;
AudioConnection c1(input,0,capture,0);
uint32_t analysisMicros=0,analysisMaxMicros=0;
#endif
AudioControlSGTL5000 codec;
Adafruit_GC9A01A lcd(TFT_CS,TFT_DC,TFT_RST);
GFXcanvas16 frame(240,240);
struct EepromStore:vex::Store{uint8_t read(int a)override{return EEPROM.read(a);}void update(int a,uint8_t v)override{EEPROM.update(a,v);}} eeprom;
vex::App app(eeprom,VEX_STAGE);
float rms=0,peak=0;
uint32_t drawAt=0,printAt=0;
void setup(){
 Serial.begin(115200);pinMode(PIN_LED,OUTPUT);const int controlPins[]={PIN_MODE,PIN_QUEST,PIN_FOOT};for(int p:controlPins)pinMode(p,INPUT_PULLUP);
 pinMode(6,OUTPUT);digitalWrite(6,HIGH);pinMode(10,OUTPUT);digitalWrite(10,HIGH);
 if(VEX_STAGE==1){pinMode(13,OUTPUT);return;}
 lcd.begin(TFT_HZ);lcd.setRotation(0);lcd.setTextWrap(false);
 bool codecOK=false;
 if(VEX_STAGE>=3){
  AudioMemory(80);codecOK=codec.enable();if(codecOK){codec.inputSelect(AUDIO_INPUT_LINEIN);codec.lineInLevel(INPUT_LEVEL);codec.muteHeadphone();codec.muteLineout();}
#ifdef VEX_PITCH_PJRC
  noteNode.begin(.15f);
#endif
 }
 app.begin(millis(),codecOK);
}
void loop(){
 uint32_t now=millis();
 if(VEX_STAGE==1){digitalWrite(13,(now/500)%2);if(now-printAt>=1000){printAt=now;Serial.println("VEX USB OK");}return;}
 if(VEX_STAGE>=3){
#ifdef VEX_PITCH_PJRC
  bool fresh=false;if(rmsNode.available()){rms=rmsNode.read();fresh=true;}if(peakNode.available()){peak=peakNode.read();fresh=true;}
  if(fresh)app.observe(rms,peak,now);
  if(noteNode.available())app.observePitch(noteNode.read(),noteNode.probability(),now);
#else
  if(analyzer.takeLevel(rms,peak))app.observe(rms,peak,now);
  uint32_t started=micros();
  if(analyzer.analyze()){analysisMicros=micros()-started;if(analysisMicros>analysisMaxMicros)analysisMaxMicros=analysisMicros;app.observePitch(analyzer.hz,analyzer.probability,millis());}
#endif
 }
 app.controls(digitalRead(PIN_MODE)==LOW,digitalRead(PIN_QUEST)==LOW,digitalRead(PIN_FOOT)==LOW,now);
 if(VEX_STAGE==4&&Serial.available())app.command(Serial.read(),now);
 app.tick(now);
 digitalWrite(PIN_LED,app.led());
 if(now-drawAt>=200){drawAt=now;vex::drawScreen(frame,app.view());lcd.drawRGBBitmap(0,0,frame.getBuffer(),240,240);}
 if(VEX_STAGE>=3&&now-printAt>=1000){
  printAt=now;const auto& p=app.progress;
  Serial.printf("stage=%d codec=%d rms=%.5f peak=%.3f gate=%.5f active=%d running=%d session_s=%lu total_s=%lu fragments=%lu cpu=%.1f memory=%u",VEX_STAGE,app.codec_ok,app.rms,app.peak,p.gate,p.active,p.running,(unsigned long)(p.session_ms/1000),(unsigned long)(p.active_ms/1000),(unsigned long)p.fragments,AudioProcessorUsageMax(),AudioMemoryUsageMax());
#ifdef VEX_PITCH_PJRC
  Serial.printf(" pitch=pjrc midi=%d cents=%.1f valid=%d\n",app.pitchNow.midi,app.pitchNow.cents,app.pitchNow.valid);
#else
  Serial.printf(" pitch=yin hz=%.2f prob=%.2f yin_us=%lu yin_max_us=%lu\n",analyzer.hz,analyzer.probability,(unsigned long)analysisMicros,(unsigned long)analysisMaxMicros);
#endif
 }
}
