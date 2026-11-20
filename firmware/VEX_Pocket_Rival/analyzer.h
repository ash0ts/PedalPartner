#pragma once
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#ifndef VEX_AUDIO_LOCK
#define VEX_AUDIO_LOCK() ((void)0)
#endif
#ifndef VEX_AUDIO_UNLOCK
#define VEX_AUDIO_UNLOCK() ((void)0)
#endif
namespace vex {
// Shared level meter and pitch estimator for 44.1 kHz 16-bit mono blocks.
// push() is the only call allowed from the audio interrupt: it accumulates level and copies samples
// into a double-buffered window. No allocation, logging or display work happens here.
// Foreground calls guard the slot swap and window copy with VEX_AUDIO_LOCK/UNLOCK (AudioNoInterrupts
// on the Teensy, no-op on single-threaded hosts). analyze() runs YIN in the foreground on the last
// completed window. Levels use the same 0..1 scale as the PJRC RMS/peak analyzers.
struct Analyzer {
 static constexpr int RATE=44100,BLOCK=128,WINDOW=2048;
 static constexpr int TAU_MIN=RATE/1400,TAU_MAX=RATE/55+1,SPAN=WINDOW-TAU_MAX; // 31, 802, 1246
 static constexpr float THRESHOLD=.15f,SILENCE_RMS=1e-4f;
 struct Level{float sum_sq=0,peak=0;uint32_t count=0;};
 Level level[2];volatile uint8_t level_slot=0;
 int16_t buffer[2][WINDOW];volatile uint8_t fill_slot=0;int fill=0;volatile uint32_t windows=0;uint32_t analyzed=0;
 float work[WINDOW],diff[TAU_MAX+1];
 float hz=0,probability=0,window_rms=0;
 void push(const int16_t* samples,int n){
  Level& l=level[level_slot];
  for(int i=0;i<n;i++){float v=samples[i]*(1.f/32768.f);l.sum_sq+=v*v;float a=fabsf(v);if(a>l.peak)l.peak=a;}
  l.count+=n;
  int16_t* dst=buffer[fill_slot];
  for(int i=0;i<n;i++){dst[fill++]=samples[i];if(fill==WINDOW){fill=0;fill_slot^=1;windows++;dst=buffer[fill_slot];}}
 }
 // Foreground. True when at least one block arrived since the previous call; returns RMS and peak
 // accumulated over those blocks so a slow loop (display transfer) cannot hide a clipped block.
 bool takeLevel(float& rms,float& peak){
  VEX_AUDIO_LOCK();
  uint8_t s=level_slot;Level snap=level[s];
  if(snap.count){level[s]=Level{};level_slot=s^1;}
  VEX_AUDIO_UNLOCK();
  if(!snap.count)return false;
  rms=sqrtf(snap.sum_sq/snap.count);peak=snap.peak;return true;
 }
 // Foreground when pitch is not displayed: mark completed windows seen so stale readings do not linger.
 void skip(){VEX_AUDIO_LOCK();analyzed=windows;VEX_AUDIO_UNLOCK();hz=0;probability=0;}
 // Foreground. True when a newly completed window was analysed; hz/probability then hold the result
 // (hz=0, probability=0 for silence or no periodic candidate).
 bool analyze(){
  VEX_AUDIO_LOCK();
  if(analyzed==windows){VEX_AUDIO_UNLOCK();return false;}
  analyzed=windows;const int16_t* src=buffer[fill_slot^1];
  for(int i=0;i<WINDOW;i++)work[i]=src[i]*(1.f/32768.f);
  VEX_AUDIO_UNLOCK();
  float energy=0;for(int i=0;i<WINDOW;i++)energy+=work[i]*work[i];
  window_rms=sqrtf(energy/WINDOW);
  if(window_rms<SILENCE_RMS){hz=0;probability=0;return true;}
  for(int tau=1;tau<=TAU_MAX;tau++){float s=0;const float* a=work;const float* b=work+tau;for(int j=0;j<SPAN;j++){float d=a[j]-b[j];s+=d*d;}diff[tau]=s;}
  float running=0;diff[0]=1;
  for(int tau=1;tau<=TAU_MAX;tau++){running+=diff[tau];diff[tau]=running>0?diff[tau]*tau/running:1;}
  int best=TAU_MIN;bool found=false;
  for(int tau=TAU_MIN;tau<=TAU_MAX&&!found;tau++){
   if(diff[tau]<THRESHOLD){while(tau+1<=TAU_MAX&&diff[tau+1]<diff[tau])tau++;best=tau;found=true;}
   else if(diff[tau]<diff[best])best=tau;
  }
  float t=best;
  if(best>TAU_MIN&&best<TAU_MAX){float a=diff[best-1],b=diff[best],c=diff[best+1],den=a-2*b+c;if(den>0)t=best+(a-c)/(2*den);}
  hz=RATE/t;probability=1-diff[best];if(probability<0)probability=0;if(probability>1)probability=1;
  return true;
 }
};
}
