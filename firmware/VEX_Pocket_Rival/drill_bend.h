#pragma once
// Bend and vibrato trainer (BEND): uses the fast 1024-sample / 512-hop pitch profile (~11.6 ms).
// Tracks upward pitch bends from a settled fretted base note to a half-step (+100 c) or whole-step
// (+200 c) target, records settled cents error across rounds of 8 bends (saving median |error| in
// tenths of a cent, lower is better), and reports vibrato rate (Hz) and peak-to-peak width (cents)
// beside a typical range (5-7 Hz, 25-70 c) when holding a note with regular wobble.
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "core.h"
#include "notes.h"
#include "drill_view.h"

namespace vex {

struct BendDrill {
 static constexpr uint8_t ROUND_LEN = 8;
 int16_t target_cents = 100;   // +100 (half step) or +200 (whole step)

 uint32_t cur_event_seq = 0;
 uint32_t checked_trace = 0;
 bool base_ready = false;
 uint8_t base_frames = 0;
 float base_cents = 0.f;
 float live_delta = 0.f;
 float peak_delta = 0.f;
 float apex_sum = 0.f;
 uint8_t apex_frames = 0;
 bool note_bent = false;

 // Last 8 settled bend errors in cents (signed: negative = flat, positive = sharp).
 int16_t hist[ROUND_LEN]{};
 uint8_t hist_count = 0;
 uint8_t round_count = 0;
 float round_abs_err[ROUND_LEN]{};
 int16_t last_err_cents = 0;
 uint16_t last_med_tenths = 0;
 uint32_t best_at = 0;
 uint32_t wrong_at = 0;

 // Recent valid trace ring for the active note to measure vibrato rate & width.
 static constexpr uint8_t VIB_CAP = 64;
 float vib_t_ms[VIB_CAP]{};
 float vib_cents[VIB_CAP]{};
 uint8_t vib_len = 0;
 bool vib_active = false;
 float vib_hz = 0.f;
 float vib_width_c = 0.f;
 uint32_t vib_seen_at = 0;

 static float medianAbs8(const float* v, int n) {
  if (n <= 0) return 0.f;
  float tmp[ROUND_LEN];
  for (int i = 0; i < n; i++) tmp[i] = v[i];
  for (int i = 1; i < n; i++) {
   float x = tmp[i];
   int j = i - 1;
   while (j >= 0 && tmp[j] > x) { tmp[j + 1] = tmp[j]; j--; }
   tmp[j + 1] = x;
  }
  return (n & 1) ? tmp[n / 2] : .5f * (tmp[n / 2 - 1] + tmp[n / 2]);
 }

 void resetNoteState() {
  base_ready = false;
  base_frames = 0;
  base_cents = 0.f;
  live_delta = 0.f;
  peak_delta = 0.f;
  apex_sum = 0.f;
  apex_frames = 0;
  note_bent = false;
  vib_len = 0;
 }

 void configure(uint8_t semitones) {
  target_cents = semitones >= 2 ? 200 : 100;
  hist_count = 0;
  round_count = 0;
  last_err_cents = 0;
  last_med_tenths = 0;
  best_at = 0;
  wrong_at = 0;
  vib_active = false;
  vib_hz = 0.f;
  vib_width_c = 0.f;
  vib_seen_at = 0;
  resetNoteState();
 }

 void start(uint32_t /*now_ms*/, uint32_t event_seq, uint32_t trace_seq) {
  cur_event_seq = event_seq;
  checked_trace = trace_seq;
  round_count = 0;
  hist_count = 0;
  wrong_at = 0;
  vib_active = false;
  resetNoteState();
 }

 bool commitBend(float settled_cents, uint32_t now_ms, SkillStat& st, bool& newBest) {
  note_bent = true;
  vib_active = false;
  float err = settled_cents - float(target_cents);
  int16_t err_i = int16_t(lroundf(err));
  last_err_cents = err_i;
  if (fabsf(err) > 15.f) wrong_at = now_ms;
  else wrong_at = 0;

  if (hist_count < ROUND_LEN) {
   hist[hist_count++] = err_i;
  } else {
   for (uint8_t i = 1; i < ROUND_LEN; i++) hist[i - 1] = hist[i];
   hist[ROUND_LEN - 1] = err_i;
  }
  round_abs_err[round_count++] = fabsf(err);
  if (round_count >= ROUND_LEN) {
   float med = medianAbs8(round_abs_err, ROUND_LEN);
   int tenths = int(lroundf(med * 10.f));
   if (tenths < 1) tenths = 1;
   if (tenths > 9999) tenths = 9999;
   last_med_tenths = uint16_t(tenths);
   st.last = last_med_tenths;
   if (st.rounds < 65535) st.rounds++;
   round_count = 0;
   if (st.best == 0 || last_med_tenths < st.best) {
    st.best = last_med_tenths;
    best_at = now_ms;
    newBest = true;
   }
   return true;
  }
  return false;
 }

 void analyzeVibrato(uint32_t now_ms) {
  if (note_bent || vib_len < 24) return;
  float sum = 0.f;
  for (uint8_t i = 0; i < vib_len; i++) sum += vib_cents[i];
  float mean = sum / float(vib_len);
  float sq = 0.f, max_abs = 0.f;
  for (uint8_t i = 0; i < vib_len; i++) {
   float d = vib_cents[i] - mean;
   sq += d * d;
   if (fabsf(d) > max_abs) max_abs = fabsf(d);
  }
  // Ignore unbent flat holds (<10c p-p) and big bends (>65% of target).
  float width_pp = 2.828427f * sqrtf(sq / float(vib_len));
  if (width_pp < 12.f || fabsf(mean - base_cents) > float(target_cents) * .55f || max_abs > 110.f) return;

  // Hysteretic upward zero-crossings around `mean` to measure vibrato rate & regularity.
  float cross_ms[20]{};
  int n_cross = 0;
  bool armed = false;
  for (uint8_t i = 1; i < vib_len && n_cross < 20; i++) {
   float y0 = vib_cents[i - 1] - mean;
   float y1 = vib_cents[i] - mean;
   if (y0 < -3.f) armed = true;
   if (armed && y1 >= 0.f && y1 > y0) {
    float frac = (0.f - y0) / (y1 - y0);
    cross_ms[n_cross++] = vib_t_ms[i - 1] + frac * (vib_t_ms[i] - vib_t_ms[i - 1]);
    armed = false;
   }
  }
  if (n_cross < 3) return;
  int n_per = n_cross - 1;
  float p_sum = 0.f;
  for (int i = 0; i < n_per; i++) p_sum += (cross_ms[i + 1] - cross_ms[i]);
  float p_mean = p_sum / float(n_per);
  if (p_mean < 95.f || p_mean > 340.f) return; // 2.9 .. 10.5 Hz
  float max_rel_dev = 0.f;
  for (int i = 0; i < n_per; i++) {
   float p = cross_ms[i + 1] - cross_ms[i];
   float dev = fabsf(p - p_mean) / p_mean;
   if (dev > max_rel_dev) max_rel_dev = dev;
  }
  if (max_rel_dev > .28f) return; // irregular wobble, not steady vibrato
  vib_hz = 1000.f / p_mean;
  vib_width_c = width_pp;
  vib_active = true;
  vib_seen_at = now_ms;
 }

 bool update(const NoteTracker& nt, uint32_t now_ms, SkillStat& st, bool& newBest) {
  newBest = false;
  bool dirty = false;

  if (nt.total_events > cur_event_seq) {
   // If the player released or re-picked after bending past 45% without holding 5 apex frames,
   // still score the bend they reached so quick bends are counted.
   if (base_ready && !note_bent && peak_delta >= float(target_cents) * .45f && apex_frames >= 2) {
    if (commitBend(apex_sum / float(apex_frames), now_ms, st, newBest)) dirty = true;
   }
   cur_event_seq = nt.total_events;
   resetNoteState();
  }

  if (nt.total_trace - checked_trace > NoteTracker::TRACE_CAP) {
   checked_trace = nt.total_trace - NoteTracker::TRACE_CAP;
  }

  const NoteEvent& ev = nt.latest();
  while (checked_trace < nt.total_trace) {
   const PitchSample& ps = nt.trace[checked_trace & (NoteTracker::TRACE_CAP - 1)];
   checked_trace++;
   if (!ps.valid || cur_event_seq == 0) continue;
   if (int32_t(ps.sample - (ev.sample + 1024u)) < 0) continue;

   if (!base_ready) {
    base_cents = ps.cents_total;
    base_ready = true;
    base_frames = 1;
    live_delta = 0.f;
    continue;
   }
   if (base_frames == 1 && fabsf(ps.cents_total - base_cents) < 16.f) {
    base_cents = .5f * (base_cents + ps.cents_total);
    base_frames = 2;
   }

   float delta = ps.cents_total - base_cents;
   live_delta = delta;

   if (vib_len < VIB_CAP) {
    vib_t_ms[vib_len] = ps.time_ms;
    vib_cents[vib_len] = ps.cents_total;
    vib_len++;
   } else {
    for (uint8_t i = 1; i < VIB_CAP; i++) { vib_t_ms[i - 1] = vib_t_ms[i]; vib_cents[i - 1] = vib_cents[i]; }
    vib_t_ms[VIB_CAP - 1] = ps.time_ms;
    vib_cents[VIB_CAP - 1] = ps.cents_total;
   }

   float min_bend = float(target_cents) * .45f;
   if (!note_bent && delta >= min_bend) {
    if (delta > peak_delta + 1.5f) {
     peak_delta = delta;
     apex_sum = delta;
     apex_frames = 1;
    } else if (fabsf(delta - peak_delta) <= 4.5f) {
     apex_sum += delta;
     apex_frames++;
     if (apex_frames >= 5) {
      if (commitBend(apex_sum / float(apex_frames), now_ms, st, newBest)) dirty = true;
     }
    }
   } else if (!note_bent) {
    analyzeVibrato(now_ms);
   }
  }

  // Also finalize a partial apex hold if the string goes silent after bending.
  if (base_ready && !note_bent && !nt.signal_active && peak_delta >= float(target_cents) * .45f && apex_frames >= 2) {
   if (commitBend(apex_sum / float(apex_frames), now_ms, st, newBest)) dirty = true;
  }
  return dirty;
 }

 void fillView(DrillView& v, const char* title, const SkillStat& st, bool running, uint32_t now_ms) const {
  v.viz = VIZ_BEND;
  snprintf(v.title, sizeof v.title, "%s", title);
  if (st.best > 0) {
   snprintf(v.subtitle, sizeof v.subtitle, "+%dC  BEST %u.%uC",
            int(target_cents), unsigned(st.best / 10), unsigned(st.best % 10));
  } else {
   snprintf(v.subtitle, sizeof v.subtitle, "TARGET +%dC  %u/%u",
            int(target_cents), unsigned(round_count), unsigned(ROUND_LEN));
  }
  v.target_cents = target_cents;
  int lc = int(lroundf(live_delta));
  if (lc < -50) lc = -50;
  if (lc > 300) lc = 300;
  v.live_cents = int16_t(lc);
  v.bend_count = hist_count;
  for (uint8_t i = 0; i < hist_count && i < ROUND_LEN; i++) v.bend_hist[i] = hist[i];
  v.ring_lit = uint8_t((uint32_t(round_count) * 60u) / ROUND_LEN);
  v.new_best = best_at > 0 && (now_ms - best_at < 2400u);
  v.wrong_flash = running && wrong_at > 0 && (now_ms - wrong_at < 900u);

  bool show_vib = vib_active && (now_ms - vib_seen_at < 1800u);
  if (show_vib) {
   snprintf(v.mid, sizeof v.mid, "VIB %.1fHZ  %dC", double(vib_hz), int(lroundf(vib_width_c)));
   snprintf(v.detail, sizeof v.detail, "TYP 5-7HZ  25-70C");
  } else if (hist_count > 0) {
   snprintf(v.mid, sizeof v.mid, "LAST %+d CENTS", int(last_err_cents));
   if (last_med_tenths > 0 && round_count == 0) {
    snprintf(v.detail, sizeof v.detail, "MEDIAN %u.%uC  8 BENDS",
             unsigned(last_med_tenths / 10), unsigned(last_med_tenths % 10));
   } else {
    snprintf(v.detail, sizeof v.detail, "BEND %u/%u  HOLD TOP",
             unsigned(round_count ? round_count : ROUND_LEN), unsigned(ROUND_LEN));
   }
  } else {
   snprintf(v.mid, sizeof v.mid, "+%dC TARGET", int(target_cents));
   snprintf(v.detail, sizeof v.detail, running ? "FRET NOTE THEN BEND" : "TAP FOOT TO START");
  }
 }
};

} // namespace vex
