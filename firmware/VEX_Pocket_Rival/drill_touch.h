#pragma once
// Picking dynamics trainer (TOUCH): runs with pitch gated off (PITCH_OFF). Draws per-note attack
// level bars in dB relative to your running median. Even mode scores dB spread across 12 notes
// (lower is better); accent presets (ACC3, ACC4) verify every 3rd or 4th note is >= +4.0 dB above
// the unaccented median and save best spread in hundredths of a dB (lower is better).
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "core.h"
#include "notes.h"
#include "drill_view.h"

namespace vex {

struct TouchDrill {
 static constexpr uint8_t ROUND_LEN = 12;
 static constexpr float ACCENT_MIN_DB = 4.0f;
 static constexpr float EVEN_PASS_DB = 2.2f;

 uint8_t accent_every = 0;     // 0 = even dynamics, 3 = every 3rd note, 4 = every 4th note
 float note_db[ROUND_LEN]{};
 uint8_t count = 0;

 int8_t bars[ROUND_LEN]{};     // half-dB units relative to median (-24..+24)
 uint16_t bar_acc_mask = 0;
 uint8_t bar_count = 0;

 float last_spread_db = 0.f;
 uint8_t last_acc_hits = 0;
 uint8_t last_acc_need = 0;
 bool round_done = false;
 bool round_clean = false;

 uint32_t checked_seq = 0;
 float prev_onset_ms = 0.f;
 uint32_t best_at = 0;
 uint32_t wrong_at = 0;

 static float medianFloat(const float* src, int n) {
  if (n <= 0) return -30.f;
  float tmp[ROUND_LEN];
  for (int i = 0; i < n; i++) tmp[i] = src[i];
  for (int i = 1; i < n; i++) {
   float x = tmp[i];
   int j = i - 1;
   while (j >= 0 && tmp[j] > x) { tmp[j + 1] = tmp[j]; j--; }
   tmp[j + 1] = x;
  }
  return (n & 1) ? tmp[n / 2] : .5f * (tmp[n / 2 - 1] + tmp[n / 2]);
 }

 void configure(uint8_t group) {
  accent_every = (group == 3 || group == 4) ? group : 0;
  count = 0;
  bar_count = 0;
  bar_acc_mask = 0;
  last_spread_db = 0.f;
  last_acc_hits = 0;
  last_acc_need = 0;
  round_done = false;
  round_clean = false;
  best_at = 0;
  wrong_at = 0;
 }

 void start(uint32_t /*now_ms*/, uint32_t cur_event_seq) {
  checked_seq = cur_event_seq;
  prev_onset_ms = 0.f;
  count = 0;
  bar_count = 0;
  bar_acc_mask = 0;
  round_done = false;
  wrong_at = 0;
 }

 void refreshBars(uint8_t n) {
  if (n == 0) { bar_count = 0; bar_acc_mask = 0; return; }
  float unacc[ROUND_LEN];
  int n_unacc = 0;
  if (accent_every > 0) {
   for (uint8_t i = 0; i < n; i++) {
    if ((i % accent_every) != 0) unacc[n_unacc++] = note_db[i];
   }
  }
  float ref_db = (accent_every > 0 && n_unacc >= 2) ? medianFloat(unacc, n_unacc) : medianFloat(note_db, n);
  bar_count = n;
  bar_acc_mask = 0;
  for (uint8_t i = 0; i < n; i++) {
   int h2 = int(lroundf((note_db[i] - ref_db) * 2.f));
   if (h2 < -24) h2 = -24;
   if (h2 > 24) h2 = 24;
   bars[i] = int8_t(h2);
   if (accent_every > 0 && (i % accent_every) == 0) bar_acc_mask |= uint16_t(1u << i);
  }
 }

 bool update(const NoteTracker& nt, uint32_t now_ms, SkillStat& st, bool& newBest) {
  newBest = false;
  bool dirty = false;
  while (checked_seq < nt.total_events) {
   uint32_t next_seq = checked_seq + 1;
   if (nt.total_events - next_seq >= NoteTracker::EVENT_CAP) { checked_seq = next_seq; continue; }
   const NoteEvent& ev = nt.atSeq(next_seq);
   // Wait ~38 ms for NoteTracker's 36 ms attack crest window to finish accumulating `ev.peak`.
   if (next_seq == nt.total_events && now_ms >= ev.onset_ms && (now_ms - ev.onset_ms) < 38u) break;
   checked_seq = next_seq;
   if (ev.legato || ev.peak <= 1e-5f) continue;

   if (count > 0 && prev_onset_ms > 0.f && (ev.onset_ms_exact - prev_onset_ms) > 2200.f) count = 0;
   prev_onset_ms = ev.onset_ms_exact;
   if (count == 0) round_done = false;
   note_db[count++] = 20.f * log10f(ev.peak);
   refreshBars(count);

   if (count >= ROUND_LEN) {
    if (accent_every == 0) {
     float med = medianFloat(note_db, ROUND_LEN);
     float sq = 0.f;
     for (uint8_t i = 0; i < ROUND_LEN; i++) {
      float d = note_db[i] - med;
      sq += d * d;
     }
     last_spread_db = sqrtf(sq / float(ROUND_LEN));
     round_clean = (last_spread_db <= EVEN_PASS_DB);
     if (!round_clean) wrong_at = now_ms;
     else wrong_at = 0;
    } else {
     float unacc[ROUND_LEN], acc[ROUND_LEN];
     int nu = 0, na = 0;
     for (uint8_t i = 0; i < ROUND_LEN; i++) {
      if ((i % accent_every) == 0) acc[na++] = note_db[i];
      else unacc[nu++] = note_db[i];
     }
     float u_med = medianFloat(unacc, nu);
     float a_med = medianFloat(acc, na);
     uint8_t hits = 0, false_acc = 0;
     for (int i = 0; i < na; i++) if ((acc[i] - u_med) >= ACCENT_MIN_DB) hits++;
     for (int i = 0; i < nu; i++) if ((unacc[i] - u_med) >= 3.5f) false_acc++;
     float sq = 0.f;
     for (int i = 0; i < nu; i++) { float d = unacc[i] - u_med; sq += d * d; }
     for (int i = 0; i < na; i++) { float d = acc[i] - a_med; sq += d * d; }
     last_spread_db = sqrtf(sq / float(ROUND_LEN));
     last_acc_hits = hits;
     last_acc_need = uint8_t(na);
     round_clean = (hits == uint8_t(na) && false_acc == 0 && last_spread_db <= EVEN_PASS_DB);
     if (!round_clean) wrong_at = now_ms;
     else wrong_at = 0;
    }

    int cdb = int(lroundf(last_spread_db * 100.f));
    if (cdb < 1) cdb = 1;
    if (cdb > 9999) cdb = 9999;
    st.last = uint16_t(cdb);
    if (st.rounds < 65535) st.rounds++;
    dirty = true;
    round_done = true;
    if (round_clean && (st.best == 0 || uint16_t(cdb) < st.best)) {
     st.best = uint16_t(cdb);
     best_at = now_ms;
     newBest = true;
    }
    count = 0;
   }
  }
  return dirty;
 }

 void fillView(DrillView& v, const char* title, const SkillStat& st, bool running, uint32_t now_ms) const {
  v.viz = VIZ_BARS;
  snprintf(v.title, sizeof v.title, "%s", title);
  if (st.best > 0) {
   snprintf(v.subtitle, sizeof v.subtitle, "BEST %u.%02uDB  %u/%u",
            unsigned(st.best / 100), unsigned(st.best % 100),
            unsigned(count ? count : ROUND_LEN), unsigned(ROUND_LEN));
  } else if (accent_every > 0) {
   snprintf(v.subtitle, sizeof v.subtitle, "ACCENT 1 OF %u  +4DB", unsigned(accent_every));
  } else {
   snprintf(v.subtitle, sizeof v.subtitle, "EVEN DYNAMICS  12 NOTES");
  }
  v.bar_count = bar_count;
  v.accent_mask = bar_acc_mask;
  for (uint8_t i = 0; i < bar_count && i < ROUND_LEN; i++) v.db_bars[i] = bars[i];
  v.ring_lit = uint8_t((uint32_t(count) * 60u) / ROUND_LEN);
  v.new_best = best_at > 0 && (now_ms - best_at < 2400u);
  v.wrong_flash = running && wrong_at > 0 && (now_ms - wrong_at < 900u);

  if (round_done && count == 0) {
   if (accent_every > 0 && !round_clean) {
    snprintf(v.mid, sizeof v.mid, "ACC %u/%u  %.1fDB",
             unsigned(last_acc_hits), unsigned(last_acc_need), double(last_spread_db));
    snprintf(v.detail, sizeof v.detail, "ACCENT BEAT 1 >= +4DB");
   } else {
    snprintf(v.mid, sizeof v.mid, "SPREAD %.2f DB", double(last_spread_db));
    snprintf(v.detail, sizeof v.detail, round_clean ? "EVEN CONTROL!" : "SMOOTH YOUR PICKING");
   }
  } else if (bar_count > 0) {
   snprintf(v.mid, sizeof v.mid, "NOTE %u/%u", unsigned(count), unsigned(ROUND_LEN));
   snprintf(v.detail, sizeof v.detail, accent_every > 0 ? "GOLD BAR = ACCENT +4DB" : "KEEP BARS ON LINE");
  } else {
   snprintf(v.mid, sizeof v.mid, accent_every > 0 ? "ACCENT EVERY %u" : "MATCH EVERY PICK", unsigned(accent_every));
   snprintf(v.detail, sizeof v.detail, running ? "PICK 12 NOTES" : "TAP FOOT TO START");
  }
 }
};

} // namespace vex
