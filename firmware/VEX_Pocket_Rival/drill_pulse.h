#pragma once
// Timing trainer (PULSE): scores note onsets against the LED beat subdivision AND against your own
// running spacing (so an external metronome works too). Four clean bars raise BPM by 4; two failed
// 4-bar tries lower BPM by 4. Starts 8 BPM under saved personal best.
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "core.h"
#include "notes.h"
#include "drill_view.h"

namespace vex {

struct PulseDrill {
 static constexpr float PASS_MS = 30.f;
 uint8_t subdiv = 2;           // 1 = quarter, 2 = eighth, 0 = free self-paced
 uint16_t bpm = 92;
 uint32_t beat_origin_ms = 0;
 uint32_t last_seq = 0;
 float prev_onset_ms = -1.f;
 float grid_phase_ms = 0.f;
 bool phase_locked = false;

 float errs[32]{};
 float iois[32]{};
 uint8_t round_notes = 0;
 uint8_t ioi_count = 0;
 int8_t dots[12]{};
 uint8_t dot_count = 0;

 float bias_ms = 0.f;
 float spread_ms = 0.f;
 float inferred_bpm = 0.f;
 uint8_t fail_streak = 0;
 uint32_t best_at = 0;
 bool last_passed = false, has_round_result = false;

 void configure(uint8_t sub, const SkillStat& st) {
  subdiv = sub;
  bpm = st.best > 0 ? uint16_t(st.best > 68 ? st.best - 8 : 60) : 92;
  resetRound(0);
  best_at = 0;
  fail_streak = 0;
  has_round_result = false;
 }

 void resetRound(uint32_t now_ms) {
  beat_origin_ms = now_ms;
  prev_onset_ms = -1.f;
  phase_locked = false;
  round_notes = 0;
  ioi_count = 0;
  dot_count = 0;
  bias_ms = 0.f;
  spread_ms = 0.f;
 }

 void start(uint32_t now_ms, uint32_t cur_event_seq) {
  last_seq = cur_event_seq;
  resetRound(now_ms);
 }

 uint32_t beatPeriodMs() const {
  return (subdiv > 0 && bpm > 0) ? uint32_t(lroundf(60000.f / float(bpm))) : 0u;
 }

 uint8_t targetNotes() const {
  return subdiv == 0 ? 16 : uint8_t(16 * subdiv); // 4 bars of 4/4 (16 qtr or 32 8th)
 }

 static float medianOf(const float* src, int n) {
  if (n <= 0) return 0.f;
  float tmp[32];
  if (n > 32) n = 32;
  for (int i = 0; i < n; i++) tmp[i] = src[i];
  for (int i = 1; i < n; i++) {
   float v = tmp[i]; int j = i - 1;
   while (j >= 0 && tmp[j] > v) { tmp[j + 1] = tmp[j]; j--; }
   tmp[j + 1] = v;
  }
  return (n & 1) ? tmp[n / 2] : .5f * (tmp[n / 2 - 1] + tmp[n / 2]);
 }

 // Returns true if `st` changed (round completed; `newBest` set when personal best improved).
 bool update(const NoteTracker& nt, uint32_t now_ms, SkillStat& st, bool& newBest) {
  newBest = false;
  bool dirty = false;
  while (last_seq < nt.total_events) {
   last_seq++;
   if (nt.total_events - last_seq >= NoteTracker::EVENT_CAP) continue;
   const NoteEvent& ev = nt.atSeq(last_seq);
   if (ev.legato) continue; // PULSE scores picked attacks explicitly

   float t = ev.onset_ms_exact;
   float grid_ms = subdiv > 0 ? 60000.f / float(bpm * subdiv) : 0.f;

   if (prev_onset_ms >= 0.f) {
    float dt = t - prev_onset_ms;
    if (dt >= 55.f && dt <= 2500.f && ioi_count < 32) iois[ioi_count++] = dt;
   }
   prev_onset_ms = t;

   float err = 0.f;
   if (subdiv > 0) {
    if (!phase_locked) {
     // Anchor beat phase on first picked note so both LED-aligned and external-click takes work cleanly,
     // while measuring every subsequent note's rush/drag and jitter against the exact tempo grid!
     grid_phase_ms = t;
     phase_locked = true;
     err = 0.f;
    } else {
     float rel = t - grid_phase_ms;
     float nearest = roundf(rel / grid_ms) * grid_ms;
     err = rel - nearest;
    }
   } else {
    if (ioi_count >= 2) {
     float med = medianOf(iois, ioi_count);
     inferred_bpm = med > 0.f ? 60000.f / med : 0.f;
     err = iois[ioi_count - 1] - med;
    } else if (ioi_count == 1) {
     inferred_bpm = 60000.f / iois[0];
    }
   }

   if (round_notes < 32) errs[round_notes++] = err;
   int clamped = int(lroundf(err));
   if (clamped < -60) clamped = -60;
   if (clamped > 60) clamped = 60;
   if (dot_count < 12) dots[dot_count++] = int8_t(clamped);
   else {
    for (int i = 0; i < 11; i++) dots[i] = dots[i + 1];
    dots[11] = int8_t(clamped);
   }

   // Recompute running bias (mean signed offset after anchor) and spread (mean absolute deviation).
   if (round_notes >= 2) {
    float sum = 0.f;
    int cnt = 0;
    for (int i = 1; i < round_notes; i++) { sum += errs[i]; cnt++; }
    bias_ms = cnt ? sum / float(cnt) : 0.f;
    float mad = 0.f;
    for (int i = 1; i < round_notes; i++) mad += fabsf(errs[i] - bias_ms);
    spread_ms = cnt ? mad / float(cnt) : 0.f;
   }

   uint8_t goal = targetNotes();
   if (round_notes >= goal) {
    bool clean = spread_ms <= PASS_MS && fabsf(bias_ms) <= PASS_MS;
    uint16_t eff_bpm = subdiv > 0 ? bpm : uint16_t(lroundf(inferred_bpm));
    if (st.rounds < 65535) st.rounds++;
    st.last = eff_bpm;
    dirty = true;
    has_round_result = true;
    last_passed = clean;
    if (clean && eff_bpm > 0) {
     fail_streak = 0;
     if (eff_bpm > st.best) {
      st.best = eff_bpm;
      best_at = now_ms;
      newBest = true;
     }
     if (subdiv > 0 && bpm + 4 <= 220) bpm = uint16_t(bpm + 4);
    } else if (subdiv > 0) {
     fail_streak++;
     if (fail_streak >= 2) {
      bpm = bpm > 56 ? uint16_t(bpm - 4) : 52;
      fail_streak = 0;
     }
    }
    // Start next 4-bar ladder step cleanly while keeping recent dots visible until next note.
    round_notes = 0;
    ioi_count = 0;
    phase_locked = false;
    prev_onset_ms = -1.f;
   }
  }
  return dirty;
 }

 void fillView(DrillView& v, const char* title, const SkillStat& st, bool running, uint32_t now_ms) const {
  v.viz = VIZ_TIMING;
  snprintf(v.title, sizeof v.title, "%s", title);
  uint16_t shown_bpm = subdiv > 0 ? bpm : uint16_t(lroundf(inferred_bpm));
  if (st.best > 0) snprintf(v.subtitle, sizeof v.subtitle, "%u BPM  BEST %u", shown_bpm, st.best);
  else if (shown_bpm > 0) snprintf(v.subtitle, sizeof v.subtitle, "%u BPM  4-BAR LADDER", shown_bpm);
  else snprintf(v.subtitle, sizeof v.subtitle, "YOUR TEMPO  16 NOTES");

  v.dot_count = dot_count;
  v.pass_ms = int8_t(PASS_MS);
  for (uint8_t i = 0; i < dot_count; i++) v.dots[i] = dots[i];

  int b = int(lroundf(bias_ms));
  if (dot_count < 2) {
   snprintf(v.mid, sizeof v.mid, running ? "PICK STEADY NOTES" : "READY");
  } else if (b <= -4) {
   snprintf(v.mid, sizeof v.mid, "RUSHING %d MS", -b);
  } else if (b >= 4) {
   snprintf(v.mid, sizeof v.mid, "LATE %d MS", b);
  } else {
   snprintf(v.mid, sizeof v.mid, "LOCKED %+d MS", b);
  }

  uint8_t goal = targetNotes();
  v.ring_lit = uint8_t((uint32_t(round_notes) * 60u) / (goal ? goal : 16u));
  v.new_best = best_at > 0 && (now_ms - best_at < 2400u);
  if (has_round_result && round_notes == 0) {
   if (last_passed) snprintf(v.detail, sizeof v.detail, "CLEAN! UP TO %u BPM", unsigned(bpm));
   else snprintf(v.detail, sizeof v.detail, "SPREAD %dMS TRY AGAIN", int(lroundf(spread_ms)));
  } else {
   snprintf(v.detail, sizeof v.detail, "SPREAD %dMS  %u/%u", int(lroundf(spread_ms)), round_notes, goal);
  }
 }
};

} // namespace vex
