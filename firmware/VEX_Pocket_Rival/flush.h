#pragma once
#include <stdint.h>
#include <string.h>
// Dirty-band flush for the SPI display. The renderer draws a whole 240 x 240 RGB565 frame into RAM;
// this header compares it row by row with what the panel already shows and merges the changed rows
// into a few horizontal bands. The .ino sends only those bands (setAddrWindow + writePixels), so a
// breathing sprite or a one-line caption costs a few kilobytes instead of 115 KB per cadence.
// Pure and allocation free; the same code is unit-tested on the host.
namespace vex {
struct Band{uint16_t y,h;};
constexpr int FLUSH_MAX_BANDS=8,FLUSH_GAP=4;   // rows between dirty runs that are cheaper to send than to split
constexpr int FRAME_W=240,FRAME_H=240;
// Fill `out` with at most `max` bands covering every row where prev and next differ. Returns the band
// count (0 when nothing changed). Runs closer than FLUSH_GAP rows merge; once `max` bands exist the
// last band grows to cover any further dirty rows, so the result always covers every change.
inline int bands(const uint16_t* prev,const uint16_t* next,Band* out,int max=FLUSH_MAX_BANDS){
 int n=0;
 for(int y=0;y<FRAME_H;y++){
  if(memcmp(prev+y*FRAME_W,next+y*FRAME_W,size_t(FRAME_W)*2)==0)continue;
  if(n>0&&(y-(out[n-1].y+out[n-1].h)<=FLUSH_GAP||n==max))out[n-1].h=uint16_t(y-out[n-1].y+1);
  else{out[n].y=uint16_t(y);out[n].h=1;n++;}
 }
 return n;
}
inline uint32_t bandBytes(const Band* b,int n){uint32_t total=0;for(int i=0;i<n;i++)total+=uint32_t(b[i].h)*FRAME_W*2;return total;}
}
