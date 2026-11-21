#pragma once
// Scale run checker (RUN): first note in each 16-note phrase sets the root; presets select minor
// pentatonic, major or natural minor. Lights 12 chromatic degree dots (gold for in-scale notes,
// red + counted for outside notes), checks note spacing evenness using the PULSE IOI median math,
// and saves best clean eighth-note tempo (0 outside notes and <=30 ms spacing spread).
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "core.h"
#include "notes.h"
#include "drill_view.h"

namespace vex {

struct RunDrill {
 static constexpr uint8_t ROUND_LEN = 16;
 static constexpr float PASS_SPREAD_MS = 30.f;
 static constexpr uint16_t MASKS[3] = {
  0x04A9u, // minor pentatonic: 0, 3, 5, 7, 10
  0x0AB5u, // major:            0, 2, 4, 5, 7, 9, 11
  0x05ADu  // natural minor:    0, 2, 3, 5, 7, 8, 10
 };
 static constexpr const char* SCALE_LABELS[3] = {"MIN PENTA", "MAJOR", "NAT MINOR"};
 static constexpr const char* PC_NAMES[12] = {"C","C#","D","D#","E","F","F#","G","G#","A","Bb","B"};

 uint8_t scale_kind = 0;       // 0=penta, 1=major, 2=minor
 uint16_t scale_mask = MASKS[0];
 int8_t root_pc = -1;
 int8_t last_rel_pc = -1;
 bool last_in_scale = true;

 uint8_t step_idx = 0;
 uint8_t outside_count = 0;
 uint8_t last_outside = 0;
 float prev_onset_ms = 0.f;
 bool prev_legato = false;
 float ioi_ms[ROUND_LEN]{};
 uint8_t ioi_count = 0;
 float spread_ms = 0.f;
 uint16_t inferred_bpm = 0;

 uint32_t checked_seq = 0;
 uint32_t best_at = 0;
 uint32_t wrong_at = 0;
 bool round_done = false;
 bool round_clean = false;

 static float medianFloat(const float* src, int n) {
  if (n <= 0) return 0.f;
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

 void resetPhrase() {
  step_idx = 0;
  outside_count = 0;
  ioi_count = 0;
  prev_onset_ms = 0.f;
  root_pc = -1;
  last_rel_pc = -1;
  last_in_scale = true;
 }

 void configure(uint8_t kind) {
  scale_kind = kind < 3 ? kind : 0;
  scale_mask = MASKS[scale_kind];
  last_outside = 0;
  spread_ms = 0.f;
  inferred_bpm = 0;
  best_at = 0;
  wrong_at = 0;
  round_done = false;
  round_clean = false;
  resetPhrase();
 }

 void start(uint32_t /*now_ms*/, uint32_t cur_event_seq) {
  checked_seq = cur_event_seq;
  wrong_at = 0;
  round_done = false;
  resetPhrase();
 }

 void updateSpacingStats() {
  if (ioi_count < 2) return;
  float med = medianFloat(ioi_ms, ioi_count);
  if (med <= 1.f) return;
  float sq = 0.f;
  for (uint8_t i = 0; i < ioi_count; i++) {
   float d = ioi_ms[i] - med;
   sq += d * d;
  }
  spread_ms = sqrtf(sq / float(ioi_count));
  int bpm = int(lroundf(30000.f / med)); // eighth-note scale run tempo (2 notes/beat)
  if (bpm < 20) bpm = 20;
  if (bpm > 300) bpm = 300;
  inferred_bpm = uint16_t(bpm);
 }

 bool update(const NoteTracker& nt, uint32_t now_ms, SkillStat& st, bool& newBest) {
  newBest = false;
  bool dirty = false;
  while (checked_seq < nt.total_events) {
   uint32_t next_seq = checked_seq + 1;
   if (nt.total_events - next_seq >= NoteTracker::EVENT_CAP) { checked_seq = next_seq; continue; }
   const NoteEvent& ev = nt.atSeq(next_seq);
   if (!ev.pitched) {
    if (next_seq < nt.total_events || (now_ms > ev.onset_ms && now_ms - ev.onset_ms > 180u)) checked_seq = next_seq;
    break;
   }
   checked_seq = next_seq;

   // A pause >2.2 s starts a fresh phrase on the newly played root note.
   if (step_idx > 0 && prev_onset_ms > 0.f && (ev.onset_ms_exact - prev_onset_ms) > 2200.f) {
    resetPhrase();
   }
   if (step_idx == 0) {
    root_pc = int8_t(ev.midi % 12);
    outside_count = 0;
    ioi_count = 0;
    round_done = false;
   } else if (prev_onset_ms > 0.f && !ev.legato && !prev_legato) {
    float dt = ev.onset_ms_exact - prev_onset_ms;
    if (dt >= 40.f && dt <= 1800.f && ioi_count < ROUND_LEN) ioi_ms[ioi_count++] = dt;
    updateSpacingStats();
   }
   prev_onset_ms = ev.onset_ms_exact;
   prev_legato = ev.legato;

   int rel = (int(ev.midi % 12) - int(root_pc) + 12) % 12;
   last_rel_pc = int8_t(rel);
   last_in_scale = ((scale_mask >> rel) & 1u) != 0;
   if (!last_in_scale) {
    if (outside_count < 255) outside_count++;
    wrong_at = now_ms;
   } else {
    wrong_at = 0;
   }

   step_idx++;
   if (step_idx >= ROUND_LEN) {
    updateSpacingStats();
    last_outside = outside_count;
    round_done = true;
    round_clean = (outside_count == 0 && ioi_count >= 8 && spread_ms <= PASS_SPREAD_MS && inferred_bpm >= 40);
    st.last = inferred_bpm;
    if (st.rounds < 65535) st.rounds++;
    dirty = true;
    if (round_clean && inferred_bpm > st.best) {
     st.best = inferred_bpm;
     best_at = now_ms;
     newBest = true;
    }
    step_idx = 0;
    prev_onset_ms = 0.f;
   }
  }
  return dirty;
 }

 void fillView(DrillView& v, const char* title, const SkillStat& st, bool running, uint32_t now_ms) const {
  v.viz = VIZ_SCALE;
  snprintf(v.title, sizeof v.title, "%s", title);
  if (root_pc >= 0) {
   if (st.best > 0) {
    snprintf(v.subtitle, sizeof v.subtitle, "%s %s  BEST %u",
             PC_NAMES[root_pc], SCALE_LABELS[scale_kind], unsigned(st.best));
   } else {
    snprintf(v.subtitle, sizeof v.subtitle, "%s %s  %u/%u",
             PC_NAMES[root_pc], SCALE_LABELS[scale_kind], unsigned(step_idx ? step_idx : ROUND_LEN), unsigned(ROUND_LEN));
   }
  } else if (st.best > 0) {
   snprintf(v.subtitle, sizeof v.subtitle, "%s  BEST %u BPM", SCALE_LABELS[scale_kind], unsigned(st.best));
  } else {
   snprintf(v.subtitle, sizeof v.subtitle, "%s  16 NOTES", SCALE_LABELS[scale_kind]);
  }
  v.scale_mask = scale_mask;
  v.root_pc = root_pc;
  v.last_rel_pc = last_rel_pc;
  v.last_in_scale = last_in_scale;
  v.ring_lit = uint8_t((uint32_t(step_idx) * 60u) / ROUND_LEN);
  v.new_best = best_at > 0 && (now_ms - best_at < 2400u);
  v.wrong_flash = running && (!last_in_scale || (wrong_at > 0 && (now_ms - wrong_at < 900u)));

  uint8_t shown_out = round_done && step_idx == 0 ? last_outside : outside_count;
  if (root_pc < 0) {
   snprintf(v.mid, sizeof v.mid, "FIRST NOTE = ROOT");
   snprintf(v.detail, sizeof v.detail, running ? "PLAY 16-NOTE RUN" : "TAP FOOT TO START");
  } else if (!last_in_scale) {
   snprintf(v.mid, sizeof v.mid, "OUTSIDE! (%u OUT)", unsigned(shown_out));
   snprintf(v.detail, sizeof v.detail, "%u BPM  SPREAD %dMS", unsigned(inferred_bpm), int(lroundf(spread_ms)));
  } else if (round_done && step_idx == 0) {
   snprintf(v.mid, sizeof v.mid, round_clean ? "CLEAN %u BPM" : "%u OUT  %dMS",
            unsigned(round_clean ? inferred_bpm : last_outside), int(lroundf(spread_ms)));
   snprintf(v.detail, sizeof v.detail, round_clean ? "PLAY NEXT ROOT" : "KEEP EVEN + IN KEY");
  } else {
   snprintf(v.mid, sizeof v.mid, "%u BPM  %u OUT", unsigned(inferred_bpm), unsigned(shown_out));
   snprintf(v.detail, sizeof v.detail, "SPREAD %dMS  NOTE %u/%u",
            int(lroundf(spread_ms)), unsigned(step_idx), unsigned(ROUND_LEN));
  }
 }
};

} // namespace vex
