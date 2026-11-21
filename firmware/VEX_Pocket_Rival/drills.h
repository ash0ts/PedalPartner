#pragma once
// Single flat list of drill presets and dispatcher connecting App to each per-drill header.
#include <stdint.h>
#include <string.h>
#include "core.h"
#include "drill_view.h"
#include "drill_pulse.h"
#include "drill_hunt.h"
#include "drill_bend.h"
#include "drill_run.h"
#include "drill_touch.h"

namespace vex {

enum DrillFamily : uint8_t {
 DRILL_PULSE = 0,
 DRILL_HUNT  = 1,
 DRILL_BEND  = 2,
 DRILL_RUN   = 3,
 DRILL_TOUCH = 4
};

struct DrillPreset {
 const char* slug;          // protocol / telemetry name: "pulse-8th", "bend-half", ...
 const char* title;         // 240x240 screen header
 DrillFamily family;
 SkillId skill;
 PitchMode pitch_mode;
 uint8_t param;             // family-specific preset parameter
};

constexpr DrillPreset DRILL_PRESETS[] = {
 {"pulse-qtr",  "PULSE QTR",  DRILL_PULSE, SKILL_PULSE, PITCH_OFF,   1},
 {"pulse-8th",  "PULSE 8TH",  DRILL_PULSE, SKILL_PULSE, PITCH_OFF,   2},
 {"pulse-free", "PULSE FREE", DRILL_PULSE, SKILL_PULSE, PITCH_OFF,   0},
 {"hunt-any",   "HUNT ANY",   DRILL_HUNT,  SKILL_HUNT,  PITCH_TUNER, 0},
 {"hunt-str",   "HUNT STR",   DRILL_HUNT,  SKILL_HUNT,  PITCH_TUNER, 1},
 {"bend-half",  "BEND 1/2",   DRILL_BEND,  SKILL_BEND,  PITCH_FAST,  1},
 {"bend-whole", "BEND FULL",  DRILL_BEND,  SKILL_BEND,  PITCH_FAST,  2},
 {"run-penta",  "RUN PENTA",  DRILL_RUN,   SKILL_RUN,   PITCH_RUN,   0},
 {"run-major",  "RUN MAJOR",  DRILL_RUN,   SKILL_RUN,   PITCH_RUN,   1},
 {"run-minor",  "RUN MINOR",  DRILL_RUN,   SKILL_RUN,   PITCH_RUN,   2},
 {"touch-even", "TOUCH EVEN", DRILL_TOUCH, SKILL_TOUCH, PITCH_OFF,   0},
 {"touch-acc3", "TOUCH ACC3", DRILL_TOUCH, SKILL_TOUCH, PITCH_OFF,   3},
 {"touch-acc4", "TOUCH ACC4", DRILL_TOUCH, SKILL_TOUCH, PITCH_OFF,   4},
};
constexpr uint8_t DRILL_PRESET_COUNT = uint8_t(sizeof(DRILL_PRESETS) / sizeof(DRILL_PRESETS[0]));

struct DrillHost {
 uint8_t preset_idx = 1; // defaults to PULSE 8TH
 PulseDrill pulse{};
 HuntDrill hunt{};
 BendDrill bend{};
 RunDrill run{};
 TouchDrill touch{};

 const DrillPreset& preset() const {
  return DRILL_PRESETS[preset_idx < DRILL_PRESET_COUNT ? preset_idx : 0];
 }

 void select(uint8_t idx, const Skills& skills) {
  preset_idx = idx < DRILL_PRESET_COUNT ? idx : 0;
  const auto& pr = preset();
  const SkillStat& st = skills.stats[pr.skill];
  switch (pr.family) {
   case DRILL_PULSE: pulse.configure(pr.param, st); break;
   case DRILL_HUNT:  hunt.configure(pr.param != 0); break;
   case DRILL_BEND:  bend.configure(pr.param); break;
   case DRILL_RUN:   run.configure(pr.param); break;
   case DRILL_TOUCH: touch.configure(pr.param); break;
  }
 }

 bool selectBySlug(const char* slug, const Skills& skills) {
  for (uint8_t i = 0; i < DRILL_PRESET_COUNT; i++) {
   if (strcmp(DRILL_PRESETS[i].slug, slug) == 0) { select(i, skills); return true; }
  }
  return false;
 }

 void start(uint32_t now_ms, uint32_t cur_event_seq, uint32_t cur_trace_seq = 0) {
  switch (preset().family) {
   case DRILL_PULSE: pulse.start(now_ms, cur_event_seq); break;
   case DRILL_HUNT:  hunt.start(now_ms, cur_event_seq); break;
   case DRILL_BEND:  bend.start(now_ms, cur_event_seq, cur_trace_seq); break;
   case DRILL_RUN:   run.start(now_ms, cur_event_seq); break;
   case DRILL_TOUCH: touch.start(now_ms, cur_event_seq); break;
  }
 }

 PitchMode pitchMode() const { return preset().pitch_mode; }

 uint32_t beatPeriodMs() const {
  if (preset().family == DRILL_PULSE) return pulse.beatPeriodMs();
  return 0u;
 }
 uint32_t beatOriginMs() const {
  if (preset().family == DRILL_PULSE) return pulse.beat_origin_ms;
  return 0u;
 }
 uint16_t liveBpm() const {
  if (preset().family == DRILL_PULSE) {
   return pulse.subdiv > 0 ? pulse.bpm : uint16_t(lroundf(pulse.inferred_bpm));
  }
  if (preset().family == DRILL_RUN) return run.inferred_bpm;
  return 0u;
 }

 bool update(const NoteTracker& nt, uint32_t now_ms, Skills& skills, bool& newBest) {
  const auto& pr = preset();
  SkillStat& st = skills.stats[pr.skill];
  switch (pr.family) {
   case DRILL_PULSE: return pulse.update(nt, now_ms, st, newBest);
   case DRILL_HUNT:  return hunt.update(nt, now_ms, st, newBest);
   case DRILL_BEND:  return bend.update(nt, now_ms, st, newBest);
   case DRILL_RUN:   return run.update(nt, now_ms, st, newBest);
   case DRILL_TOUCH: return touch.update(nt, now_ms, st, newBest);
  }
  newBest = false;
  return false;
 }

 DrillView view(const Skills& skills, bool running, uint32_t now_ms) const {
  DrillView v{};
  const auto& pr = preset();
  const SkillStat& st = skills.stats[pr.skill];
  switch (pr.family) {
   case DRILL_PULSE: pulse.fillView(v, pr.title, st, running, now_ms); break;
   case DRILL_HUNT:  hunt.fillView(v, pr.title, st, running, now_ms); break;
   case DRILL_BEND:  bend.fillView(v, pr.title, st, running, now_ms); break;
   case DRILL_RUN:   run.fillView(v, pr.title, st, running, now_ms); break;
   case DRILL_TOUCH: touch.fillView(v, pr.title, st, running, now_ms); break;
  }
  return v;
 }
};

} // namespace vex
