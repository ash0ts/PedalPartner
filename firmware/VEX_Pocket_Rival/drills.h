#pragma once
// Single flat list of drill presets and dispatcher connecting App to each per-drill header.
#include <stdint.h>
#include <string.h>
#include "core.h"
#include "drill_view.h"
#include "drill_pulse.h"
#include "drill_hunt.h"

namespace vex {

enum DrillFamily : uint8_t {
 DRILL_PULSE = 0,
 DRILL_HUNT  = 1
};

struct DrillPreset {
 const char* slug;          // protocol / telemetry name: "pulse_8th", "hunt_str", ...
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
};
constexpr uint8_t DRILL_PRESET_COUNT = uint8_t(sizeof(DRILL_PRESETS) / sizeof(DRILL_PRESETS[0]));

struct DrillHost {
 uint8_t preset_idx = 1; // defaults to PULSE 8TH
 PulseDrill pulse{};
 HuntDrill hunt{};

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
  }
 }

 bool selectBySlug(const char* slug, const Skills& skills) {
  for (uint8_t i = 0; i < DRILL_PRESET_COUNT; i++) {
   if (strcmp(DRILL_PRESETS[i].slug, slug) == 0) { select(i, skills); return true; }
  }
  return false;
 }

 void start(uint32_t now_ms, uint32_t cur_event_seq) {
  switch (preset().family) {
   case DRILL_PULSE: pulse.start(now_ms, cur_event_seq); break;
   case DRILL_HUNT:  hunt.start(now_ms, cur_event_seq); break;
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
  return 0u;
 }

 bool update(const NoteTracker& nt, uint32_t now_ms, Skills& skills, bool& newBest) {
  const auto& pr = preset();
  SkillStat& st = skills.stats[pr.skill];
  switch (pr.family) {
   case DRILL_PULSE: return pulse.update(nt, now_ms, st, newBest);
   case DRILL_HUNT:  return hunt.update(nt, now_ms, st, newBest);
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
  }
  return v;
 }
};

} // namespace vex
