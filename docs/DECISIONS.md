# Design decisions — R1.1

| Decision | Reason / boundary |
|---|---|
| Keep the real 1.28-inch screen | A documented Adafruit board and GC9A01A driver support a first build. Concept-art screen proportions are aspirational. If standing readability fails, make a deliberate new hardware revision. |
| Transfer the rival vibe through details | Charcoal octagonal bezel, thin muted-gold rim, crimson cheek/base band/buttons, shallow panel marks and original pixel bat preserve personality without a complex shell. |
| Flat 40 × 1 mm window | An accessible, replaceable cover. A fishbowl lens remains a later optical experiment requiring glare/distortion tests and its own retention design. |
| Listen on a buffered dry branch | Keeps prototype software out of the audible signal path. The passive attenuator is not a high-impedance guitar preamp. No audio output or mute function is implemented. |
| USB 5 V only | Avoids adding a 9 V protection/regulation design to the first build. USB ground noise must still be tested with the actual rig. |
| Participation XP, one quest fragment per session | Clear and testable motivation. Audio activity is not evidence of good timing, intonation or technique. No calendar clock means no true daily streaks. |
| Five STEP service groups | Makes review and removal of the lid/carrier practical. Hardware belongs to its mating assembly; wiring reserves are separate. |
| Python construction plus neutral solids | Reproducible source and broad CAD interoperability. STEP has no native feature history. |
| Shared C++ logic and renderer | Host simulation exercises the actual rules/pixels. It injects signal measurements rather than pretending to emulate SGTL5000/DSP. |
| Shared device state machine (`app.h`) | Calibration, hold actions, notices, auto-save and the ordered two-slot write live in one header driven by both the sketch and the simulator through tiny adapters (clock, controls, observations, byte store). Scenario scripts and golden frames then test the firmware's real behaviour, including power cuts mid-write. |
| Shared YIN analyzer in the foreground (`analyzer.h`) | The audio interrupt only copies blocks and accumulates level; pitch runs in `loop()` on a 2048-sample window. The same code processes the reference WAV and Mac input on the desktop. `VEX_PITCH_PJRC` keeps the PJRC detector selectable for bench comparison. Desktop results are not SGTL5000/gain/latency evidence; `yin_us` telemetry exists so the bench can measure cost. |
| Presentation facts stay out of the save record | Fragment/level-up banners, strum animation and the session summary derive from runtime fields in `App`; the version 1 record is unchanged, so no migration was needed. |
| Two CRC-protected save records | Recovers the last valid slot after a damaged write. Hardware power interruption and EEPROM wear still need validation; changing format requires migration. |
| Lean AGENTS.md plus two task skills | Always-load context stays small; detailed procedures and evidence are read when relevant. No session-history importer is required. |
| Actual-CAD marketing images | Reuse checked-in STEP geometry and shared UI pixels with studio lighting/proposed finishes. Record hashes; no generative product imagery or invented hardware. |
| Build-free mentor packing and bounded publication | Package reviewed outputs with a deterministic manifest. Send only changed small files; never repeat chunk/bridge loops or target builds for presentation work. |
| Browser workbench runs the firmware as WebAssembly | The earlier page proxied each action over HTTP to a native process and polled PNGs, so presses were dropped and timing was the poll's, not the device's. The shared headers now compile to WASM through a thin adapter (`simulator/web/vex_web.cpp`); the page owns a millisecond clock, real-time or faster, and blits the shared renderer's RGB565 frame directly. The native `build/vex-sim` remains the test oracle and `tools/web.py check` proves both render identical frames. No server logic to keep in sync. |
| Tuner states are labelled, and the last note fades | Waiting, listening, uncertain (`PITCH_UNCERTAIN` ≤ probability < `PITCH_VALID`), sounding, stale (1.5 s fade) and clipping each carry text, so colour never carries meaning alone and a decaying string does not blank the display. A strobe view was rejected: the pedal redraws every 200 ms over SPI. |

Rejected for R1.1: large fictional screen, external ears/armor, custom dome, RGB spectacle, audio bypass, mains/battery/9 V conversion, broad game systems and speculative automatic technique scores. These are scope decisions, not permanent bans.

## Extension direction

- Retain the offline practice companion as the core product; richer coaching is optional.
- Preserve platform-independent practice rules and presentation data. Extract hardware adapters incrementally as features need them.
- Introduce versioned session summaries and a bounded USB application protocol before host-agent integration; existing debug serial is not that protocol.
- A coaching policy proposes supported exercises; the device owns measurement, session state and reward accounting. A general language-model planner belongs on an optional host initially.
- Port data/content contracts to V2 rather than promising processor pin or enclosure compatibility. No processor upgrade substitutes for measured recognition accuracy, readability or noise/strength testing.

The concrete boundaries, current gaps and acceptance sequence are in [EXTENSIBILITY.md](EXTENSIBILITY.md). These are architecture decisions for future implementation, not claims that those capabilities are present.
