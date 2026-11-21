/* VEX Pocket Rival R1, Teensy4.1 + Audio Adapter D/D2 + Adafruit6178.
 * USB-powered sensing companion; no audio through-path or output/mute relay.
 * Device behavior lives in app.h and is shared with the desktop simulator; this
 * sketch only adapts I2S audio, GPIO, EEPROM, the SPI display and USB serial.
 * Host core tests and Teensy target builds supplied; hardware acceptance still required.
 *
 * Bench serial (stages 2-4, one character each; stage 4 also keeps c s p r from app.h):
 *   f  force a full-frame flush every cadence      b  toggle the dirty-band diff (default on)
 *   1 / 2  redraw every 1x / 2x DISPLAY_MS          l  toggle the bench link (band + state packets)
 *   m q k  tap MODE / QUEST / FOOT (100 ms)         M Q K  hold MODE / QUEST / FOOT (1.3 s)
 *   - / +  panel SPI clock down / up (8 16 24 30 40 60 MHz; boots at TFT_HZ)
 *   g / G  codec line-in level down / up (0..15; boots at INPUT_LEVEL; stage 3-4 with a codec)
 *   t  toggle the display test pattern (ui.h drawTestPattern; full flush while on)
 *   ?  print the current switch settings
 * Telemetry is one JSON line per second (draw_us vs SPI flush_us, credit_drops, pitch gated by
 * App::wantsPitch); tools/web.py serve --device mirrors the panel from the link.
 * Every knob boots at its config.h value; nothing typed here is remembered across a reset.
 */
#include <Audio.h>
#define VEX_AUDIO_LOCK() AudioNoInterrupts()
#define VEX_AUDIO_UNLOCK() AudioInterrupts()
#include <Wire.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include "config.h"
#include "core.h"
#include "app.h"
#include "ui.h"
#include "flush.h"
#include "link.h"
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
#endif
AudioControlSGTL5000 codec;
Adafruit_GC9A01A lcd(TFT_CS,TFT_DC,TFT_RST);
GFXcanvas16 frame(240,240);
// Panel adapter: the only place the display library is called for pixels. A DMA-capable driver can
// replace this struct without touching the renderer, the band diff or the link.
struct Panel{
 void begin(){lcd.startWrite();}
 void writeBand(uint16_t y,uint16_t h,const uint16_t* rows){lcd.setAddrWindow(0,y,vex::FRAME_W,h);lcd.writePixels(const_cast<uint16_t*>(rows),size_t(h)*vex::FRAME_W);}
 void end(){lcd.endWrite();}
} panel;
// What the panel currently shows, kept in RAM2 so the 115 KB does not compete with audio in RAM1.
// Each cadence only the rows that differ from it are sent (flush.h); the first frame is sent whole.
DMAMEM uint16_t shadow[vex::FRAME_W*vex::FRAME_H];
bool shadowValid=false;
struct EepromStore:vex::Store{uint8_t read(int a)override{return EEPROM.read(a);}void update(int a,uint8_t v)override{EEPROM.update(a,v);}} eeprom;
vex::App app(eeprom,VEX_STAGE);
float rms=0,peak=0;
uint32_t drawAt=0,printAt=0,txMaxBytes=0;
vex::LinkTiming timing;
// 1 kHz hardware timer drives the amber LED beat flash on drills so a slow SPI display flush cannot jitter it.
IntervalTimer ledTimer;
volatile uint32_t isrBeatPeriod=0,isrBeatOrigin=0;
volatile bool isrBeatActive=false;
void ledBeatIsr(){
 if(!isrBeatActive)return;
 digitalWriteFast(PIN_LED,vex::App::beatFlash(millis(),isrBeatOrigin,isrBeatPeriod)?HIGH:LOW);
}
// Bench switches (serial) and simulated control presses; presses go through App::controls like buttons.
bool forceFull=false,bandDiff=true,linkOn=false,testOn=false;uint32_t tapUntil[3]={0,0,0};
// Live knobs: boot at the config.h values, step from serial, never persisted. Steps the GC9A01A and
// the Teensy 4.1 SPI divider can both produce; 16 MHz is the documented starting point.
const uint32_t SPI_STEPS[]={8000000,16000000,24000000,30000000,40000000,60000000};
uint32_t spiHz=TFT_HZ;uint8_t inLevel=INPUT_LEVEL;
void setSpi(uint32_t hz){spiHz=hz;lcd.setSPISpeed(hz);shadowValid=false;}   // full frame so a bad clock shows at once
void benchCommand(char c,uint32_t now){
 switch(c){
  case 'f':forceFull=!forceFull;break;
  case 'b':bandDiff=!bandDiff;break;
  case '1':timing.cadence_mul=1;break;
  case '2':timing.cadence_mul=2;break;
  case 'l':linkOn=!linkOn;shadowValid=false;break;   // a fresh full frame so the mirror starts complete
  case 't':testOn=!testOn;shadowValid=false;break;
  case '-':for(int i=5;i>=0;i--)if(SPI_STEPS[i]<spiHz){setSpi(SPI_STEPS[i]);break;}break;
  case '+':for(int i=0;i<6;i++)if(SPI_STEPS[i]>spiHz){setSpi(SPI_STEPS[i]);break;}break;
  case 'g':if(VEX_STAGE>=3&&app.codec_ok&&inLevel>0){inLevel--;codec.lineInLevel(inLevel);}break;
  case 'G':if(VEX_STAGE>=3&&app.codec_ok&&inLevel<15){inLevel++;codec.lineInLevel(inLevel);}break;
  case 'm':tapUntil[0]=now+100;break;case 'q':tapUntil[1]=now+100;break;case 'k':tapUntil[2]=now+100;break;
  case 'M':tapUntil[0]=now+1300;break;case 'Q':tapUntil[1]=now+1300;break;case 'K':tapUntil[2]=now+1300;break;
  case '?':Serial.printf("{\"switches\":1,\"full\":%d,\"diff\":%d,\"cadence_ms\":%lu,\"link\":%d,\"test\":%d,\"spi_hz\":%lu,\"in_level\":%u}\n",forceFull,bandDiff,(unsigned long)(DISPLAY_MS*timing.cadence_mul),linkOn,testOn,(unsigned long)spiHz,inLevel);break;
  default:if(VEX_STAGE==4)app.command(c,now);
 }
}
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
 if(VEX_STAGE==4)ledTimer.begin(ledBeatIsr,1000);
}
void loop(){
 uint32_t loopStart=micros(),now=millis();
 if(VEX_STAGE==1){digitalWrite(13,(now/500)%2);if(now-printAt>=1000){printAt=now;Serial.println("VEX USB OK");}return;}
 if(VEX_STAGE>=3){
#ifdef VEX_PITCH_PJRC
  bool fresh=false;if(rmsNode.available()){rms=rmsNode.read();fresh=true;}if(peakNode.available()){peak=peakNode.read();fresh=true;}
  if(fresh)app.observe(rms,peak,now);
  if(app.wantsPitch()&&noteNode.available())app.observePitch(noteNode.read(),noteNode.probability(),now);
#else
  analyzer.setGate(app.progress.gate);
  uint32_t sc=0;
  if(analyzer.takeLevel(rms,peak,&sc))app.observe(rms,peak,now,sc);
  vex::Onset ons;while(analyzer.takeOnset(ons))app.observeOnset(ons,now);
  vex::PitchMode pm=app.pitchMode();
  if(pm!=vex::PITCH_OFF){
   uint32_t started=micros();
   if(analyzer.analyze(pm,app.slow_run_hop)){
    timing.yin_us=micros()-started;if(timing.yin_us>timing.yin_max_us)timing.yin_max_us=timing.yin_us;
    if(pm==vex::PITCH_RUN&&timing.yin_us>14000)app.slow_run_hop=true; // >60% of 23.2 ms hop -> fall back to 2048 hop
    app.observePitch(analyzer.hz,analyzer.probability,millis(),analyzer.pitch_sample,vex::pitchProfile(pm,app.slow_run_hop).window);
   }
  }else{analyzer.skip();timing.yin_us=0;}
#endif
 }
 app.controls(digitalRead(PIN_MODE)==LOW||now<tapUntil[0],digitalRead(PIN_QUEST)==LOW||now<tapUntil[1],digitalRead(PIN_FOOT)==LOW||now<tapUntil[2],now);
 while(Serial.available())benchCommand(char(Serial.read()),now);
 app.tick(now);
 if(app.beat_period_ms>0&&app.progress.running){
  noInterrupts();isrBeatPeriod=app.beat_period_ms;isrBeatOrigin=app.beat_origin_ms;isrBeatActive=true;interrupts();
 }else{
  if(isrBeatActive){noInterrupts();isrBeatActive=false;interrupts();}
  digitalWrite(PIN_LED,app.led());
 }
 if(now-drawAt>=DISPLAY_MS*timing.cadence_mul){
  drawAt=now;uint32_t drawStart=micros();
  if(testOn)vex::drawTestPattern(frame,now);else vex::drawScreen(frame,app.view());
  const uint16_t* px=frame.getBuffer();
  vex::Band bands[vex::FLUSH_MAX_BANDS];int n;
  if(shadowValid&&bandDiff&&!forceFull&&!testOn)n=vex::bands(shadow,px,bands);else{bands[0].y=0;bands[0].h=vex::FRAME_H;n=1;shadowValid=true;}
  timing.draw_us=micros()-drawStart;if(timing.draw_us>timing.draw_max_us)timing.draw_max_us=timing.draw_us;
  uint32_t flushStart=micros();
  if(n){
   panel.begin();
   for(int i=0;i<n;i++){size_t at=size_t(bands[i].y)*vex::FRAME_W,count=size_t(bands[i].h)*vex::FRAME_W;panel.writeBand(bands[i].y,bands[i].h,px+at);memcpy(shadow+at,px+at,count*2);}
   panel.end();
   timing.tx_bytes=vex::bandBytes(bands,n);if(timing.tx_bytes>txMaxBytes)txMaxBytes=timing.tx_bytes;
   timing.flush_us=micros()-flushStart;
  }else{timing.tx_bytes=0;timing.flush_us=0;}
  if(timing.flush_us>timing.flush_max_us)timing.flush_max_us=timing.flush_us;
  if(linkOn){for(int i=0;i<n;i++)vex::linkBand(Serial,bands[i].y,bands[i].h,px+size_t(bands[i].y)*vex::FRAME_W);vex::linkState(Serial,vex::linkState(app,timing));}
 }
 if(now-printAt>=1000){
  printAt=now;const auto& p=app.progress;timing.cpu=VEX_STAGE>=3?AudioProcessorUsageMax():0;vex::AnimFrame f=vex::chooseAnim(vex::quantised(app.view()));
  Serial.printf("{\"stage\":%d,\"up_ms\":%lu,\"codec\":%d,\"activity\":\"%s\",\"onsets\":%lu,\"bpm\":%u,\"rms\":%.5f,\"peak\":%.3f,\"gate\":%.5f,\"active\":%d,\"running\":%d,\"session_s\":%lu,\"total_s\":%lu,\"fragments\":%lu,\"level\":%lu,\"credit_drops\":%lu,\"cpu\":%.1f,\"memory\":%u,"
   "\"tx_bytes\":%lu,\"tx_max_bytes\":%lu,\"draw_us\":%lu,\"draw_max_us\":%lu,\"flush_us\":%lu,\"flush_max_us\":%lu,\"loop_max_us\":%lu,\"cadence_ms\":%lu,\"diff\":%d,\"full\":%d,\"link\":%d,\"test\":%d,\"spi_hz\":%lu,\"in_level\":%u,\"anim\":%d,\"frame\":%d",
   VEX_STAGE,(unsigned long)now,app.codec_ok,app.activityName(),(unsigned long)app.notes.total_events,(unsigned)app.liveBpm(),app.rms,app.peak,p.gate,p.active,p.running,(unsigned long)(p.session_ms/1000),(unsigned long)(p.active_ms/1000),(unsigned long)p.fragments,(unsigned long)p.level(),(unsigned long)p.credit_drops,timing.cpu,VEX_STAGE>=3?AudioMemoryUsageMax():0,
   (unsigned long)timing.tx_bytes,(unsigned long)txMaxBytes,(unsigned long)timing.draw_us,(unsigned long)timing.draw_max_us,(unsigned long)timing.flush_us,(unsigned long)timing.flush_max_us,(unsigned long)timing.loop_max_us,(unsigned long)(DISPLAY_MS*timing.cadence_mul),bandDiff,forceFull,linkOn,testOn,(unsigned long)spiHz,inLevel,f.anim,f.frame);
#ifdef VEX_PITCH_PJRC
  Serial.printf(",\"pitch\":\"pjrc\",\"midi\":%d,\"cents\":%.1f,\"valid\":%d}\n",app.pitchNow.midi,app.pitchNow.cents,app.pitchNow.valid);
#else
  Serial.printf(",\"pitch\":\"yin\",\"hz\":%.2f,\"prob\":%.2f,\"yin_us\":%lu,\"yin_max_us\":%lu}\n",VEX_STAGE>=3?analyzer.hz:0.f,VEX_STAGE>=3?analyzer.probability:0.f,(unsigned long)timing.yin_us,(unsigned long)timing.yin_max_us);
#endif
  timing.loop_max_us=0;   // per-second maximum
 }
 uint32_t loopUs=micros()-loopStart;if(loopUs>timing.loop_max_us)timing.loop_max_us=loopUs;
}
