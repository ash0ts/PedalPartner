# Current handoff — R1.1

Last prepared: 17 November 2026, evening (New York). Repository: [ash0ts/PedalPartner](https://github.com/ash0ts/PedalPartner).

## Where we left off

The user wants a feasible first pedal build with a mentor, retaining a rival-device / virtual-pet personality. Earlier concept images were appealing but too futuristic; the first practical CAD lost charm. R1.1 keeps the feasible mechanical/electrical architecture and transfers the color/character through small cosmetic changes. Do not return to a huge imaginary display or flatten the design into a generic box.

This is a **complete starter design package for review**, not a physically commissioned device. Target compilation of all four stages now passes locally; the next useful work is measured bench bring-up, followed by fit-coupon changes. Automatic CI covers only desktop tests. Re-verify target compilation after any firmware change before bench use; it is not a publication prerequisite.

## Evidence

| Check | State / evidence |
|---|---|
| Shared core unit tests | Passed locally; `make test` reproduces. Includes `App` calibration/notice/save checks and the YIN analyzer on synthetic tones, harmonic-rich and decaying tones, noise, silence and square waves |
| Native simulator integration | Passed: eight scenario scripts through the shared `App` (calibration variants, hold actions, goals, saves, torn writes at 0/10/50/52 bytes, stages, tuner, personality states) with 26 golden frames; the reference WAV yields E2 A2 D3 G3 B3 E4 A4 through the shared detector and credits practice only while the tones sound |
| Target Teensy compilation | **Passed locally on 17 Nov 2026**: arduino-cli 1.6.0 (Homebrew), Teensy core 1.62.0, pinned Adafruit libraries; all four stages with the shared analyzer and stage 4 with `VEX_PITCH_PJRC`. Stage 4: 100,808 B code, 46,016 B RAM1 variables, no warnings from project sources. Report in `build/target_build_report.json` (ignored). A compile is not a bench result. |
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
- `simulator/`: new line protocol (`press`, `audio`, `wait`, `wav`, `raw`, `serial`, `powercut`, ...), press-and-hold browser controls, test-recording playback and a Mac-input path (`/live`, raw PCM ≤ 1 s per POST). `simulator/frames.py` provides standard-library PNG helpers.
- Tests: `tests/scenarios/*.txt` + `tests/golden/*.png` (pixel-exact, `--update-golden`), analyzer cases in `core_tests.cpp`. `tools/render_gallery.py` writes `assets/screens/`.
- Toolchain fix: `tools/firmware.py` wrote the config key `boards_manager`; arduino-cli expects `board_manager`, so the PJRC index was never registered and `core install` failed. This, not a 403, is the likely reason earlier setup attempts failed. Versions are unchanged.
- CAD environment note: `pip install -r requirements-cad.txt` on Python 3.13 pulled `ocp_gordon` 0.3.1, which imports `OCP.collections` and fails with cadquery-ocp 7.8.1; `pip install ocp_gordon==0.2.2` afterwards made build123d 0.10.0 import. `requirements-cad.txt` was not changed. `make hero` then re-rendered the hero/share images (texture now shows the quest ring) and refreshed provenance; geometry is identical.

Commands run: `make test` (before and after, with Apple clang and `CXX=g++-16`; golden frames identical across compilers), `make setup-firmware`, `make firmware-all` (baseline on the unchanged sketch, then after the refactor, then final), a stage 4 compile with `VEX_PITCH_PJRC`, `python3 tools/make_sprites.py`, `python3 tools/render_screen.py`, `python3 tools/render_gallery.py`, `python3 tests/simulator_test.py --update-golden`, `make hero`. Logs stayed in `build/`.

Still desktop-only: SGTL5000 line-in gain and the attenuator, pickup signals with real harmonics and decay, detector latency and `yin_us` cost at 600 MHz, display SPI timing, EEPROM timing, USB noise. Mac input and the −20 dBFS reference WAV are not codec evidence.

## Exact next actions

1. For desktop exploration, run `make sim` and try the hold controls, test recording and Mac input. Before hardware bring-up, re-run `make firmware-all` on the final tree (passes today) and keep the report with the bench notes.
2. Order the bench electronics from `docs/SHOPPING.md`. Confirm Teensy 4.1, Audio Adapter D/D2 and Adafruit 6178 identities.
3. Meet the mentor with the guide and actual parts. Prove stage 1 and stage 2 before audio wiring. Test display readability at the intended pedalboard position.
4. Measure header gap, audio-board revision/component heights, jack body/lugs/thread, USB plug, button stack and footswitch travel. Enter actual values in `hardware/data/measurements.csv`.
5. Update CAD if needed, regenerate/export/check, then print the fit coupon. Order/cut the metal plate and window only after fit decisions.
6. Wire the attenuator, validate stages 3/4, then enclose and complete `acceptance_log.csv`. Test structural loading with electronics removed before foot operation.
7. At stage 3/4 on the bench, record `yin_us`, `yin_max_us` and `cpu` from the serial telemetry with the reference WAV and a real guitar; compare note/cents against the `VEX_PITCH_PJRC` build on the same source. Adjust `Analyzer::THRESHOLD` or the window only with that evidence.

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
