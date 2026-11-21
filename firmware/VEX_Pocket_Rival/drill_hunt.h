#pragma once
// Fretboard note finder (HUNT): prompts notes by name anywhere (HUNT ANY) or on a specific string
// checked by exact pitch register (HUNT STR). Runs seeded 10-note rounds, repeats slow notes more
// often, and saves best average time per note (in centiseconds, lower is better).
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "core.h"
#include "notes.h"
#include "drill_view.h"

namespace vex {

struct HuntDrill {
 static constexpr uint32_t SEED = 0x48554E54u; // "HUNT"
 static constexpr uint8_t ROUND_LEN = 10;
 static constexpr int OPEN_MIDI[6] = {40, 45, 50, 55, 59, 64}; // 6E 5A 4D 3G 2B 1E
 static constexpr const char* STR_NAMES[6] = {"E6 STR", "A STR", "D STR", "G STR", "B STR", "E1 STR"};
 static constexpr const char* PC_NAMES[12] = {"C","C#","D","D#","E","F","F#","G","G#","A","Bb","B"};

 bool by_string = false;
 uint32_t rng = SEED;
 uint16_t weight_ms[12]{};      // adaptive slow-note weights (ms); higher = picked more often
 uint8_t step_idx = 0;          // 0..9 within current 10-note round
 uint8_t target_pc = 6;         // 0..11 (6 = F#)
 uint8_t target_str = 2;        // 0..5 (2 = D string)
 int16_t target_midi = 54;      // exact MIDI when `by_string` is true

 uint32_t prompt_at_ms = 0;
 uint32_t round_sum_ms = 0;
 uint32_t last_find_ms = 0;
 uint16_t last_avg_cs = 0;
 uint32_t best_at = 0;
 uint32_t wrong_at = 0;
 int16_t wrong_midi = 0;
 uint32_t checked_seq = 0;

 uint32_t nextRng() {
  uint32_t x = rng ? rng : SEED;
  x ^= x << 13; x ^= x >> 17; x ^= x << 5;
  rng = x;
  return x;
 }

 void configure(bool stringMode) {
  by_string = stringMode;
  rng = SEED;
  for (int i = 0; i < 12; i++) weight_ms[i] = 800;
  step_idx = 0;
  round_sum_ms = 0;
  last_find_ms = 0;
  best_at = 0;
  wrong_at = 0;
  pickNextTarget(0, true);
 }

 void pickNextTarget(uint32_t now_ms, bool first) {
  uint8_t prev_pc = target_pc;
  // Weighted draw biased toward notes the player was slower to find; never repeats immediately.
  uint32_t total_w = 0;
  for (int i = 0; i < 12; i++) if (first || i != prev_pc) total_w += weight_ms[i];
  uint32_t r = nextRng() % (total_w ? total_w : 1u);
  uint8_t chosen = 0;
  for (int i = 0; i < 12; i++) {
   if (!first && i == prev_pc) continue;
   if (r < weight_ms[i]) { chosen = uint8_t(i); break; }
   r -= weight_ms[i];
  }
  target_pc = chosen;
  if (by_string) {
   target_str = uint8_t(nextRng() % 6u);
   int open = OPEN_MIDI[target_str];
   int fret = (int(target_pc) - (open % 12) + 12) % 12;
   target_midi = int16_t(open + fret); // frets 0..11 uniquely span all 12 pitch classes per string
  } else {
   target_midi = 0;
  }
  prompt_at_ms = now_ms;
 }

 void start(uint32_t now_ms, uint32_t cur_event_seq) {
  checked_seq = cur_event_seq;
  step_idx = 0;
  round_sum_ms = 0;
  wrong_at = 0;
  prompt_at_ms = now_ms;
 }

 bool matches(int16_t midi) const {
  if (midi <= 0) return false;
  if (!by_string) return (midi % 12) == int(target_pc);
  // Exact pitch on that string (frets 0..11, plus fret 12 octave on open string).
  int open = OPEN_MIDI[target_str];
  if (midi == target_midi) return true;
  if (target_midi == open && midi == open + 12) return true;
  return false;
 }

 bool update(const NoteTracker& nt, uint32_t now_ms, SkillStat& st, bool& newBest) {
  newBest = false;
  bool dirty = false;
  // Note pitch settles ~25-50 ms after `total_events` increments, so inspect events once `pitched`!
  while (checked_seq < nt.total_events) {
   uint32_t next_seq = checked_seq + 1;
   if (nt.total_events - next_seq >= NoteTracker::EVENT_CAP) { checked_seq = next_seq; continue; }
   const NoteEvent& ev = nt.atSeq(next_seq);
   if (!ev.pitched) {
    // Wait up to 180 ms for YIN to settle on this latest event before skipping an unpitched scrape.
    if (next_seq < nt.total_events || (now_ms > ev.onset_ms && now_ms - ev.onset_ms > 180u)) checked_seq = next_seq;
    break;
   }
   checked_seq = next_seq;
   if (matches(ev.midi)) {
    uint32_t dt = now_ms > prompt_at_ms ? (now_ms - prompt_at_ms) : 100u;
    if (dt < 80u) dt = 80u;
    if (dt > 15000u) dt = 15000u;
    last_find_ms = dt;
    round_sum_ms += dt;
    // Boost weight on slow finds so hesitant notes come back more often.
    uint32_t w = (uint32_t(weight_ms[target_pc]) + dt * 2u) / 3u;
    if (w < 200u) w = 200u;
    if (w > 12000u) w = 12000u;
    weight_ms[target_pc] = uint16_t(w);
    wrong_at = 0;
    step_idx++;
    if (step_idx >= ROUND_LEN) {
     uint16_t avg_cs = uint16_t(lroundf(float(round_sum_ms) / float(ROUND_LEN * 10u)));
     if (avg_cs < 1u) avg_cs = 1u;
     last_avg_cs = avg_cs;
     st.last = avg_cs;
     if (st.rounds < 65535) st.rounds++;
     dirty = true;
     if (st.best == 0 || avg_cs < st.best) {
      st.best = avg_cs;
      best_at = now_ms;
      newBest = true;
     }
     step_idx = 0;
     round_sum_ms = 0;
    }
    pickNextTarget(now_ms, false);
   } else {
    wrong_midi = ev.midi;
    wrong_at = now_ms;
   }
  }
  return dirty;
 }

 void fillView(DrillView& v, const char* title, const SkillStat& st, bool running, uint32_t now_ms) const {
  v.viz = VIZ_PROMPT;
  snprintf(v.title, sizeof v.title, "%s", title);
  if (st.best > 0) {
   snprintf(v.subtitle, sizeof v.subtitle, "NOTE %u/%u  BEST %u.%02uS",
            unsigned(step_idx + 1), unsigned(ROUND_LEN), unsigned(st.best / 100), unsigned(st.best % 100));
  } else {
   snprintf(v.subtitle, sizeof v.subtitle, "NOTE %u/%u  10-NOTE ROUND",
            unsigned(step_idx + 1), unsigned(ROUND_LEN));
  }
  snprintf(v.big, sizeof v.big, "%s", PC_NAMES[target_pc]);
  snprintf(v.mid, sizeof v.mid, "%s", by_string ? STR_NAMES[target_str] : "ANY STRING");
  v.ring_lit = uint8_t((uint32_t(step_idx) * 60u) / ROUND_LEN);
  v.new_best = best_at > 0 && (now_ms - best_at < 2400u);
  v.wrong_flash = running && wrong_at > 0 && (now_ms - wrong_at < 900u);
  if (v.wrong_flash) {
   char played[8]; midiNoteName(wrong_midi, played);
   snprintf(v.detail, sizeof v.detail, "HEARD %s - TRY AGAIN", played);
  } else if (last_find_ms > 0) {
   snprintf(v.detail, sizeof v.detail, "LAST %u.%02uS", unsigned(last_find_ms / 1000), unsigned((last_find_ms % 1000) / 10));
  } else if (last_avg_cs > 0) {
   snprintf(v.detail, sizeof v.detail, "ROUND AVG %u.%02uS", unsigned(last_avg_cs / 100), unsigned(last_avg_cs % 100));
  } else {
   snprintf(v.detail, sizeof v.detail, running ? "PLAY TARGET NOTE" : "TAP FOOT TO START");
  }
 }
};

} // namespace vex
