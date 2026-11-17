# Current handoff — R1.1

Last prepared: 17 November 2026, evening (New York). Repository: [ash0ts/PedalPartner](https://github.com/ash0ts/PedalPartner).

## Where we left off

The user wants a feasible first pedal build with a mentor, retaining a rival-device / virtual-pet personality. Earlier concept images were appealing but too futuristic; the first practical CAD lost charm. R1.1 keeps the feasible mechanical/electrical architecture and transfers the color/character through small cosmetic changes. Do not return to a huge imaginary display or flatten the design into a generic box.

This is a **complete starter design package for review**, not a physically commissioned device. Target compilation of all four stages now passes locally; the next useful work is measured bench bring-up, followed by fit-coupon changes. Automatic CI covers only desktop tests. Re-verify target compilation after any firmware change before bench use; it is not a publication prerequisite.

## Evidence

| Check | State / evidence |
|---|---|
| Shared core unit tests | Passed locally; `make test` reproduces. Includes `App` calibration/notice/save checks and the YIN analyzer on synthetic tones, harmonic-rich and decaying tones, noise, silence and square waves |
| Native simulator integration | Passed: eight scenario scripts through the shared `App` (calibration variants, hold actions, goals, saves, torn writes at 0/10/50/52 bytes, stages, tuner, personality states) with 31 golden frames, plus `tests/art_test.py` for the art importer; the reference WAV yields E2 A2 D3 G3 B3 E4 A4 through the shared detector and credits practice only while the tones sound |
| Target Teensy compilation | **Passed locally on 17 Nov 2026**: arduino-cli 1.6.0 (Homebrew), Teensy core 1.62.0, pinned Adafruit libraries; all four stages with the shared analyzer and stage 4 with `VEX_PITCH_PJRC`. Stage 4 after the art pipeline change: 102,920 B code, 55,232 B RAM1 variables, 149,440 B RAM2 variables (display shadow), no warnings from project sources. Report in `build/target_build_report.json` (ignored). A compile is not a bench result. |
| CAD validity / collisions | Passed: 72 parts, 2,556 pairs, no modeled overlaps; `hardware/generated/checks/cad_validation.json` |
| Grouped STEP round trip | Passed: five groups and all 72 component names/bounds/volumes/solid counts; `step_roundtrip.json` |
| CAD hero and share image | Rendered with Blender/Cycles from the checked-in STEP; all 72 tessellated component bounds match the recorded CAD. Both layouts visually checked; hashes in `assets/marketing/provenance.json`. |
| Lean mentor pack | `make pack-mentor` passed; 37 ZIP members, 36 hashed payloads, 10,831,292 bytes. Independent index/link/hash/CRC review passed; ten print-candidate STLs and printing instructions included. |
| Physical build / uploaded firmware | Not performed |
| Electrical, optical, thermal, foot strength | Not performed; blank acceptance rows intentionally remain blank |

## Extensibility review checkpoint

[EXTENSIBILITY.md](EXTENSIBILITY.md) records a source-reviewed proposal for portable practice rules, typed observations, session history/save migration, USB sync and bounded local/host coaching policies. Core rules and rendering are already shared with the simulator; versioned host commands, session history, adaptive policies, migration tooling and a companion app are not implemented.

This is a documentation-only proposal. Firmware, physical parts and the R1.1 fabrication packet are unchanged. Review covered `core.h`, `ui.h`'s existing role, `.ino`, `config.h`, existing decisions/development guidance and PJRC specifications. Source-to-manifest checks and local Markdown-link checks were used for the proposal; existing software/CAD evidence is reused, with no new firmware or CAD test claim. The next action remains target compilation and measured bench bring-up.

## Presentation, packaging and publication

The initial starter and extension proposal are merged. The presentation update starts from `main` commit `816bd2653e25b703aa466740ec4dd8b8bc9ceee4`; all 141 source files were verified against that tree before edits. The README now uses a studio render of the actual grouped STEP, with a share crop and reproducible scripts. Geometry, firmware, BOM, guide PDFs and assembly-guide illustrations are unchanged. No generative imagery is used.

`make hero` tessellates/renders existing geometry; `make pack-mentor` packages existing outputs into the lean ZIP. [MARKETING.md](MARKETING.md) records reproduction/provenance; [MENTOR_PACK.md](MENTOR_PACK.md) explains navigation. Host core/simulator tests and project checks pass. Four packer regression tests cover deterministic archive integrity, missing-input preservation, stale-image rejection and external-symlink rejection. Both repository skills pass frontmatter validation and an independent task review.

The previous visible failure was a conversation-length limit during repeated file-transfer segments, not evidence of a target compiler crash. [Publishing instructions](PUBLISHING.md) now require changed-file-only uploads and forbid chunk/bridge retry loops. A complete-file transfer of each finished JPEG succeeded with its Git blob SHA verified, without payloads entering model context. Automatic CI remains one Linux desktop job; CAD/firmware workflows stay manual. Target compilation and all physical acceptance remain unverified; no Arduino setup/build was run for this presentation change.

## Desktop firmware workbench (17 November 2026)

Branch `firmware/desktop-workbench`. The simulator now runs the firmware rather than imitating it, real audio reaches the shared detector, the character gained five poses and the screen gained a quest ring, banners, a session summary and tuner cues. Save record version 1 is unchanged; pins, BOM, CAD geometry and the guide PDFs are unchanged.

- `firmware/VEX_Pocket_Rival/app.h`: `vex::App` owns controls/debounce, calibration median, notices, auto-save, the ordered two-slot write through a byte `Store`, and presentation-only session facts. The `.ino` is adapters (I2S capture, GPIO, EEPROM, GC9A01A, USB serial).
- `firmware/VEX_Pocket_Rival/analyzer.h`: `Analyzer::push` (interrupt side: level accumulate and block copy) and `Analyzer::analyze` (foreground YIN, 2048-sample window, 55–1400 Hz). Default on the target through a one-input `AudioStream`; `VEX_PITCH_PJRC` restores the PJRC nodes for bench comparison. Telemetry adds `yin_us`/`yin_max_us`.
- `simulator/`: new line protocol (`press`, `audio`, `wait`, `wav`, `raw`, `serial`, `powercut`, ...) for the native `build/vex-sim`. The HTTP page built in this step was replaced the same day by the WebAssembly workbench below. `simulator/frames.py` provides standard-library PNG helpers.
- Tests: `tests/scenarios/*.txt` + `tests/golden/*.png` (pixel-exact, `--update-golden`), analyzer cases in `core_tests.cpp`. `tools/render_gallery.py` writes `assets/screens/`.
- Toolchain fix: `tools/firmware.py` wrote the config key `boards_manager`; arduino-cli expects `board_manager`, so the PJRC index was never registered and `core install` failed. This, not a 403, is the likely reason earlier setup attempts failed. Versions are unchanged.
- CAD environment note: `pip install -r requirements-cad.txt` on Python 3.13 pulled `ocp_gordon` 0.3.1, which imports `OCP.collections` and fails with cadquery-ocp 7.8.1; `pip install ocp_gordon==0.2.2` afterwards made build123d 0.10.0 import. `requirements-cad.txt` was not changed. `make hero` then re-rendered the hero/share images (texture now shows the quest ring) and refreshed provenance; geometry is identical.

Commands run: `make test` (before and after, with Apple clang and `CXX=g++-16`; golden frames identical across compilers), `make setup-firmware`, `make firmware-all` (baseline on the unchanged sketch, then after the refactor, then final), a stage 4 compile with `VEX_PITCH_PJRC`, `python3 tools/make_sprites.py`, `python3 tools/render_screen.py`, `python3 tools/render_gallery.py`, `python3 tests/simulator_test.py --update-golden`, `make hero`. Logs stayed in `build/`.

Still desktop-only: SGTL5000 line-in gain and the attenuator, pickup signals with real harmonics and decay, detector latency and `yin_us` cost at 600 MHz, display SPI timing, EEPROM timing, USB noise. Mac input and the −20 dBFS reference WAV are not codec evidence.

## In-browser WebAssembly workbench (17 November 2026)

Branch `sim/wasm-workbench`. The HTTP/PNG-polling simulator page (`simulator/server.py`, old `simulator/index.html`) is removed. `make sim` now compiles `app.h`, `analyzer.h` and `ui.h` to WebAssembly through `simulator/web/vex_web.cpp` (Emscripten 7.0.0 from Homebrew; `tools/web.py` requires 7.0+), serves `build/web/` on 127.0.0.1:8765, rebuilds on save and reloads the page. The page (`simulator/web/index.html`, `app.js`, `worklet.js`) steps the real `vex::App` one millisecond at a time in the sketch's loop order at 1×/10×/100×, holds controls from pointer or keyboard, blits the shared renderer's frame every 200 ms like the pedal (or every frame), runs synthetic observations, a synth note, the bundled recording or Mac input (AudioWorklet, 44.1 kHz) through the shared analyzer, shows state, telemetry, both EEPROM slots, power-cut control, and records/replays scenarios. Saves persist in `localStorage`.

- Tuner UX in shared `ui.h`/`app.h`: states waiting, listening, uncertain, sounding, stale (1.5 s fade of the last confident note) and clipping, each labelled; a signal meter against the gate under the title; `UI_FADE` mid-tone for faded text. `App` gained `uncertain`, `last_pitch`, `last_pitch_at`; `core.h` names `PITCH_VALID`/`PITCH_UNCERTAIN` (0.90/0.60; the valid threshold is unchanged). Save record version 1, pins, the `.ino` and the audio callback are unchanged.
- `make test` now ends with `tools/web.py check`: builds the module and compares `pet_listening` and `tuner_sharp` frames under Node with the native goldens, pixel for pixel; it prints `SKIP` without `em++`/`node` (Linux CI).
- Tested here: `make test` (core tests, 8 scenarios, 29 goldens regenerated after the tuner change and reviewed as a contact sheet, pack test, project check, WASM check PASS); `make firmware-all` after the header changes: all four stages PASS, no upload; a headless-Chrome DevTools script exercised the page end to end (hold-QUEST calibration refusing a loud floor, calibrating at gate 0.012, footswitch start, recorder output, `practice_quest.txt` and `stages_and_tuner.txt` replayed in the browser with all expectations met, synth A2 +20 cents detected as midi 45 within 6 cents by YIN, WAV E2 detected while practice credited, save, power cut mid-save → `SAVE FAILED`, reboot restored the earlier slot, `localStorage` populated). Mac microphone capture was not exercised headlessly; it needs a user gesture in a real browser.
- Follow-up pass (same day): a headless-Chrome run drove every control through real pointer/keyboard events and buttons (51 checks: clock/speed/pause/step, held readout, goal taps, calibration refusing a loud floor then succeeding on silence, fragment and banner, clipping/stale/tuner earn nothing, synth E2 −30 cents detected, fade, uncertain band, WAV at 1× and 100×, fake microphone at the real-time block rate, summary card, hold-MODE save, EEPROM tab, armed power cut → `SAVE FAILED`, reboot restoring the earlier slot, `localStorage` across a page reload, stage banners, state/log/scenario tabs, recorder, runner pass and fail, true-size toggle, live reload, no console errors). Fixes from it: the rebuild watcher now survives files vanishing during a git checkout (it had died silently), an inline favicon removes a 404, and the idle auto-save no longer flashes "Already saved" every minute when nothing changed (`App::save(bool manual)`; `saves.txt` covers it). `make firmware-all` passes after that `app.h` change.
- Known limits: the mic path is real-time only (faster speeds starve it and samples go stale, which is the correct firmware response); analysis time shown is this machine's; no CPU, codec, SPI or EEPROM timing is emulated. `python3 tools/web.py serve` must be started from a terminal that stays open (macOS kills it with its parent shell).

## Art pipeline and motion (17 November 2026)

Branch `art/pipeline`. Character art, colours, animation timing and fonts moved out of code into `art/` (PNG strips + JSON), imported by a standard-library validator into generated headers; the strum jitter is fixed by locking animation to the display cadence; the Teensy adapter sends only changed rows. Contract and drawing guidance: [ART.md](ART.md); figure `docs/figures/art-pipeline.svg`; skill `.agents/skills/vex-art-pipeline`.

- `art/theme.json` (13 colours, 7 UI roles), `art/vex/manifest.json` (idle 2 × 1000 ms, blink 3 × 100 one-shot, strum 4 × 100, cheer 2 × 200, levelup 2 × 200, listen 1, sleep 2 × 1000), `art/vex/*.png`, `art/fonts/tiny.png` (byte-identical to the previous `tinyfont.h`), `art/fonts/display.png` (DejaVu Sans Bold 9 px in 8 × 12 cells). The eight previous poses were exported pixel-identically and became frames of these animations; new frames are a wind-up/down/follow-through/rest strum, a 1 px breath, a three-step blink, a cheer bounce and a level-up sparkle twinkle.
- `tools/make_sprites.py` is now the importer (`tools/pngio.py` decodes/encodes PNG without Pillow). It writes `theme.h`, `sprites.h` (16 frames, 6 sprite colours), `anim.h`, `tinyfont.h`, `displayfont.h` (proportional width table) and `assets/vex_sheet.png`; `--check` verifies without writing. `tests/art_test.py` is in `make test`. `assets/vex_*.png` and `assets/sprites.json` previews are removed (the strips in `art/` are the source).
- `ui.h`: `chooseAnim` reads `anim.h`; `drawScreen` quantises `now` to `DISPLAY_MS` (new in `config.h`, 100) and pulls timestamps inside the current window back to it; continuous 4 px quest arc with an ink/gold lead pulse every 500 ms; display font for titles, note names and banners; colours come from `theme.h`.
- `flush.h`: `bands()` merges dirty rows (≤ 8 bands, 4-row gap). `.ino`: `DMAMEM uint16_t shadow[240*240]`, per-band `setAddrWindow`/`writePixels`, whole frame on the first draw, telemetry `tx_bytes`/`tx_max_bytes`. Host measurements through the shared renderer: blink step 2,400 B, strum step 31,200 B, breath 60,000 B, unchanged screen 0 B, full frame 115,200 B.
- Workbench: `tools/web.py` watches `art/**` and runs the importer before `em++`; the cadence label reads `vex_display_ms()`; the pose readout follows the manifest order.
- Tests: `core_tests.cpp` adds cadence-locked strum stepping, blink one-shot bounds, quantisation and band-diff cases (unchanged → 0 bands, blink < 15 % of rows, caption band ≥ row 180, eight-band cap covering every change). `personality.txt` captures four strum frames (`strum_frame_a..d`); 31 goldens regenerated and reviewed as a contact sheet. `make hero` re-rendered the hero/share images for the new texture; geometry unchanged.
- Motion evidence: the simulator's state JSON now reports `anim`/`frame`; `personality.txt` asserts strum frames 2→3→0→1 one cadence apart, cheer at the claim, level-up at level 2 and sleep after 31 s; `tools/web.py check` also compares the WASM build's two consecutive strum frames with the native goldens. `make motion` (`tools/render_motion.py`, ffmpeg) drives an eight-segment storyboard at `DISPLAY_MS`, fails if a segment shows a wrong animation or a loop skips a frame, and writes `assets/motion/vex-motion.mp4` (480 × 540, 10 fps, 15.8 s, 140 KB), `vex-motion.gif` (76 KB, embedded in the README and ART.md), `storyboard.png` and `storyboard.json`. Captions use the firmware display font. This is renderer evidence on the simulator clock, not panel timing.
- Stage 4 target compile after the change: 102,920 B code (+2,112), RAM1 variables 55,232 B, RAM2 variables 149,440 B (shadow 115,200 B in RAM2; audio memory and the canvas are heap). Not a bench result: SPI time per band, visible tearing between bands and whether 100 ms holds are unmeasured. If `tx_max_bytes` and loop timing show the bus saturating, set `DISPLAY_MS` to 200 and re-run `python3 tools/make_sprites.py`.

## Exact next actions

1. For desktop exploration, run `make sim` (needs `brew install emscripten`) and work through the walkthrough in `docs/LOCAL_DEVELOPMENT.md`; try Mac input with a real guitar interface at 1× speed. Before hardware bring-up, re-run `make firmware-all` on the final tree (passes today) and keep the report with the bench notes.
2. Order the bench electronics from `docs/SHOPPING.md`. Confirm Teensy 4.1, Audio Adapter D/D2 and Adafruit 6178 identities.
3. Meet the mentor with the guide and actual parts. Prove stage 1 and stage 2 before audio wiring. Test display readability at the intended pedalboard position.
4. Measure header gap, audio-board revision/component heights, jack body/lugs/thread, USB plug, button stack and footswitch travel. Enter actual values in `hardware/data/measurements.csv`.
5. Update CAD if needed, regenerate/export/check, then print the fit coupon. Order/cut the metal plate and window only after fit decisions.
6. Wire the attenuator, validate stages 3/4, then enclose and complete `acceptance_log.csv`. Test structural loading with electronics removed before foot operation.
7. At stage 2 on the bench, watch the idle breath, blink and strum at the 100 ms cadence and read `tx_bytes`/`tx_max_bytes`; look for tearing at band edges. Then at stage 3/4, record `yin_us`, `yin_max_us` and `cpu` from the serial telemetry with the reference WAV and a real guitar; compare note/cents against the `VEX_PITCH_PJRC` build on the same source. Adjust `Analyzer::THRESHOLD` or the window only with that evidence.

## Critical unknowns

- Header seating is modeled at 9.4 mm; exact socket/pin combinations and access to used pins need bench confirmation.
- Audio board community CAD may differ from purchased D2. The three mount holes and component clearance must be measured.
- Simplified switch/jack bodies do not certify solder-lug or inserted-plug clearance.
- The 32.4 mm screen may be too small for a standing tuner; pet charm can work seated while tuner legibility needs its own test.
- No force rating exists for the printed enclosure. Metal plate/column contact and layer adhesion require a mentor-defined load test.
- Actual pitch stability, ADC gain, USB noise and power-loss behavior have not been characterized.

## Source map and continuation prompt

`AGENTS.md` maps authoritative files. `docs/DECISIONS.md` explains constraints. `docs/PLAN.md` has milestone acceptance. `docs/RESEARCH.md` links the primary sources behind the agent setup and toolchain.

Suggested next-agent prompt:

> Read AGENTS.md and docs/HANDOFF.md. Continue PedalPartner R1.1 without redesigning the product. Start with the requested task and reuse evidence for unchanged work. Keep output short; do not run optional firmware/CAD builds unless the task needs them. Before physical bring-up, verify all four Teensy stages. Preserve the real display size, serviceable CAD groups, USB-only power and buffered dry-input architecture. Report measured facts separately from modeled assumptions. Update the handoff with results and the exact next step.

Repo-local skills are in `.agents/skills/`. Pi can discover that directory after project trust; invoke `/skill:vex-cad-review` or `/skill:vex-firmware-change`. Other agents can read the same SKILL.md directly if their discovery convention differs. Do not import private global session history into this public repository.
