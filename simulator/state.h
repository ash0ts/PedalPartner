#pragma once
// Single source of truth for simulator state fields, shared by native JSON output (simulator/main.cpp)
// and WebAssembly numeric/string snapshots + schema export (simulator/web/vex_web.cpp).
#include <stdint.h>
#include "../firmware/VEX_Pocket_Rival/app.h"
#include "../firmware/VEX_Pocket_Rival/analyzer.h"

namespace vexsim {

struct StateSource {
 const vex::App& app;
 const vex::Analyzer& analyzer;
 bool powered;
 uint32_t analyses;
 bool test;
};

template<class Emit>
inline void stateFields(const StateSource& s, Emit&& emit) {
 const auto& p = s.app.progress;
 vex::AnimFrame f = vex::chooseAnim(vex::quantised(s.app.view()));
 emit.u("active_ms", p.active_ms);
 emit.u("session_ms", p.session_ms);
 emit.u("xp", p.xp());
 emit.u("level", p.level());
 emit.u("fragments", p.fragments);
 emit.u("goal_minutes", p.goalMs() / 60000);
 emit.b("running", p.running);
 emit.b("calibrated", p.calibrated);
 emit.b("active", p.active);
 emit.s("activity", s.app.act().name);
 emit.b("tuning", s.app.tuning());
 emit.b("calibrating", s.app.calibrating);
 emit.f("gate", p.gate);
 emit.f("rms", s.app.rms);
 emit.f("env", s.app.env);
 emit.f("peak", s.app.peak);
 emit.u("hold_ms", p.hold_ms);
 emit.u("credit_drops", p.credit_drops);
 emit.i("stage", s.app.stage);
 emit.s("notice", s.app.notice);
 emit.i("saved_slot", s.app.saved_slot);
 emit.u("sequence", s.app.sequence);
 emit.b("power", s.powered);
 emit.i("pitch_midi", s.app.pitchNow.midi);
 emit.f("pitch_cents", s.app.pitchNow.cents);
 emit.b("pitch_valid", s.app.pitchNow.valid);
 emit.b("uncertain", s.app.uncertain);
 emit.f("hz", s.analyzer.hz);
 emit.f("probability", s.analyzer.probability);
 emit.u("analyses", s.analyses);
 emit.u("now_ms", s.app.now);
 emit.i("anim", f.anim);
 emit.i("frame", f.frame);
 emit.b("test", s.test);
}

} // namespace vexsim
