---
name: vex-firmware-change
description: Implement or review PedalPartner VEX firmware, simulator, pixel UI, practice rewards and saved progress. Use for local testing, Teensy builds, audio-gating changes or device bring-up.
---

# VEX firmware change

Read `AGENTS.md`, `docs/HANDOFF.md` and `docs/LOCAL_DEVELOPMENT.md`. Inspect the relevant source before modifying it: `core.h` for rules/persistence, `ui.h` for shared pixels, `.ino` for hardware and `config.h` for pins.

1. Define the observable behavior and regression risk. Keep boot paused/uncalibrated, stale/clipped/silent/tuner samples excluded from practice, and one fragment per completed session.
2. Implement shared behavior in the existing C++ core/renderer. Keep hardware I/O at the firmware boundary. Never make an audio-callback change that adds display work, logging, allocation or EEPROM writes.
3. Add a focused behavioral regression where warranted, then run `make test`. For UI work, inspect relevant states with `make sim`; injected Hz is not a pitch-detector test.
4. Run `make firmware-all` after dependency setup. Inspect all four stage results. If downloads/toolchain are unavailable, report that explicitly and inspect the corresponding CI run; do not call host success a target compile.
5. If persistence changes, version/migrate the record and test save/reboot, corrupt latest fallback and old data handling. If pins change, update wiring/BOM/CAD and document the new hardware revision.
6. Update the handoff with commands, results and remaining bench checks. Upload only as a distinct, explicitly intended hardware operation; no build/test command should upload.

Use staged bring-up 1→2→3→4. The simulator shares logic and pixels but does not emulate the codec, physical display timing, analog noise or real EEPROM failure timing. Keep those limits clear in any result.
