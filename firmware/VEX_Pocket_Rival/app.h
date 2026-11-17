#pragma once
#include "core.h"
#include "ui.h"
namespace vex {
// Byte storage adapter. Teensy wraps EEPROM; the simulator wraps a file and can cut power mid-write.
struct Store {virtual uint8_t read(int address)=0;virtual void update(int address,uint8_t value)=0;virtual ~Store(){}};
// Device state machine shared by the sketch and the desktop simulator. Adapters feed control
// levels, audio observations and a millisecond clock; nothing here touches hardware or draws.
struct App {
 Store& store;int stage;
 Progress progress;Button mode,quest,foot;Pitch pitchNow;
 bool codec_ok=false,tuning=false,calibrating=false,raw_mode=false,raw_quest=false,raw_foot=false;
 // Tuner presentation facts: a periodic candidate below the confidence threshold (uncertain), and the last
 // confident reading so the display can fade it instead of blanking the moment a string decays.
 bool uncertain=false;Pitch last_pitch;uint32_t last_pitch_at=0;
 float rms=0,peak=0,noise[300],hz_hist[3]={0,0,0};uint16_t noise_count=0;uint8_t hz_count=0;
 uint32_t now=0,rms_at=0,peak_at=0,pitch_at=0,tick_at=0,save_at=0,active_at=0,cal_at=0,sample_at=0,notice_at=0,sequence=0,saved_hash=0;
 // Presentation-only session facts (never saved): when the fragment/level-up happened, what the last
 // session earned, and when it was paused so the summary card can show for a few seconds.
 uint32_t fragment_at=0,levelup_at=0,summary_at=0,summary_ms=0;uint64_t session_xp_start=0;uint32_t summary_xp=0;uint32_t last_level=1;
 int saved_slot=-1;const char* notice="Hold QUEST: calibrate";
 App(Store& s,int stage_):store(s),stage(stage_){}
 void message(const char* s){notice=s;notice_at=now;}
 void begin(uint32_t t,bool codec){now=t;codec_ok=codec;if(stage==4)load();tick_at=save_at=active_at=t;last_level=progress.level();session_xp_start=progress.xp();fragment_at=levelup_at=summary_at=t-100000;message("Hold QUEST: calibrate");}
 // Session bookkeeping around running transitions. Called after any change to progress.running or newSession().
 void startSession(){session_xp_start=progress.xp();}
 void pauseSession(){summary_ms=uint32_t(progress.session_ms);summary_xp=uint32_t(progress.xp()-session_xp_start);summary_at=now;}
 void setRunning(bool on){if(on==progress.running)return;progress.running=on;if(on)startSession();else pauseSession();}
 Record readRecord(int address){Record r{};uint8_t* p=(uint8_t*)&r;for(size_t i=0;i<sizeof r;i++)p[i]=store.read(address+i);return r;}
 void load(){
  Record a=readRecord(0),b=readRecord(64);bool av=valid(a),bv=valid(b);
  if(av||bv){saved_slot=(bv&&(!av||newer(b.sequence,a.sequence)))?1:0;const Record& r=saved_slot?b:a;restore(progress,r);sequence=r.sequence;saved_hash=record(progress,0).checksum;}
 }
 // Ordered write: clear magic, body, magic last, then read back. A torn write leaves the other slot valid.
 // `manual` saves (hold MODE, serial s) always report; the idle auto-save stays silent when nothing changed.
 bool save(bool manual=true){
  uint32_t fingerprint=record(progress,0).checksum;
  if(fingerprint==saved_hash){if(manual)message("Already saved");return false;}
  int target=saved_slot==0?1:0,address=64*target;Record r=record(progress,sequence+1);const uint8_t* p=(const uint8_t*)&r;
  for(int i=0;i<4;i++)store.update(address+i,0);
  for(size_t i=4;i<sizeof r;i++)store.update(address+i,p[i]);
  for(int i=0;i<4;i++)store.update(address+i,p[i]);
  if(valid(readRecord(address))){saved_slot=target;sequence=r.sequence;saved_hash=fingerprint;save_at=now;message("Saved - safe to unplug");return true;}
  message("SAVE FAILED");return false;
 }
 void calibrate(){
  if(!codec_ok){message("Check audio board");return;}
  setRunning(false);summary_at=now-100000;progress.calibrated=false;progress.active=false;calibrating=true;noise_count=0;cal_at=now;sample_at=0;message("Mute strings 3 seconds");
 }
 void finishCalibration(){
  calibrating=false;if(noise_count<50){message("No audio samples");return;}
  for(uint16_t i=1;i<noise_count;i++){float v=noise[i];int j=i-1;while(j>=0&&noise[j]>v){noise[j+1]=noise[j];j--;}noise[j+1]=v;}
  float floor=noise[noise_count/2];if(!isfinite(floor)||floor>.08f){message("Too loud: try again");return;}
  progress.gate=fmaxf(.001f,floor*4);progress.calibrated=true;message("Ready: tap footswitch");
 }
 void controls(bool m,bool q,bool f,uint32_t t){
  now=t;raw_mode=m;raw_quest=q;raw_foot=f;int em=mode.update(m,t),eq=quest.update(q,t),ef=foot.update(f,t);
  if(stage!=4)return;
  if(em==1){tuning=!tuning;setRunning(false);progress.active=false;message(tuning?"G3S handles muting":"Tap footswitch to start");}
  if(em==2)save();
  if(eq==1){if(progress.setGoal((progress.goal_index+1)%3))message("Goal set; session reset");else message("Pause to change goal");}
  if(eq==2)calibrate();
  if(ef==1&&!tuning&&!calibrating){if(progress.calibrated)setRunning(!progress.running);else message("Hold QUEST: calibrate");}
  if(ef==2){setRunning(false);progress.newSession();summary_at=now-100000;message("New session / paused");}
 }
 void command(char c,uint32_t t){now=t;if(stage!=4)return;if(c=='c')calibrate();if(c=='s')save();if(c=='p')setRunning(false);if(c=='r'){setRunning(false);progress.newSession();summary_at=now-100000;}}
 void observe(float r,float p,uint32_t t){now=t;rms=r;peak=p;rms_at=peak_at=t;}
 // Tuner smoothing: a three-sample median over valid estimates removes single-window glitches, and the
 // displayed note only changes once the estimate is more than 60 cents from the current note's centre.
 void observePitch(float hz,float probability,uint32_t t){
  now=t;pitch_at=t;Pitch raw=pitch(hz,probability);
  if(!raw.valid){hz_count=0;pitchNow=Pitch{};uncertain=isfinite(hz)&&isfinite(probability)&&hz>=55&&hz<=1400&&probability>=PITCH_UNCERTAIN;return;}
  uncertain=false;
  hz_hist[0]=hz_hist[1];hz_hist[1]=hz_hist[2];hz_hist[2]=hz;if(hz_count<3)hz_count++;
  float m=hz;if(hz_count>=3){float a=hz_hist[0],b=hz_hist[1],c=hz_hist[2];m=fmaxf(fminf(a,b),fminf(fmaxf(a,b),c));}
  Pitch p=pitch(m,probability);
  if(pitchNow.valid&&p.valid&&p.midi!=pitchNow.midi){float from=(69+12*log2f(m/440.f)-pitchNow.midi)*100;if(fabsf(from)<=60){p.midi=pitchNow.midi;p.cents=from;}}
  pitchNow=p;if(p.valid){last_pitch=p;last_pitch_at=t;}
 }
 bool fresh()const{return codec_ok&&now-rms_at<100&&now-peak_at<100;}
 void tick(uint32_t t){
  now=t;if(stage!=4)return;
  if(calibrating&&now-sample_at>=10&&now-rms_at<50){if(noise_count<300)noise[noise_count++]=rms;sample_at=now;}
  if(calibrating&&now-cal_at>=3000)finishCalibration();
  if(now-tick_at>=10){
   uint32_t dt=now-tick_at;tick_at=now;bool was_claimed=progress.claimed;
   progress.tick(dt,rms,peak,fresh()&&!tuning&&!calibrating);if(progress.active)active_at=now;
   if(progress.claimed&&!was_claimed)fragment_at=now;
   if(progress.level()>last_level){last_level=progress.level();levelup_at=now;}
  }
  if(now-save_at>=60000&&now-active_at>=3000&&!calibrating){save(false);save_at=now;}
 }
 bool led()const{return stage==2?raw_foot:progress.active;}
 View view()const{
  View v;v.progress=progress;v.pitch=pitchNow;v.stage=stage;v.tuning=tuning;v.codec_ok=codec_ok;v.calibrating=calibrating;
  v.rms=rms;v.peak=peak;v.now=now;v.active_at=active_at;v.notice_at=notice_at;v.notice=notice;
  v.pitch_fresh=now-pitch_at<350&&now-rms_at<100&&now-peak_at<100;
  v.uncertain=uncertain&&v.pitch_fresh;v.last_pitch=last_pitch;v.last_pitch_at=last_pitch_at;
  v.mode_pressed=raw_mode;v.quest_pressed=raw_quest;v.foot_pressed=raw_foot;
  v.fragment_at=fragment_at;v.levelup_at=levelup_at;v.summary_at=summary_at;v.summary_ms=summary_ms;v.summary_xp=summary_xp;return v;
 }
};
}
