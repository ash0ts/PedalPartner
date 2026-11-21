#pragma once
// Shared note event tracker and continuous pitch trace. Merges ISR block-energy onsets (Onset)
// with foreground YIN pitch frames into settled NoteEvents (onset time, MIDI note, settled cents,
// attack crest peak, settled bend excursion) plus a 128-frame continuous pitch trace for bends
// and vibrato. No dynamic memory; safe for Teensy 4.1, native host and WebAssembly.
#include <stdint.h>
#include <math.h>
#include "core.h"

namespace vex {

struct NoteEvent {
 uint32_t seq=0;
 uint32_t sample=0;
 uint32_t onset_ms=0;
 float onset_ms_exact=0;   // sample * 1000 / 44100: immune to foreground SPI display flush jitter
 int16_t midi=0;
 float cents=0;            // initial settled cents offset relative to `midi`
 float bend_cents=0;       // settled pitch excursion relative to initial note (for bends)
 float peak=0;             // max crest amplitude within 36 ms of attack
 float rms=0;              // max block RMS within 36 ms of attack
 bool pitched=false;
 bool legato=false;        // started from a >=1 semitone pitch step without an energy onset
};

struct PitchSample {
 uint32_t sample=0;
 float time_ms=0;
 float hz=0;
 float cents_total=0;      // continuous 6900 + 1200 * log2(hz / 440)
 int16_t midi=0;
 float cents=0;
 bool valid=false;
};

struct NoteTracker {
 static constexpr int EVENT_CAP=64;
 static constexpr int TRACE_CAP=128;
 static constexpr float SR=44100.f;
 static constexpr uint32_t CREST_SAMPLES=1600;       // ~36 ms attack crest window
 static constexpr uint32_t LEGATO_MIN_SAMPLES=3500;  // ~79 ms after onset before legato step allowed

 NoteEvent events[EVENT_CAP]{};
 uint32_t total_events=0;
 PitchSample trace[TRACE_CAP]{};
 uint32_t total_trace=0;

 bool signal_active=false;
 uint8_t quiet_blocks=0;
 uint8_t settle_frames=0;
 float base_cents_total=0;
 float prev_bend_delta=0;
 bool has_prev_bend=false;

 // Legato step detector state (distinguishes sudden fretted semitone steps from smooth string bends).
 bool prev_frame_valid=false;
 float prev_frame_cents=0;
 bool glide_seen=false;
 int16_t step_cand_midi=0;
 float step_cand_cents=0;
 uint8_t step_cand_count=0;
 float last_level_rms=0, last_level_peak=0;

 void clear() { *this = NoteTracker{}; }

 const NoteEvent& latest() const {
  static constexpr NoteEvent empty{};
  return total_events ? events[(total_events - 1) & (EVENT_CAP - 1)] : empty;
 }
 NoteEvent& latestMut() { return events[(total_events - 1) & (EVENT_CAP - 1)]; }
 const NoteEvent& atSeq(uint32_t seq) const { return events[(seq - 1) & (EVENT_CAP - 1)]; }

 void onOnset(const Onset& o, uint32_t now_ms) {
  NoteEvent ev{};
  ev.seq = ++total_events;
  ev.sample = o.sample;
  ev.onset_ms = now_ms;
  ev.onset_ms_exact = float(double(o.sample) * 1000.0 / double(SR));
  ev.rms = o.rms;
  ev.peak = o.peak;
  events[(ev.seq - 1) & (EVENT_CAP - 1)] = ev;
  settle_frames = 0;
  has_prev_bend = false;
  glide_seen = false;
  step_cand_midi = 0;
  step_cand_count = 0;
  signal_active = true;
  quiet_blocks = 0;
 }

 void onLevel(float blk_rms, float blk_peak, uint32_t sample, float gate) {
  last_level_rms = blk_rms;
  last_level_peak = blk_peak;
  if (blk_rms > gate) {
   signal_active = true;
   quiet_blocks = 0;
  } else if (blk_rms <= gate * .8f) {
   if (quiet_blocks < 255) quiet_blocks++;
   if (quiet_blocks >= 4) {
    signal_active = false;
    prev_frame_valid = false;
    step_cand_count = 0;
   }
  }
  if (total_events > 0) {
   NoteEvent& ev = latestMut();
   if (!ev.legato && sample >= ev.sample && (sample - ev.sample) <= CREST_SAMPLES) {
    if (blk_peak > ev.peak) ev.peak = blk_peak;
    if (blk_rms > ev.rms) ev.rms = blk_rms;
   }
  }
 }

 void onPitch(float hz, float prob, uint32_t win_end_sample, uint16_t win_len, bool allow_legato, uint32_t now_ms) {
  PitchSample ps{};
  ps.sample = win_end_sample;
  ps.time_ms = float(double(win_end_sample) * 1000.0 / double(SR));
  ps.hz = hz;
  Pitch p = pitch(hz, prob);
  if (p.valid) {
   ps.valid = true;
   ps.cents_total = 6900.f + 1200.f * log2f(hz / 440.f);
   ps.midi = int16_t(p.midi);
   ps.cents = p.cents;
  }
  trace[total_trace & (TRACE_CAP - 1)] = ps;
  total_trace++;

  if (!ps.valid) {
   prev_frame_valid = false;
   step_cand_count = 0;
   return;
  }

  if (total_events == 0) {
   // If audio started gently without crossing the onset ratio (e.g. fade-in), open an initial note.
   if (signal_active) {
    uint32_t est = win_end_sample > win_len ? win_end_sample - win_len : 0;
    onOnset({est, last_level_rms, last_level_peak}, now_ms);
   } else {
    prev_frame_valid = true;
    prev_frame_cents = ps.cents_total;
    return;
   }
  }

  NoteEvent& ev = latestMut();
  uint32_t need_after = uint32_t(win_len);
  if (int32_t(win_end_sample - (ev.sample + need_after)) < 0) {
   prev_frame_valid = true;
   prev_frame_cents = ps.cents_total;
   return;
  }

  if (!ev.pitched) {
   ev.midi = ps.midi;
   ev.cents = ps.cents;
   ev.pitched = true;
   base_cents_total = ps.cents_total;
   settle_frames = 1;
   prev_frame_valid = true;
   prev_frame_cents = ps.cents_total;
   return;
  }

  if (settle_frames == 1 && ps.midi == ev.midi && fabsf(ps.cents_total - base_cents_total) < 18.f) {
   base_cents_total = .5f * (base_cents_total + ps.cents_total);
   ev.cents = base_cents_total - 100.f * float(ev.midi);
   settle_frames = 2;
  }

  float delta = ps.cents_total - base_cents_total;
  float frame_jump = prev_frame_valid ? fabsf(ps.cents_total - prev_frame_cents) : 0.f;

  // Detect gradual mid-semitone glide (characteristic of string bends, not fretted hammer-ons).
  if (fabsf(delta) >= 28.f && fabsf(delta) <= 72.f && frame_jump < 55.f) glide_seen = true;

  // Legato hammer-on / pull-off: sudden >=1 semitone step to a clean fretted pitch without mid-bend glide.
  if (allow_legato && signal_active && !glide_seen &&
      int32_t(win_end_sample - ev.sample) >= int32_t(LEGATO_MIN_SAMPLES) &&
      ps.midi != ev.midi && fabsf(ps.cents) < 32.f) {
   if (step_cand_midi == ps.midi) {
    step_cand_count++;
    if (step_cand_count >= 2) {
     uint32_t step_sample = win_end_sample > win_len ? win_end_sample - win_len / 2u : win_end_sample;
     NoteEvent leg{};
     leg.seq = ++total_events;
     leg.sample = step_sample;
     leg.onset_ms = now_ms;
     leg.onset_ms_exact = float(double(step_sample) * 1000.0 / double(SR));
     leg.midi = ps.midi;
     leg.cents = .5f * (step_cand_cents + ps.cents);
     leg.peak = last_level_peak;
     leg.rms = last_level_rms;
     leg.pitched = true;
     leg.legato = true;
     events[(leg.seq - 1) & (EVENT_CAP - 1)] = leg;
     base_cents_total = 100.f * float(leg.midi) + leg.cents;
     settle_frames = 2;
     has_prev_bend = false;
     glide_seen = false;
     step_cand_count = 0;
     prev_frame_valid = true;
     prev_frame_cents = ps.cents_total;
     return;
    }
   } else {
    step_cand_midi = ps.midi;
    step_cand_cents = ps.cents;
    step_cand_count = 1;
   }
  } else {
   step_cand_count = 0;
  }

  // Settled bend excursion: two consecutive frames within 8 cents lock the plateau excursion.
  if (has_prev_bend && fabsf(delta - prev_bend_delta) < 8.f) {
   float settled = .5f * (delta + prev_bend_delta);
   if (fabsf(settled) > fabsf(ev.bend_cents)) ev.bend_cents = settled;
  }
  prev_bend_delta = delta;
  has_prev_bend = true;
  prev_frame_valid = true;
  prev_frame_cents = ps.cents_total;
 }
};

} // namespace vex
