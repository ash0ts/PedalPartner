#pragma once
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include "core.h"
#ifndef VEX_AUDIO_LOCK
#define VEX_AUDIO_LOCK() ((void)0)
#endif
#ifndef VEX_AUDIO_UNLOCK
#define VEX_AUDIO_UNLOCK() ((void)0)
#endif
namespace vex {
// Shared level meter, block-energy onset detector and YIN pitch estimator for 44.1 kHz 16-bit mono blocks.
// push() is the only call allowed from the audio interrupt: it accumulates level, runs O(1) block-energy
// onset detection into a fixed ring, and writes samples into a 4096-sample ring buffer.
// Foreground calls guard shared rings with VEX_AUDIO_LOCK/UNLOCK (AudioNoInterrupts on Teensy).
struct Analyzer {
 static constexpr int RATE=44100, BLOCK=128, MAX_WINDOW=2048, WINDOW=2048;
 static constexpr int RING_SIZE=4096, RING_MASK=RING_SIZE-1;
 static constexpr int TAU_MIN=RATE/1400, TAU_MAX=RATE/55+1; // 31, 802
 static constexpr int ONSET_CAP=32;
 static constexpr uint32_t ONSET_REFRACT_SAMPLES=2425;      // ~55 ms refractory gap
 static constexpr float ONSET_RATIO=1.40f;
 static constexpr float THRESHOLD=.15f, SILENCE_RMS=1e-4f;

 struct Level { float sum_sq=0, peak=0; uint32_t count=0; };
 Level level[2];
 volatile uint8_t level_slot=0;

 int16_t ring[RING_SIZE]{};
 volatile uint32_t sample_count=0;
 uint32_t next_pitch_sample=MAX_WINDOW;
 PitchMode last_mode=PITCH_TUNER;

 // O(1) ISR onset state: 4-block (~11.6 ms, >1 period of E2) mean-square boxcar vs trailing envelope.
 float blk_ms[4]{};
 uint8_t blk_idx=0;
 float slow_rms=0;
 volatile float onset_floor=.006f;
 bool onset_armed=true;
 uint32_t last_onset_sample=0;
 bool has_onset=false;
 Onset onsets[ONSET_CAP]{};
 volatile uint8_t onset_write=0;
 uint8_t onset_read=0;

 float work[MAX_WINDOW]{}, diff[TAU_MAX+1]{};
 float hz=0, probability=0, window_rms=0;
 uint32_t pitch_sample=0;

 void setGate(float gate) {
  float floor = gate * 1.5f;
  if (floor < .002f) floor = .002f;
  onset_floor = floor;
 }

 void push(const int16_t* samples, int n) {
  if (n <= 0) return;
  Level& l = level[level_slot];
  float b_sq = 0, b_pk = 0;
  uint32_t sc = sample_count;
  for (int i = 0; i < n; i++) {
   int16_t s = samples[i];
   ring[(sc + uint32_t(i)) & RING_MASK] = s;
   float v = s * (1.f / 32768.f), sq = v * v, a = fabsf(v);
   b_sq += sq;
   if (a > b_pk) b_pk = a;
  }
  l.sum_sq += b_sq;
  if (b_pk > l.peak) l.peak = b_pk;
  l.count += uint32_t(n);
  uint32_t end_sc = sc + uint32_t(n);
  sample_count = end_sc;

  // 4-block (~11.6 ms) RMS smooths intra-cycle beating on low E2 while keeping 1-block (2.9 ms) step response.
  blk_ms[blk_idx] = b_sq / float(n);
  blk_idx = (blk_idx + 1) & 3;
  float fast = sqrtf(.25f * (blk_ms[0] + blk_ms[1] + blk_ms[2] + blk_ms[3]));
  float floor = onset_floor;
  if (fast <= slow_rms * 1.12f || fast < floor) onset_armed = true;
  bool refract_ok = !has_onset || uint32_t(sc - last_onset_sample) >= ONSET_REFRACT_SAMPLES;
  if (onset_armed && refract_ok && fast > floor && fast > slow_rms * ONSET_RATIO && (fast - slow_rms) > floor * .45f) {
   uint8_t w = onset_write;
   onsets[w & (ONSET_CAP - 1)] = {sc, fast, b_pk};
   onset_write = uint8_t(w + 1);
   last_onset_sample = sc;
   has_onset = true;
   onset_armed = false;
   slow_rms = fast;
  }
  if (!onset_armed && fast > slow_rms) slow_rms = fast;
  else if (fast < slow_rms) slow_rms = fast + .85f * (slow_rms - fast);
  else slow_rms = slow_rms + .04f * (fast - slow_rms);
 }

 // Foreground. True when at least one block arrived since the previous call.
 bool takeLevel(float& rms, float& peak, uint32_t* out_sample=nullptr) {
  VEX_AUDIO_LOCK();
  uint8_t s = level_slot;
  Level snap = level[s];
  uint32_t sc = sample_count;
  if (snap.count) { level[s] = Level{}; level_slot = s ^ 1; }
  VEX_AUDIO_UNLOCK();
  if (!snap.count) return false;
  rms = sqrtf(snap.sum_sq / snap.count);
  peak = snap.peak;
  if (out_sample) *out_sample = sc;
  return true;
 }

 // Foreground. Pop one queued onset under lock; if the ring overflowed, drop oldest first.
 bool takeOnset(Onset& out) {
  VEX_AUDIO_LOCK();
  uint8_t w = onset_write;
  if (uint8_t(w - onset_read) > ONSET_CAP) onset_read = uint8_t(w - ONSET_CAP);
  if (onset_read == w) { VEX_AUDIO_UNLOCK(); return false; }
  out = onsets[onset_read & (ONSET_CAP - 1)];
  onset_read = uint8_t(onset_read + 1);
  VEX_AUDIO_UNLOCK();
  return true;
 }

 // Foreground when pitch is not displayed: advance past current samples so stale windows do not linger.
 void skip() {
  VEX_AUDIO_LOCK();
  uint32_t sc = sample_count;
  VEX_AUDIO_UNLOCK();
  next_pitch_sample = sc + MAX_WINDOW;
  last_mode = PITCH_OFF;
  hz = 0;
  probability = 0;
 }

 // Foreground YIN on the latest window matching `profile`. Defaults to PITCH_TUNER (2048/2048, 55..1400 Hz).
 bool analyze(PitchMode mode=PITCH_TUNER, bool slowRunFallback=false) {
  PitchProfile prof = pitchProfile(mode, slowRunFallback);
  if (!prof.window) { skip(); return false; }
  VEX_AUDIO_LOCK();
  uint32_t sc = sample_count;
  if (mode != last_mode) {
   uint32_t nextGrid = sc - (sc % prof.hop) + prof.hop;
   if (nextGrid < prof.window) nextGrid = prof.window;
   next_pitch_sample = nextGrid;
   last_mode = mode;
  }
  if (int32_t(sc - next_pitch_sample) < 0 || int32_t(sc - prof.window) < 0) {
   VEX_AUDIO_UNLOCK();
   return false;
  }
  uint32_t win_end = sc;
  uint32_t start = win_end - prof.window;
  for (uint16_t i = 0; i < prof.window; i++) {
   work[i] = ring[(start + i) & RING_MASK] * (1.f / 32768.f);
  }
  next_pitch_sample = win_end + prof.hop;
  VEX_AUDIO_UNLOCK();

  pitch_sample = win_end;
  int W = prof.window, tmin = prof.tau_min, tmax = prof.tau_max, span = W - tmax;
  float energy = 0;
  for (int i = 0; i < W; i++) energy += work[i] * work[i];
  window_rms = sqrtf(energy / W);
  if (window_rms < SILENCE_RMS) { hz = 0; probability = 0; return true; }
  for (int tau = 1; tau <= tmax; tau++) {
   float s = 0;
   const float* a = work;
   const float* b = work + tau;
   for (int j = 0; j < span; j++) { float d = a[j] - b[j]; s += d * d; }
   diff[tau] = s;
  }
  float running = 0;
  diff[0] = 1;
  for (int tau = 1; tau <= tmax; tau++) {
   running += diff[tau];
   diff[tau] = running > 0 ? diff[tau] * tau / running : 1;
  }
  int best = tmin;
  bool found = false;
  for (int tau = tmin; tau <= tmax && !found; tau++) {
   if (diff[tau] < THRESHOLD) {
    while (tau + 1 <= tmax && diff[tau + 1] < diff[tau]) tau++;
    best = tau;
    found = true;
   } else if (diff[tau] < diff[best]) {
    best = tau;
   }
  }
  float t = float(best);
  if (best > tmin && best < tmax) {
   float a = diff[best - 1], b = diff[best], c = diff[best + 1], den = a - 2 * b + c;
   if (den > 0) t = best + (a - c) / (2 * den);
  }
  hz = RATE / t;
  probability = 1 - diff[best];
  if (probability < 0) probability = 0;
  if (probability > 1) probability = 1;
  return true;
 }
};
}
