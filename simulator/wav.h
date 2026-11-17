#pragma once
// Minimal readers for the desktop workbench: 16-bit PCM WAV (left channel) or raw 16-bit mono PCM, both 44.1 kHz.
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
inline bool readRaw(const std::string& path,std::vector<int16_t>& out,std::string& error){
 std::ifstream in(path,std::ios::binary);if(!in){error="cannot open "+path;return false;}
 std::vector<char> bytes((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
 if(bytes.size()%2){error="odd byte count in raw PCM";return false;}
 out.resize(bytes.size()/2);std::memcpy(out.data(),bytes.data(),bytes.size());return true;
}
inline bool readWav(const std::string& path,std::vector<int16_t>& out,std::string& error){
 std::ifstream in(path,std::ios::binary);if(!in){error="cannot open "+path;return false;}
 std::vector<char> b((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
 auto u32=[&](size_t o){return uint32_t(uint8_t(b[o]))|uint32_t(uint8_t(b[o+1]))<<8|uint32_t(uint8_t(b[o+2]))<<16|uint32_t(uint8_t(b[o+3]))<<24;};
 auto u16=[&](size_t o){return uint16_t(uint8_t(b[o])|uint8_t(b[o+1])<<8);};
 if(b.size()<12||std::memcmp(b.data(),"RIFF",4)||std::memcmp(b.data()+8,"WAVE",4)){error="not a RIFF/WAVE file";return false;}
 int channels=0,bits=0;uint32_t rate=0;size_t pos=12,data=0,size=0;
 while(pos+8<=b.size()){
  uint32_t len=u32(pos+4);size_t body=pos+8;
  if(!std::memcmp(b.data()+pos,"fmt ",4)&&len>=16){if(u16(body)!=1){error="only PCM WAV is supported";return false;}channels=u16(body+2);rate=u32(body+4);bits=u16(body+14);}
  else if(!std::memcmp(b.data()+pos,"data",4)){data=body;size=len;break;}
  pos=body+len+(len&1);
 }
 if(!data||!channels){error="missing fmt or data chunk";return false;}
 if(rate!=44100||bits!=16){error="need 16-bit 44100 Hz PCM";return false;}
 if(data+size>b.size())size=b.size()-data;
 size_t frames=size/(2*channels);out.resize(frames);
 for(size_t i=0;i<frames;i++)out[i]=int16_t(u16(data+i*2*channels));
 return true;
}
