# Starter plan and acceptance gates

| Milestone | State | Done when |
|---|---|---|
| 0. Practical R1.1 package | Complete as a digital proposal | Grouped STEP, source, selected STLs/DXFs, BOM, illustrated guide and explicit limitations are present |
| 0a. Presentation and mentor handoff | Prepared for PR review | Real-CAD hero/share images with provenance, reproducible lean ZIP, and bounded task/publication guidance |
| 1. Desktop behavior | Host tests passed; native simulator and the in-browser WebAssembly workbench run the shared `App`, analyzer and renderer | Fresh checkout passes tests; scenario scripts and golden frames cover practice, tuner states, calibration, holds, save/reboot, power-cut fallback and the reference recording through the shared detector; the WASM module renders native goldens |
| 2. Target builds | Passed locally on 17 Nov 2026 (arduino-cli 1.6.0, Teensy 1.62.0; both analyzer paths) | All four stages compile with pinned dependencies; build report and binaries retained in `build/` |
| 2a. Bench instrumentation | Built and host-tested 17 Nov 2026; nothing run on a board | Sketch reports `flush_us`/`loop_max_us`/`tx_bytes` JSON and takes serial switches and live knobs (SPI clock, codec level, test pattern); `make bench`/`make upload`/`make monitor`; workbench Device tab mirrors the panel over the `link.h` protocol; sessions logged and summarised by `make bench-report`; `make measure-check` gates fabrication on `measurements.csv` (see `docs/BRINGUP.md`) |
| 3. Pet on desk | Next physical work | Stage 1/2 work; all controls and screen readable; actual supply measured; first-session observables in HANDOFF recorded (`flush_max_us`, seams, 100 vs 200 ms, mirror diff 0) |
| 3a. Display driver decision | Gated on milestone 3 numbers | Keep the pinned Adafruit path if `flush_max_us` stays under about a third of the cadence with dirty bands and no seams; otherwise implement a DMA driver behind the sketch's `Panel` adapter with the measurement attached |
| 4. Audio listener | Pending | Mentor-reviewed attenuator; stage 3 clean input; silence/playing/clipping distinctions verified for intended guitars |
| 5. Fit and fabrication | Pending | Real parts measured, CAD revised if needed, coupon accepted, full enclosure fits without force |
| 6. Enclosed prototype | Pending | Service loops, plate/columns, window clearance, heat/noise and structural tests pass |
| 7. Useful practice partner | Pending | Reference-note tuner trial, 5/10/20-minute quests, repeated save/reboot and normal rig presets pass |

## First two-hour mentor session

- 0–20 min: review architecture, parts and power; identify the dry feed and mechanical unknowns.
- 20–50 min: practice soldering, meter switch/trimmer terminals, inspect rails.
- 50–100 min: stage 1, screen and controls, stage 2 (`make upload STAGE=2`, `make monitor`; note `flush_us`).
- 100–120 min: readability and measured fit; assign the next bench session.

Later candidates, after milestone 7: richer original pet animations, better exercise feedback with reference datasets, larger display revision, optional lens experiment, protected 9 V input or MIDI. Each needs its own requirements and tests. The first prototype remains intentionally small in software scope.

## Extension path after a working buddy

See [EXTENSIBILITY.md](EXTENSIBILITY.md) for the reviewed architecture proposal: portable practice logic, explicit observations/exercise definitions, versioned history and USB sync, local adaptive suggestions, then an optional computer-hosted agent. These interfaces and features are not yet implemented.

Keep the immediate build as the existing listening companion. Future processor/display revisions should preserve progress and software contracts while accepting that drivers, carrier and enclosure may change. Complete target builds and bench measurements before selecting extensions from measured needs.
