---
name: vex-firmware-change
description: Implement or review PedalPartner VEX firmware, simulator, pixel UI, practice rewards and saved progress. Use for local testing, Teensy builds, audio-gating changes or device bring-up.
---

# VEX firmware change

Read `AGENTS.md`, `docs/HANDOFF.md` and `docs/LOCAL_DEVELOPMENT.md`. Inspect the relevant source before modifying it: `core.h` for rules/persistence, `app.h` for the shared device state machine (controls, calibration, notices, saves), `analyzer.h` for level/pitch analysis, `ui.h` for shared pixels, `.ino` for hardware adapters and `config.h` for pins.

1. Define the observable behavior and regression risk. Keep boot paused/uncalibrated, stale/clipped/silent/tuner samples excluded from practice, and one fragment per completed session.
2. Implement shared behavior in the existing C++ core/app/analyzer/renderer. Keep hardware I/O at the firmware boundary (`.ino` adapters). Never make an audio-callback change that adds display work, logging, allocation or EEPROM writes; `Analyzer::push` is the only interrupt-side call.
3. Add a focused behavioral regression where warranted: a `tests/scenarios/*.txt` script with `expect`/`golden` lines for device behavior or UI, `tests/core_tests.cpp` for pure functions and detector cases. The browser workbench (`make sim`) can record a session as protocol lines (Scenario tab, "Copy as scenario") and replay scenario text with `expect` lines; the native `build/vex-sim` stays the oracle that `make test` runs. After an intentional UI change run `python3 tests/simulator_test.py --update-golden` and review the frames. Injected Hz exercises note math; the `wav`/`raw` commands exercise the shared detector on Mac audio, which is still not codec/gain/latency evidence.
4. For changes to target code, shared headers, pins or dependencies, compile affected target stages (all four before hardware bring-up). Run setup only if dependencies are missing. Redirect verbose output to `build/` and inspect a short error excerpt plus the report. For docs, marketing and packaging changes, reuse existing evidence; do not set up or compile firmware. If the toolchain/download is unavailable, record the exact blocker and stop retrying; manual CI is optional, not a required fallback. Never call host success a target compile.
5. If persistence changes, version/migrate the record and test save/reboot, corrupt latest fallback and old data handling. If pins change, update wiring/BOM/CAD and document the new hardware revision.
6. Update the handoff with commands, results and remaining bench checks. Upload only as a distinct, explicitly intended hardware operation; no build/test command should upload.

Use staged bring-up 1→2→3→4. Both simulators (native and WebAssembly) share logic and pixels but do not emulate the codec, physical display timing, analog noise or real EEPROM failure timing. Keep those limits clear in any result. A change confined to `simulator/web/` needs `make test` (which includes `tools/web.py check`), not a target compile.

Publish changes using `docs/PUBLISHING.md`. Preserve unverified target/bench status until actual evidence changes it. Never route build logs, binary data or repeated encoding/bridge chunks through chat.
