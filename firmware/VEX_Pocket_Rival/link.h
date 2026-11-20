#pragma once
#include <stdint.h>
#include <string.h>
#include "app.h"
#include "ui.h"
// Bench link: what the device shows and measures, framed for USB serial so the workbench can mirror
// the real panel beside the WebAssembly one. Two packet types share a 12-byte header:
//   'V' 'X' type pad | uint16 a | uint16 b | uint32 crc32(payload)
//   type 'B' band:  a = first row, b = row count, payload = b * 240 RGB565 little-endian pixels
//   type 'S' state: a = sizeof(LinkState), b = 0, payload = LinkState (packed, little-endian)
// Plain text lines (the JSON telemetry) may sit between packets; a decoder resynchronises on 'VX'
// and a matching CRC. Encoding writes through any object with write(const uint8_t*, size_t), so the
// same code feeds Serial on the Teensy and a byte vector in the host tests. Nothing here allocates.
namespace vex {
constexpr uint8_t LINK_BAND='B',LINK_STATE='S';constexpr size_t LINK_HEADER=12;
inline uint32_t crc32(const uint8_t* p,size_t n){uint32_t c=0xffffffffu;for(size_t i=0;i<n;++i){c^=p[i];for(int j=0;j<8;j++)c=(c>>1)^(0xedb88320u&uint32_t(-int32_t(c&1)));}return ~c;}
#pragma pack(push,1)
struct LinkState{
 uint32_t now_ms,active_ms,session_ms,fragments,sequence;
 float gate,rms,peak,cents,cpu;
 uint32_t tx_bytes,flush_us,flush_max_us,yin_us,yin_max_us,loop_max_us;
 int16_t midi;uint16_t goal_minutes;
 uint8_t stage,flags,anim,frame;int8_t saved_slot;uint8_t cadence_mul,level,pad;
};
#pragma pack(pop)
static_assert(sizeof(LinkState)==76,"LinkState layout is part of the bench protocol; update tools/link.py too");
// flags bits
constexpr uint8_t LINK_RUNNING=1,LINK_CALIBRATED=2,LINK_ACTIVE=4,LINK_TUNING=8,LINK_CALIBRATING=16,LINK_CLAIMED=32,LINK_CODEC=64,LINK_PITCH_VALID=128;
// Measurements the adapter owns (display timing, analysis cost, audio CPU); App knows nothing about them.
struct LinkTiming{uint32_t tx_bytes=0,draw_us=0,draw_max_us=0,flush_us=0,flush_max_us=0,yin_us=0,yin_max_us=0,loop_max_us=0;float cpu=0;uint8_t cadence_mul=1;};
inline LinkState linkState(const App& app,const LinkTiming& t){
 LinkState s{};const Progress& p=app.progress;View v=quantised(app.view());AnimFrame f=chooseAnim(v);
 s.now_ms=app.now;s.active_ms=uint32_t(p.active_ms);s.session_ms=uint32_t(p.session_ms);s.fragments=p.fragments;s.sequence=app.sequence;
 s.gate=p.gate;s.rms=app.rms;s.peak=app.peak;s.cents=app.pitchNow.cents;s.cpu=t.cpu;
 s.tx_bytes=t.tx_bytes;s.flush_us=t.flush_us;s.flush_max_us=t.flush_max_us;s.yin_us=t.yin_us;s.yin_max_us=t.yin_max_us;s.loop_max_us=t.loop_max_us;
 s.midi=int16_t(app.pitchNow.midi);s.goal_minutes=uint16_t(p.goalMs()/60000);
 s.stage=uint8_t(app.stage);s.anim=uint8_t(f.anim);s.frame=uint8_t(f.frame);s.saved_slot=int8_t(app.saved_slot);s.cadence_mul=t.cadence_mul;s.level=uint8_t(p.level()>255?255:p.level());
 s.flags=(p.running?LINK_RUNNING:0)|(p.calibrated?LINK_CALIBRATED:0)|(p.active?LINK_ACTIVE:0)|(app.tuning?LINK_TUNING:0)|(app.calibrating?LINK_CALIBRATING:0)|(p.claimed?LINK_CLAIMED:0)|(app.codec_ok?LINK_CODEC:0)|(app.pitchNow.valid?LINK_PITCH_VALID:0);
 return s;
}
template<class Out> void linkPacket(Out& out,uint8_t type,uint16_t a,uint16_t b,const uint8_t* payload,size_t n){
 uint8_t h[LINK_HEADER]={'V','X',type,0,uint8_t(a),uint8_t(a>>8),uint8_t(b),uint8_t(b>>8),0,0,0,0};
 uint32_t c=crc32(payload,n);h[8]=uint8_t(c);h[9]=uint8_t(c>>8);h[10]=uint8_t(c>>16);h[11]=uint8_t(c>>24);
 out.write(h,LINK_HEADER);out.write(payload,n);
}
template<class Out> void linkBand(Out& out,uint16_t y,uint16_t h,const uint16_t* rows){linkPacket(out,LINK_BAND,y,h,(const uint8_t*)rows,size_t(h)*FRAME_W*2);}
template<class Out> void linkState(Out& out,const LinkState& s){linkPacket(out,LINK_STATE,uint16_t(sizeof s),0,(const uint8_t*)&s,sizeof s);}
// Host-side parse of one packet at the start of `buf`. Returns bytes consumed; 0 when more bytes are
// needed; -1 when the header or CRC is wrong (caller skips one byte and retries).
struct LinkPacket{uint8_t type;uint16_t a,b;const uint8_t* payload;size_t length;};
inline long linkParse(const uint8_t* buf,size_t n,LinkPacket& out){
 if(n<LINK_HEADER)return 0;
 if(buf[0]!='V'||buf[1]!='X'||(buf[2]!=LINK_BAND&&buf[2]!=LINK_STATE)||buf[3]!=0)return -1;
 uint16_t a=uint16_t(buf[4]|buf[5]<<8),b=uint16_t(buf[6]|buf[7]<<8);uint32_t c=uint32_t(buf[8])|uint32_t(buf[9])<<8|uint32_t(buf[10])<<16|uint32_t(buf[11])<<24;
 size_t length=buf[2]==LINK_BAND?size_t(b)*FRAME_W*2:a;
 if(buf[2]==LINK_BAND&&(b==0||a+b>FRAME_H))return -1;
 if(buf[2]==LINK_STATE&&a!=sizeof(LinkState))return -1;
 if(n<LINK_HEADER+length)return 0;
 if(crc32(buf+LINK_HEADER,length)!=c)return -1;
 out={buf[2],a,b,buf+LINK_HEADER,length};return long(LINK_HEADER+length);
}
}
