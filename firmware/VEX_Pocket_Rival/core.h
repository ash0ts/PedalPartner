#pragma once
#include <stdint.h>
#include <stddef.h>
#include <math.h>
namespace vex {
struct Progress {
 uint64_t active_ms=0, session_ms=0;
 uint32_t fragments=0;
 uint8_t goal_index=0;
 bool claimed=false, running=false, calibrated=false, active=false;
 float gate=.004f;
 void newSession(){session_ms=0;claimed=false;running=false;active=false;}
 bool setGoal(uint8_t i){if(running||i>2)return false;goal_index=i;newSession();return true;}
 uint32_t goalMs() const {const uint32_t m[]={5,10,20};return m[goal_index]*60000u;}
 void tick(uint32_t dt,float rms,float peak,bool fresh){
  active=false;
  if(!running||!calibrated||!fresh||dt>100||!isfinite(rms)||!isfinite(peak)||rms<0||peak<0||peak>=.98f)return;
  active=rms>gate;
  if(active){active_ms+=dt;session_ms+=dt;}
  if(!claimed&&session_ms>=goalMs()){claimed=true;++fragments;}
 }
 uint64_t xp() const{return active_ms/6000;}
 uint32_t level() const{return 1+uint32_t(active_ms/600000);}
};
struct Pitch {int midi=0;float cents=0;bool valid=false;};
inline Pitch pitch(float hz,float probability){
 if(!isfinite(hz)||!isfinite(probability)||hz<55||hz>1400||probability<.90f)return {};
 float n=69+12*log2f(hz/440.f);int note=int(lroundf(n));return {note,(n-note)*100,true};
}
// 48 bytes; two 64-byte EEPROM slots. CRC-32/ISO-HDLC protects payload.
struct Record {uint32_t magic,version,sequence,fragments,goal_index,claimed;uint64_t active_ms,session_ms;uint32_t checksum,padding;};
static_assert(sizeof(Record)==48,"Record layout changed: version and migration required");
inline uint32_t crc(const Record&r){uint32_t c=0xffffffffu;const auto*p=(const uint8_t*)&r;for(size_t i=0;i<offsetof(Record,checksum);++i){c^=p[i];for(int j=0;j<8;j++)c=(c>>1)^(0xedb88320u&uint32_t(-int32_t(c&1)));}return ~c;}
inline Record record(const Progress&p,uint32_t seq){Record r{};r.magic=0x56455831;r.version=1;r.sequence=seq;r.fragments=p.fragments;r.goal_index=p.goal_index;r.claimed=p.claimed;r.active_ms=p.active_ms;r.session_ms=p.session_ms;r.checksum=crc(r);return r;}
inline bool valid(const Record&r){return r.magic==0x56455831&&r.version==1&&r.goal_index<3&&r.claimed<2&&r.checksum==crc(r);}
inline bool newer(uint32_t a,uint32_t b){return int32_t(a-b)>0;}
inline void restore(Progress&p,const Record&r){p.active_ms=r.active_ms;p.session_ms=r.session_ms;p.fragments=r.fragments;p.goal_index=r.goal_index;p.claimed=r.claimed;p.running=false;p.calibrated=false;}
struct Button {
 bool raw=false,stable=false,held=false;uint32_t changed=0,down=0;
 int update(bool pressed,uint32_t now){
  if(pressed!=raw){raw=pressed;changed=now;}
  if(now-changed>=25&&raw!=stable){stable=raw;if(stable){down=now;held=false;}else if(!held)return 1;}
  if(stable&&!held&&now-down>=1200){held=true;return 2;}return 0;
 }
};
}
