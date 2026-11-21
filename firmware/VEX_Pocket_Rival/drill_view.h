#pragma once
// Shared DrillView struct consumed by ui.h (drawDrill) and populated by each flat drill header.
#include <stdint.h>

namespace vex {

enum DrillVizKind : uint8_t {
 VIZ_TIMING = 0,   // PULSE: early/late dots around a centre beat line + bias/spread
 VIZ_PROMPT = 1,   // HUNT:  bold note + optional string prompt + reaction timer
 VIZ_BEND   = 2,   // BEND:  live cents curve vs target line + last-8 bends / vibrato readout
 VIZ_SCALE  = 3,   // RUN:   12-degree chromatic ring + in/out flashes + tempo evenness
 VIZ_BARS   = 4    // TOUCH: per-note dB level bars vs running median + accent markers
};

struct DrillView {
 DrillVizKind viz = VIZ_TIMING;
 char title[20] = "PULSE 8TH";
 char subtitle[28] = "";
 char big[16] = "";
 char mid[24] = "";
 char detail[28] = "";
 uint8_t ring_lit = 0;         // 0..60 outer ring progress segments
 bool new_best = false;
 bool wrong_flash = false;

 // VIZ_TIMING: up to 12 recent timing offsets in ms, clamped to [-60, +60].
 int8_t dots[12]{};
 uint8_t dot_count = 0;
 int8_t pass_ms = 30;

 // VIZ_BEND (populated in PR 4)
 int16_t target_cents = 100;
 int16_t live_cents = 0;
 int16_t bend_hist[8]{};
 uint8_t bend_count = 0;

 // VIZ_SCALE (populated in PR 4)
 uint16_t scale_mask = 0;      // 12-bit mask relative to root
 int8_t root_pc = -1;
 int8_t last_rel_pc = -1;
 bool last_in_scale = true;

 // VIZ_BARS (populated in PR 4)
 int8_t db_bars[12]{};         // dB*2 relative to median (-24..+24 => -12..+12 dB)
 uint8_t accent_mask = 0;      // bit k = bar k was an accent target
 uint8_t bar_count = 0;
};

} // namespace vex
