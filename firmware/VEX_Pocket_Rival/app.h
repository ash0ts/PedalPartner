#pragma once
#include "core.h"
#include "notes.h"
#include "drills.h"
#include "ui.h"
namespace vex {
// Byte storage adapter. Teensy wraps EEPROM; the simulator wraps a file and can cut power mid-write.
struct Store {virtual uint8_t read(int address)=0;virtual void update(int address,uint8_t value)=0;virtual ~Store(){}};
// Mean-square level window: spans more than three periods of low E2 (82.4 Hz) so block-to-block
// harmonic troughs do not drop activity mid-note, and settles with no infinite tail when playing stops.
constexpr uint32_t LEVEL_SMOOTH_MS=40;
struct LevelObs{uint16_t dt;float ms;};
// Device state machine shared by the sketch and the desktop simulator. Adapters feed control
// levels, audio observations and a millisecond clock; nothing here touches hardware or draws.
struct App {
 Store& store;int stage;
 Progress progress;Skills skills;DrillHost drills;Button mode,quest,foot;Pitch pitchNow;NoteTracker notes;
 Activity activity=ACT_PET;
 PitchMode pitch_override=PITCH_OFF;
 bool codec_ok=false,calibrating=false,raw_mode=false,raw_quest=false,raw_foot=false,obs_seen=false,slow_run_hop=false;
 // Tuner presentation facts: a periodic candidate below the confidence threshold (uncertain), and the last
 // confident reading so the display can fade it instead of blanking the moment a string decays.
 bool uncertain=false;Pitch last_pitch;uint32_t last_pitch_at=0;
 float rms=0,peak=0,env=0,noise[300],hz_hist[3]={0,0,0};uint16_t noise_count=0;uint8_t hz_count=0,obs_head=0,obs_count=0;
 LevelObs obs[LEVEL_SMOOTH_MS]{};
 uint32_t now=0,rms_at=0,peak_at=0,pitch_at=0,tick_at=0,save_at=0,active_at=0,cal_at=0,sample_at=0,notice_at=0,sequence=0,saved_hash=0;
 uint32_t skills_seq=0,skills_hash=0,beat_period_ms=0,beat_origin_ms=0,next_beat_ms=0;
 // Presentation-only session facts (never saved): when the fragment/level-up happened, what the last
 // session earned, and when it was paused so the summary card can show for a few seconds.
 uint32_t fragment_at=0,levelup_at=0,summary_at=0,summary_ms=0;uint64_t session_xp_start=0;uint32_t summary_xp=0;uint32_t last_level=1;
 int saved_slot=-1,skills_slot=-1;const char* notice="Hold QUEST: calibrate";
 App(Store& s,int stage_):store(s),stage(stage_){}
 void message(const char* s){notice=s;notice_at=now;}
 void begin(uint32_t t,bool codec){
  now=t;codec_ok=codec;obs_seen=false;obs_head=obs_count=0;env=0;
  progress=Progress();skills=Skills();saved_slot=-1;skills_slot=-1;sequence=0;skills_seq=0;saved_hash=0;
  notes.clear();pitch_override=PITCH_OFF;activity=ACT_PET;beat_period_ms=beat_origin_ms=next_beat_ms=0;
  if(stage==4)load();
  skills_hash=skillsHash(skills);
  drills.select(drills.preset_idx,skills);
  tick_at=save_at=active_at=t;last_level=progress.level();session_xp_start=progress.xp();fragment_at=levelup_at=summary_at=t-100000;
  message(codec?"Hold QUEST: calibrate":"Check audio board");
 }
 // Session bookkeeping around running transitions. Called after any change to progress.running or newSession().
 void startSession(){
  session_xp_start=progress.xp();
  if(activity==ACT_DRILL){
   drills.start(now,notes.total_events);
   syncBeatClock();
  }
 }
 void pauseSession(){progress.active=false;progress.hold_ms=0;beat_period_ms=next_beat_ms=0;summary_ms=uint32_t(progress.session_ms);summary_xp=uint32_t(progress.xp()-session_xp_start);summary_at=now;}
 void setRunning(bool on){if(on==progress.running)return;progress.running=on;if(on)startSession();else pauseSession();}
 Record readRecord(int address){Record r{};uint8_t* p=(uint8_t*)&r;for(size_t i=0;i<sizeof r;i++)p[i]=store.read(address+i);return r;}
 bool readSkillsSlot(int address,Skills& out){
  uint8_t buf[SKILLS_BYTES];
  for(int i=0;i<SKILLS_BYTES;i++)buf[i]=store.read(address+i);
  return unpackSkills(buf,out);
 }
 void load(){
  Record a=readRecord(0),b=readRecord(64);bool av=valid(a),bv=valid(b);
  if(av||bv){saved_slot=(bv&&(!av||newer(b.sequence,a.sequence)))?1:0;const Record& r=saved_slot?b:a;restore(progress,r);sequence=r.sequence;saved_hash=record(progress,0).checksum;}
  Skills sa{},sb{};bool sav=readSkillsSlot(SKILLS_SLOT_A,sa),sbv=readSkillsSlot(SKILLS_SLOT_B,sb);
  if(sav||sbv){skills_slot=(sbv&&(!sav||newer(sb.sequence,sa.sequence)))?1:0;skills=skills_slot?sb:sa;skills_seq=skills.sequence;}
 }
 bool saveSkills(){
  uint32_t fp=skillsHash(skills);
  if(fp==skills_hash&&skills_slot>=0)return false;
  int target=skills_slot==0?1:0,address=target?SKILLS_SLOT_B:SKILLS_SLOT_A;
  uint8_t buf[SKILLS_BYTES];
  packSkills(skills,skills_seq+1,buf);
  for(int i=0;i<4;i++)store.update(address+i,0);
  for(int i=4;i<SKILLS_BYTES;i++)store.update(address+i,buf[i]);
  for(int i=0;i<4;i++)store.update(address+i,buf[i]);
  Skills check{};
  if(readSkillsSlot(address,check)){skills_slot=target;skills_seq=check.sequence;skills.sequence=check.sequence;skills_hash=fp;return true;}
  return false;
 }
 // Ordered write: clear magic, body, magic last, then read back. A torn write leaves the other slot valid.
 // `manual` saves (hold MODE, serial s) always report; the idle auto-save stays silent when nothing changed.
 bool save(bool manual=true){
  uint32_t fingerprint=record(progress,0).checksum;
  bool recChanged=(fingerprint!=saved_hash);
  bool skChanged=(skillsHash(skills)!=skills_hash);
  if(!recChanged&&!skChanged){if(manual)message("Already saved");return false;}
  bool ok=true;
  if(recChanged){
   int target=saved_slot==0?1:0,address=64*target;Record r=record(progress,sequence+1);const uint8_t* p=(const uint8_t*)&r;
   for(int i=0;i<4;i++)store.update(address+i,0);
   for(size_t i=4;i<sizeof r;i++)store.update(address+i,p[i]);
   for(int i=0;i<4;i++)store.update(address+i,p[i]);
   if(valid(readRecord(address))){saved_slot=target;sequence=r.sequence;saved_hash=fingerprint;}
   else ok=false;
  }
  if(ok&&skChanged){
   if(!saveSkills())ok=false;
  }
  if(ok){save_at=now;message("Saved - safe to unplug");return true;}
  message("SAVE FAILED");return false;
 }
 void calibrate(){
  if(!codec_ok){message("Check audio board");return;}
  setRunning(false);summary_at=now-100000;progress.calibrated=false;progress.active=false;progress.hold_ms=0;calibrating=true;noise_count=0;cal_at=now;sample_at=0;message("Mute strings 3 seconds");
 }
 void finishCalibration(){
  calibrating=false;if(noise_count<50){message("No audio samples");return;}
  for(uint16_t i=1;i<noise_count;i++){float v=noise[i];int j=i-1;while(j>=0&&noise[j]>v){noise[j+1]=noise[j];j--;}noise[j+1]=v;}
  float floor=noise[noise_count/2];if(!isfinite(floor)||floor>.08f){message("Too loud: try again");return;}
  progress.gate=fmaxf(.001f,floor*4);progress.calibrated=true;message("Ready: tap footswitch");
 }
 const ActivityDef& act()const{return activityDef(activity);}
 const char* activityName()const{return activity==ACT_DRILL?drills.preset().slug:act().name;}
 uint8_t activityCode()const{return activity==ACT_DRILL?uint8_t(2+drills.preset().family):uint8_t(activity);}
 bool tuning()const{return activity==ACT_TUNER;}
 void syncBeatClock(){
  if(activity==ACT_DRILL&&progress.running){
   beat_period_ms=drills.beatPeriodMs();
   beat_origin_ms=drills.beatOriginMs();
   if(beat_period_ms>0){
    uint32_t rel=now>=beat_origin_ms?(now-beat_origin_ms):0;
    next_beat_ms=beat_origin_ms+(rel/beat_period_ms+1u)*beat_period_ms;
   }else next_beat_ms=0;
  }else{
   beat_period_ms=next_beat_ms=0;
  }
 }
 void setActivity(Activity a){
  activity=a;setRunning(false);progress.active=false;progress.hold_ms=0;
  if(activity==ACT_DRILL)drills.select(drills.preset_idx,skills);
  syncBeatClock();
  message(act().enter_notice);
 }
 bool selectDrillPreset(uint8_t idx){
  if(progress.running){message("Pause to change preset");return false;}
  drills.select(idx,skills);
  message(drills.preset().title);
  return true;
 }
 void controls(bool m,bool q,bool f,uint32_t t){
  now=t;raw_mode=m;raw_quest=q;raw_foot=f;int em=mode.update(m,t),eq=quest.update(q,t),ef=foot.update(f,t);
  if(stage!=4)return;
  if(em==1)setActivity(Activity((uint8_t(activity)+1)%ACT_COUNT));
  if(em==2)save();
  if(eq==1){
   if(activity==ACT_PET){if(progress.setGoal((progress.goal_index+1)%3))message("Goal set; session reset");else message("Pause to change goal");}
   else if(activity==ACT_DRILL)selectDrillPreset(uint8_t((drills.preset_idx+1)%DRILL_PRESET_COUNT));
  }
  if(eq==2)calibrate();
  if(ef==1&&act().allows_run&&!calibrating){if(progress.calibrated)setRunning(!progress.running);else message("Hold QUEST: calibrate");}
  if(ef==2){setRunning(false);progress.newSession();summary_at=now-100000;message("New session / paused");}
 }
 void command(char c,uint32_t t){now=t;if(stage!=4)return;if(c=='c')calibrate();if(c=='s')save();if(c=='p')setRunning(false);if(c=='r'){setRunning(false);progress.newSession();summary_at=now-100000;}}
 PitchMode pitchMode()const{
  if(pitch_override!=PITCH_OFF)return pitch_override;
  if(stage==3)return PITCH_TUNER;
  if(activity==ACT_DRILL)return drills.pitchMode();
  return act().pitch_mode;
 }
 bool wantsPitch()const{return pitchMode()!=PITCH_OFF;}
 void observeOnset(const Onset& o,uint32_t t){now=t;notes.onOnset(o,t);}
 void observe(float r,float p,uint32_t t,uint32_t sample=0){
  now=t;rms=r;peak=p;
  if(sample&&isfinite(r)&&isfinite(p)&&r>=0&&p>=0)notes.onLevel(r,p,sample,progress.gate);
  if(!isfinite(r)||!isfinite(p)||r<0||p<0){env=r;obs_head=obs_count=0;obs_seen=false;rms_at=peak_at=t;return;}
  if(p>=.98f){env=0;obs_head=obs_count=0;obs_seen=true;rms_at=peak_at=t;return;}
  uint32_t gap=obs_seen?t-rms_at:100;rms_at=peak_at=t;obs_seen=true;
  float ms=r*r;
  if(gap>=100||obs_count==0){obs_head=0;obs_count=1;obs[0]={1,ms};env=r;return;}
  if(gap==0){obs[(obs_head+obs_count-1)%LEVEL_SMOOTH_MS].ms=ms;}
  else{
   uint16_t w=gap>LEVEL_SMOOTH_MS?uint16_t(LEVEL_SMOOTH_MS):uint16_t(gap);
   uint32_t total=0;for(uint8_t i=0;i<obs_count;i++)total+=obs[(obs_head+i)%LEVEL_SMOOTH_MS].dt;
   while(obs_count>0&&total+w>LEVEL_SMOOTH_MS){
    LevelObs& old=obs[obs_head];uint32_t excess=total+w-LEVEL_SMOOTH_MS;
    if(old.dt<=excess){total-=old.dt;obs_head=(obs_head+1)%LEVEL_SMOOTH_MS;obs_count--;}
    else{old.dt=uint16_t(old.dt-excess);total-=excess;}
   }
   obs[(obs_head+obs_count)%LEVEL_SMOOTH_MS]={w,ms};obs_count++;
  }
  float sum=0;uint32_t dur=0;
  for(uint8_t i=0;i<obs_count;i++){const LevelObs& o=obs[(obs_head+i)%LEVEL_SMOOTH_MS];sum+=o.ms*o.dt;dur+=o.dt;}
  env=dur?sqrtf(sum/dur):r;
 }
 // Tuner smoothing: a three-sample median over valid estimates removes single-window glitches, and the
 // displayed note only changes once the estimate is more than 60 cents from the current note's centre.
 void observePitch(float hz,float probability,uint32_t t,uint32_t win_end_sample=0,uint16_t win_len=2048){
  now=t;pitch_at=t;
  if(win_end_sample)notes.onPitch(hz,probability,win_end_sample,win_len,pitchMode()!=PITCH_FAST,t);
  Pitch raw=pitch(hz,probability);
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
  if(calibrating&&now-sample_at>=10&&now-rms_at<50){if(noise_count<300)noise[noise_count++]=env;sample_at=now;}
  if(calibrating&&now-cal_at>=3000)finishCalibration();
  if(activity==ACT_DRILL&&progress.running&&!calibrating){
   bool newBest=false;
   if(drills.update(notes,now,skills,newBest)&&newBest)saveSkills();
   syncBeatClock();
  }
  if(now-tick_at>=10){
   uint32_t dt=now-tick_at;tick_at=now;bool was_claimed=progress.claimed;
   progress.tick(dt,env,peak,fresh()&&act().credits_practice&&!calibrating,rms>progress.gate);if(progress.active)active_at=now;
   if(progress.claimed&&!was_claimed)fragment_at=now;
   if(progress.level()>last_level){last_level=progress.level();levelup_at=now;}
  }
  if(now-save_at>=60000&&now-active_at>=3000&&!calibrating){save(false);save_at=now;}
 }
 static bool beatFlash(uint32_t t_ms,uint32_t origin_ms,uint32_t period_ms){
  if(!period_ms)return false;
  uint32_t rel=t_ms>=origin_ms?(t_ms-origin_ms):0;
  uint32_t beat_num=rel/period_ms,in_beat=rel%period_ms;
  return in_beat<((beat_num%4u==0u)?75u:32u);
 }
 bool ledAt(uint32_t t_ms)const{
  if(stage==2)return raw_foot;
  if(beat_period_ms>0&&progress.running)return beatFlash(t_ms,beat_origin_ms,beat_period_ms);
  return progress.active;
 }
 bool led()const{return ledAt(now);}
 uint16_t liveBpm()const{return activity==ACT_DRILL?drills.liveBpm():0u;}
 View view()const{
  View v;v.progress=progress;v.pitch=pitchNow;v.stage=stage;v.activity=activity;v.codec_ok=codec_ok;v.calibrating=calibrating;
  if(activity==ACT_DRILL)v.drill=drills.view(skills,progress.running,now);
  v.rms=rms;v.peak=peak;v.now=now;v.active_at=active_at;v.notice_at=notice_at;v.notice=notice;
  v.pitch_fresh=now-pitch_at<350&&now-rms_at<100&&now-peak_at<100;
  v.uncertain=uncertain&&v.pitch_fresh;v.last_pitch=last_pitch;v.last_pitch_at=last_pitch_at;
  v.mode_pressed=raw_mode;v.quest_pressed=raw_quest;v.foot_pressed=raw_foot;
  v.fragment_at=fragment_at;v.levelup_at=levelup_at;v.summary_at=summary_at;v.summary_ms=summary_ms;v.summary_xp=summary_xp;return v;
 }
};
}
