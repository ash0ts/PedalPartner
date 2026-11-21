#pragma once
#include <stdint.h>
#include <stddef.h>
#include <math.h>
namespace vex {
// Pitch analysis budgets selected per activity:
// - PITCH_TUNER: 55..1400 Hz, 2048 window, 2048 hop (46.4 ms; classic tuner & note finder)
// - PITCH_FAST:  140..1400 Hz, 1024 window, 512 hop (11.6 ms; ~0.22 M ops for bends & vibrato)
// - PITCH_RUN:   55..1400 Hz, 2048 window, 1024 hop (23.2 ms; scale runs, with 2048 fallback)
enum PitchMode : uint8_t { PITCH_OFF=0, PITCH_TUNER=1, PITCH_FAST=2, PITCH_RUN=3 };
struct PitchProfile { uint16_t window, hop, tau_min, tau_max; };
constexpr PitchProfile pitchProfile(PitchMode m, bool slowRunFallback=false) {
 switch (m) {
  case PITCH_FAST:  return {1024, 512,  44100/1400, 44100/140 + 1};
  case PITCH_RUN:   return {2048, uint16_t(slowRunFallback ? 2048 : 1024), 44100/1400, 44100/55 + 1};
  case PITCH_TUNER: return {2048, 2048, 44100/1400, 44100/55 + 1};
  case PITCH_OFF: default: return {0, 0, 0, 0};
 }
}
struct Onset { uint32_t sample=0; float rms=0, peak=0; };

// Foreground screens on stage 4. One table defines whether each screen credits practice, starts on
// the footswitch or runs pitch analysis, so adding a drill does not scatter flags across App and UI.
enum Activity : uint8_t { ACT_PET=0, ACT_TUNER=1, ACT_DRILL=2, ACT_COUNT=3 };
struct ActivityDef {
 Activity id;
 const char* name;
 const char* enter_notice;
 bool credits_practice, allows_run;
 PitchMode pitch_mode;
};
constexpr ActivityDef ACTIVITIES[ACT_COUNT]={
 {ACT_PET,  "pet",  "Tap footswitch to start", true,  true,  PITCH_OFF},
 {ACT_TUNER,"tuner","Rig handles muting",      false, false, PITCH_TUNER},
 {ACT_DRILL,"drill","QUEST: preset  FOOT: go", true,  true,  PITCH_OFF},
};
inline const ActivityDef& activityDef(Activity a){return ACTIVITIES[uint8_t(a)<ACT_COUNT?uint8_t(a):0];}
inline void midiNoteName(int midi,char out[8]){
 static constexpr const char* NAMES[12]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
 if(midi<=0||midi>127){out[0]='-';out[1]='\0';return;}
 const char* n=NAMES[midi%12];int oct=midi/12-1,k=0;
 while(*n)out[k++]=*n++;
 if(oct<0){out[k++]='-';oct=-oct;}
 if(oct>=10)out[k++]=char('0'+oct/10);
 out[k++]=char('0'+oct%10);out[k]='\0';
}
// Hold practice activity across inter-strum gaps; silence longer than this earns nothing.
constexpr uint16_t ACTIVITY_HOLD_MS=500;
struct Progress {
 uint64_t active_ms=0, session_ms=0;
 uint32_t fragments=0, credit_drops=0;
 uint16_t hold_ms=0;
 uint8_t goal_index=0;
 bool claimed=false, running=false, calibrated=false, active=false;
 float gate=.004f;
 void newSession(){session_ms=0;claimed=false;running=false;active=false;hold_ms=0;}
 bool setGoal(uint8_t i){if(running||i>2)return false;goal_index=i;newSession();return true;}
 uint32_t goalMs() const {const uint32_t m[]={5,10,20};return m[goal_index]*60000u;}
 void tick(uint32_t dt,float rms,float peak,bool fresh,bool refill=true){
  if(!running||!calibrated||!fresh||!isfinite(rms)||!isfinite(peak)||rms<0||peak<0||peak>=.98f){active=false;hold_ms=0;return;}
  if(dt>100){active=false;hold_ms=0;++credit_drops;return;}
  if(rms>gate&&refill){hold_ms=ACTIVITY_HOLD_MS;active=true;active_ms+=dt;session_ms+=dt;}
  else if(rms>gate){hold_ms=hold_ms>dt?uint16_t(hold_ms-dt):0;active=true;active_ms+=dt;session_ms+=dt;}
  else if(hold_ms>0){uint32_t c=dt<hold_ms?dt:hold_ms;hold_ms=uint16_t(hold_ms-c);active_ms+=c;session_ms+=c;active=hold_ms>0;}
  else active=false;
  if(!claimed&&session_ms>=goalMs()){claimed=true;++fragments;}
 }
 uint64_t xp() const{return active_ms/6000;}
 uint32_t level() const{return 1+uint32_t(active_ms/600000);}
};
struct Pitch {int midi=0;float cents=0;bool valid=false;};
// Confidence bands for the estimator's probability: at or above PITCH_VALID the note is shown; between
// PITCH_UNCERTAIN and PITCH_VALID there is a periodic candidate the tuner reports as uncertain.
constexpr float PITCH_VALID=.90f,PITCH_UNCERTAIN=.60f;
inline Pitch pitch(float hz,float probability){
 if(!isfinite(hz)||!isfinite(probability)||hz<55||hz>1400||probability<PITCH_VALID)return {};
 float n=69+12*log2f(hz/440.f);int note=int(lroundf(n));return {note,(n-note)*100,true};
}
// 48 bytes; two 64-byte EEPROM slots. CRC-32/ISO-HDLC protects payload.
struct Record {uint32_t magic,version,sequence,fragments,goal_index,claimed;uint64_t active_ms,session_ms;uint32_t checksum,padding;};
static_assert(sizeof(Record)==48,"Record layout changed: version and migration required");
inline uint32_t crcBytes(const uint8_t* p,size_t n){uint32_t c=0xffffffffu;for(size_t i=0;i<n;++i){c^=p[i];for(int j=0;j<8;j++)c=(c>>1)^(0xedb88320u&uint32_t(-int32_t(c&1)));}return ~c;}
inline uint32_t crc(const Record&r){return crcBytes((const uint8_t*)&r,offsetof(Record,checksum));}
inline Record record(const Progress&p,uint32_t seq){Record r{};r.magic=0x56455831;r.version=1;r.sequence=seq;r.fragments=p.fragments;r.goal_index=p.goal_index;r.claimed=p.claimed;r.active_ms=p.active_ms;r.session_ms=p.session_ms;r.checksum=crc(r);return r;}
inline bool valid(const Record&r){return r.magic==0x56455831&&r.version==1&&r.goal_index<3&&r.claimed<2&&r.checksum==crc(r);}
inline bool newer(uint32_t a,uint32_t b){return int32_t(a-b)>0;}
inline void restore(Progress&p,const Record&r){p.active_ms=r.active_ms;p.session_ms=r.session_ms;p.fragments=r.fragments;p.goal_index=r.goal_index;p.claimed=r.claimed;p.running=false;p.calibrated=false;p.active=false;p.hold_ms=0;p.credit_drops=0;}

// Two-slot CRC-protected skills block at EEPROM 128 and 192 (64 bytes each, explicit little-endian).
// Holds personal-best, last score and round count for up to 8 drills; never affects XP or Record v1.
enum SkillId : uint8_t { SKILL_PULSE=0, SKILL_HUNT=1, SKILL_BEND=2, SKILL_RUN=3, SKILL_TOUCH=4, SKILL_SLOTS=8 };
struct SkillStat { uint16_t best=0, last=0, rounds=0; };
struct Skills {
 uint32_t sequence=0;
 SkillStat stats[SKILL_SLOTS]{};
};
constexpr int SKILLS_SLOT_A=128, SKILLS_SLOT_B=192, SKILLS_BYTES=64;
constexpr uint32_t SKILLS_MAGIC=0x56585331u, SKILLS_VERSION=1u;

inline void putLe16(uint8_t* p,uint16_t v){p[0]=uint8_t(v&0xffu);p[1]=uint8_t((v>>8)&0xffu);}
inline void putLe32(uint8_t* p,uint32_t v){p[0]=uint8_t(v&0xffu);p[1]=uint8_t((v>>8)&0xffu);p[2]=uint8_t((v>>16)&0xffu);p[3]=uint8_t((v>>24)&0xffu);}
inline uint16_t getLe16(const uint8_t* p){return uint16_t(p[0])|(uint16_t(p[1])<<8);}
inline uint32_t getLe32(const uint8_t* p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}

inline void packSkills(const Skills& s,uint32_t seq,uint8_t out[SKILLS_BYTES]){
 putLe32(out+0,SKILLS_MAGIC);
 putLe32(out+4,SKILLS_VERSION);
 putLe32(out+8,seq);
 for(int i=0;i<SKILL_SLOTS;i++){
  int off=12+i*6;
  putLe16(out+off+0,s.stats[i].best);
  putLe16(out+off+2,s.stats[i].last);
  putLe16(out+off+4,s.stats[i].rounds);
 }
 putLe32(out+60,crcBytes(out,60));
}
inline bool unpackSkills(const uint8_t in[SKILLS_BYTES],Skills& out){
 if(getLe32(in+0)!=SKILLS_MAGIC||getLe32(in+4)!=SKILLS_VERSION)return false;
 if(getLe32(in+60)!=crcBytes(in,60))return false;
 out.sequence=getLe32(in+8);
 for(int i=0;i<SKILL_SLOTS;i++){
  int off=12+i*6;
  out.stats[i].best=getLe16(in+off+0);
  out.stats[i].last=getLe16(in+off+2);
  out.stats[i].rounds=getLe16(in+off+4);
 }
 return true;
}
inline uint32_t skillsHash(const Skills& s){
 uint8_t buf[SKILLS_BYTES];
 packSkills(s,0,buf);
 return getLe32(buf+60);
}
struct Button {
 bool raw=false,stable=false,held=false;uint32_t changed=0,down=0;
 int update(bool pressed,uint32_t now){
  if(pressed!=raw){raw=pressed;changed=now;}
  if(now-changed>=25&&raw!=stable){stable=raw;if(stable){down=now;held=false;}else if(!held)return 1;}
  if(stable&&!held&&now-down>=1200){held=true;return 2;}return 0;
 }
};
}
