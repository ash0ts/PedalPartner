# PedalPartner agent contract

## Read first

1. `docs/HANDOFF.md` is the current checkpoint, including unverified items.
2. `docs/DECISIONS.md` explains choices; `docs/PLAN.md` lists the next work.
3. Read the relevant CAD, software or build document before changing that subsystem.

The product is VEX / Pocket Rival, a buildable first guitar companion with original pixel-bat personality. Three readers share the repository: a hobbyist alone, a hobbyist with a mentor, and a design studio taking it to product (`docs/STUDIO_BRIEF.md`); keep the build guide usable by all three and keep agent-facing material out of their route. Preserve charcoal/crimson/muted gold, simple controls and the real small screen. Concept renders are not mechanical specifications.

## Commands from repository root

| Task | Command |
|---|---|
| Host logic, simulator, repository consistency | `make test` |
| In-browser workbench (firmware as WebAssembly; needs `em++`) | `make sim` |
| Workbench with a Teensy mirrored in the Device tab (needs pyserial) | `make sim DEVICE=auto` |
| Compile the WebAssembly module only / compare it with native goldens | `make web` / `make web-check` |
| Install target dependencies | `make setup-firmware` |
| Compile all target stages; no upload | `make firmware-all` |
| List PJRC serial ports / program one compiled stage / read telemetry and link packets | `make ports` / `make upload STAGE=n [SERIAL=/dev/cu.…]` / `make monitor [SEND=t]` |
| Compile, upload and open the Device tab in one go / summarise the latest bench log | `make bench STAGE=n [SERIAL=…]` / `make bench-report [LOG=…]` |
| Measured parts against CAD assumptions; which STL/DXF files may be fabricated now | `make measure-check` |
| Install CAD/docs packages in a virtualenv | `python3 -m pip install -r requirements-cad.txt` |
| Regenerate CAD and validate exported STEP | `make cad cad-check` |
| Assembly illustrations from existing meshes | `make render` |
| Studio hero from checked-in STEP; no CAD rebuild | `make hero` |
| Review ZIP for a hobbyist/mentor, or the studio pack with the product brief | `make pack-mentor` / `make pack-studio` |
| Regenerate BOM, guide and templates | `make docs` |
| Import `art/` into the generated firmware headers / verify they match | `make art` / `make art-check` |
| Verify motion segment by segment and re-encode `assets/motion/` (needs ffmpeg) | `make motion` |

Python 3.10+ and C++17 suffice for host tests. CAD uses build123d 0.10.0. Target pins/versions are in `config.h` and `tools/firmware.py`. Do not silently upgrade them.

## Sources of truth

- Geometry: `hardware/cad/build_cad.py`; assembly ownership: `hardware/cad/assembly_tree.py`.
- Parts, wiring and procurement: `hardware/data/project_data.py`; regenerate derived CSVs/docs. Measurement-to-CAD mapping and fabrication gating: `MEASUREMENTS` and `CUT_PROFILES` there, read by `tools/measure_check.py`; measured values live only in `hardware/data/measurements.csv` and `acceptance_log.csv`, entered by hand.
- Practice, pitch math and record layout: `firmware/VEX_Pocket_Rival/core.h`.
- Device behavior (controls, calibration, notices, saves): shared `app.h`; level/pitch analysis: shared `analyzer.h`.
- Display: shared `ui.h`; dirty-band flush `flush.h`. Sprites, animation timing, theme colours and fonts: `art/` (PNG + JSON), imported by `tools/make_sprites.py` into generated `theme.h`, `sprites.h`, `anim.h`, `tinyfont.h`, `displayfont.h`; never hand-edit those. See `docs/ART.md`.
- Bench link (band/state packets, CRC-32, 76-byte `LinkState`): shared `link.h` and its Python mirror `tools/link.py` (also `BenchLog`, the `build/bench/*.jsonl` writer); telemetry JSON, serial switches, live knobs (`- + g G t`) and the `Panel` adapter: `.ino`; test pattern: `ui.h` `drawTestPattern`; upload/monitor: `tools/firmware.py`; serial bridge and Device tab: `tools/web.py`, `simulator/web/`; session summary: `tools/bench_report.py`. Knob boot values stay in `config.h`.
- Physical integration: `.ino` (adapters only); native simulator and test oracle: `simulator/main.cpp`; browser adapter and page: `simulator/web/` (built by `tools/web.py`); scenario scripts and golden frames: `tests/scenarios/`, `tests/golden/`.
- Evidence: `hardware/generated/checks/`, `checks/`, GitHub Actions, measured CSV logs.

## Invariants

- USB 5 V only. Teensy GPIO is 3.3 V. No direct 9 V connection.
- Buffered dry signal through the documented attenuator to Audio Adapter LINE IN L. No passive-pickup loading, MIC substitution, speaker input, audio output or bypass claims.
- Preserve pin ownership. No SD cards; unused selects stay high.
- Boot paused and uncalibrated. Silence longer than the activity hold, stale samples, clipping and tuner mode earn no practice time. One fragment per completed session.
- Save-record changes require explicit versioning/migration and corruption/reboot tests. Avoid EEPROM writes in the audio callback.
- No display rendering, allocation or logging in the audio callback. Keep the shared renderer usable by both targets.
- A valid STEP is not a fit, strength or electrical certification. Never fill measured acceptance fields with inferred values.

## Keep validation and context small

- This is a personal prototype. Automatic CI is one Linux `make test` job. Firmware and CAD Actions are manual; do not trigger them just to publish or edit docs.
- Run checks relevant to the change once. Reuse recorded evidence for unchanged CAD and firmware; do not reinstall toolchains, regenerate all assets or repeat fresh-agent reviews on every handoff.
- Keep command output bounded. Redirect verbose builds to `build/` logs and inspect only a short error excerpt. Do not paste binary/base64 data or full build logs into chat.
- Publish only changed files on a review branch; reuse unchanged blobs from the base tree. Prefer ordinary Git. A connector transfer must pass one complete, bounded file internally, verify its hash, and emit only metadata. Never use repeated chunk reads, shared-memory/bridge segment loops or payloads in model context. If one complete transfer is unavailable, stop that method and provide the local patch/archive. See `docs/PUBLISHING.md`.
- For hero work, read `docs/MARKETING.md`: use actual STEP meshes and shared UI pixels; no generative imagery or invented hardware. `make hero` only tessellates/renders; `make pack-mentor` / `make pack-studio` only package. Neither requires target compilation. Do not overwrite assembly-guide illustrations for a marketing-only change.
- When a toolchain is unavailable, record that limit and continue with the recoverable checkpoint. Do not repeatedly retry downloads or wait on optional CI.
- Before a firmware upload to real hardware, compile the affected target stages and complete the documented bench checks. Manual CI is optional; local compilation is sufficient.

## Change discipline

Implement the user's authorized work. Keep changes focused and preserve unrelated work. Add tests for changed behavior or meaningful regressions; do not add tests that merely restate constants. Regenerate affected exports and record commands/results in the handoff. If a toolchain or physical test is unavailable, state exactly what remains untested.

Repository skills live in `.agents/skills/`: `vex-cad-review`, `vex-firmware-change` and `vex-art-pipeline`. Read the matching SKILL.md when it fits the task. Do not require personal accounts, global memory imports or a proprietary agent framework to work here.

At handoff, update the current status, exact next action, decisions, failed/blocked checks and any measured values. Keep historical narrative out of this contract. Do not publish credentials, private chat transcripts or unrelated personal/work material.

## Learned User Preferences

- Installing local toolchains on this machine (arduino-cli and the pinned Teensy core, emscripten, the CAD virtualenv) is approved; install what the work needs instead of asking first.
- Desktop tooling must be something the user can open and try by hand; when a workflow is judged unusable, research established patterns before redesigning rather than iterating on the current approach.
- Verify delivered work end to end and show the evidence (drive the workbench through real pointer/keyboard events in headless Chrome, assert animations from simulator state, render a video), then leave the workbench running with short try-it steps so the user can test it by hand, and state plainly what can only be proven on the Teensy bench.
- Finished work goes to `main` through a PR merged (merge commit, branch deleted) as soon as the Linux `make test` CI job passes; do not wait to be asked before merging.

## Learned Workspace Facts

- On macOS, processes started from an agent shell (`nohup ... &`, subshells) are killed when that shell call ends and `setsid` does not exist; run long jobs such as `make hero` in the foreground with a long wait, or start servers with `subprocess.Popen(..., start_new_session=True)` and verify with `pgrep`.
- System Python has no Pillow; sprite generation, gallery contact sheets and CAD/hero work use the disposable `build/venv` (`build/venv/bin/python`), which `make clean` removes.
- Headless Chrome (`/Applications/Google Chrome.app/Contents/MacOS/Google Chrome --headless=new`, driven over CDP) can exercise the workbench: open it with `?noreload=1` so the live-reload long-poll does not hang virtual time, and auto-accept `Page.javascriptDialogOpening` because Factory reset uses `confirm()`. Mac microphone capture still needs a real browser gesture.
- The Homebrew ffmpeg on this machine has no `drawtext` filter (no libfreetype); `tools/render_motion.py` draws video captions with the firmware display font and uses ffmpeg only to scale and encode.
