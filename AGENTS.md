# PedalPartner agent contract

## Read first

1. `docs/HANDOFF.md` is the current checkpoint, including unverified items.
2. `docs/DECISIONS.md` explains choices; `docs/PLAN.md` lists the next work.
3. Read the relevant CAD, software or build document before changing that subsystem.

The product is VEX / Pocket Rival, a buildable first guitar companion with original pixel-bat personality. Preserve charcoal/crimson/muted gold, simple controls and the real small screen. Concept renders are not mechanical specifications.

## Commands from repository root

| Task | Command |
|---|---|
| Host logic, simulator, repository consistency | `make test` |
| In-browser workbench (firmware as WebAssembly; needs `em++`) | `make sim` |
| Compile the WebAssembly module only / compare it with native goldens | `make web` / `make web-check` |
| Install target dependencies | `make setup-firmware` |
| Compile all target stages; no upload | `make firmware-all` |
| Install CAD/docs packages in a virtualenv | `python3 -m pip install -r requirements-cad.txt` |
| Regenerate CAD and validate exported STEP | `make cad cad-check` |
| Assembly illustrations from existing meshes | `make render` |
| Studio hero from checked-in STEP; no CAD rebuild | `make hero` |
| Lean mentor review ZIP from existing files | `make pack-mentor` |
| Regenerate BOM, guide and templates | `make docs` |
| Import `art/` into the generated firmware headers / verify they match | `make art` / `make art-check` |
| Verify motion segment by segment and re-encode `assets/motion/` (needs ffmpeg) | `make motion` |

Python 3.10+ and C++17 suffice for host tests. CAD uses build123d 0.10.0. Target pins/versions are in `config.h` and `tools/firmware.py`. Do not silently upgrade them.

## Sources of truth

- Geometry: `hardware/cad/build_cad.py`; assembly ownership: `hardware/cad/assembly_tree.py`.
- Parts, wiring and procurement: `hardware/data/project_data.py`; regenerate derived CSVs/docs.
- Practice, pitch math and record layout: `firmware/VEX_Pocket_Rival/core.h`.
- Device behavior (controls, calibration, notices, saves): shared `app.h`; level/pitch analysis: shared `analyzer.h`.
- Display: shared `ui.h`; dirty-band flush `flush.h`. Sprites, animation timing, theme colours and fonts: `art/` (PNG + JSON), imported by `tools/make_sprites.py` into generated `theme.h`, `sprites.h`, `anim.h`, `tinyfont.h`, `displayfont.h`; never hand-edit those. See `docs/ART.md`.
- Physical integration: `.ino` (adapters only); native simulator and test oracle: `simulator/main.cpp`; browser adapter and page: `simulator/web/` (built by `tools/web.py`); scenario scripts and golden frames: `tests/scenarios/`, `tests/golden/`.
- Evidence: `hardware/generated/checks/`, `checks/`, GitHub Actions, measured CSV logs.

## Invariants

- USB 5 V only. Teensy GPIO is 3.3 V. No direct 9 V connection.
- Buffered dry signal through the documented attenuator to Audio Adapter LINE IN L. No passive-pickup loading, MIC substitution, speaker input, audio output or bypass claims.
- Preserve pin ownership. No SD cards; unused selects stay high.
- Boot paused and uncalibrated. Silence, stale samples, clipping and tuner mode earn no practice time. One fragment per completed session.
- Save-record changes require explicit versioning/migration and corruption/reboot tests. Avoid EEPROM writes in the audio callback.
- No display rendering, allocation or logging in the audio callback. Keep the shared renderer usable by both targets.
- A valid STEP is not a fit, strength or electrical certification. Never fill measured acceptance fields with inferred values.

## Keep validation and context small

- This is a personal prototype. Automatic CI is one Linux `make test` job. Firmware and CAD Actions are manual; do not trigger them just to publish or edit docs.
- Run checks relevant to the change once. Reuse recorded evidence for unchanged CAD and firmware; do not reinstall toolchains, regenerate all assets or repeat fresh-agent reviews on every handoff.
- Keep command output bounded. Redirect verbose builds to `build/` logs and inspect only a short error excerpt. Do not paste binary/base64 data or full build logs into chat.
- Publish only changed files on a review branch; reuse unchanged blobs from the base tree. Prefer ordinary Git. A connector transfer must pass one complete, bounded file internally, verify its hash, and emit only metadata. Never use repeated chunk reads, shared-memory/bridge segment loops or payloads in model context. If one complete transfer is unavailable, stop that method and provide the local patch/archive. See `docs/PUBLISHING.md`.
- For hero work, read `docs/MARKETING.md`: use actual STEP meshes and shared UI pixels; no generative imagery or invented hardware. `make hero` only tessellates/renders; `make pack-mentor` only packages. Neither requires target compilation. Do not overwrite assembly-guide illustrations for a marketing-only change.
- When a toolchain is unavailable, record that limit and continue with the recoverable checkpoint. Do not repeatedly retry downloads or wait on optional CI.
- Before a firmware upload to real hardware, compile the affected target stages and complete the documented bench checks. Manual CI is optional; local compilation is sufficient.

## Change discipline

Implement the user's authorized work. Keep changes focused and preserve unrelated work. Add tests for changed behavior or meaningful regressions; do not add tests that merely restate constants. Regenerate affected exports and record commands/results in the handoff. If a toolchain or physical test is unavailable, state exactly what remains untested.

Repository skills live in `.agents/skills/`: `vex-cad-review`, `vex-firmware-change` and `vex-art-pipeline`. Read the matching SKILL.md when it fits the task. Do not require personal accounts, global memory imports or a proprietary agent framework to work here.

At handoff, update the current status, exact next action, decisions, failed/blocked checks and any measured values. Keep historical narrative out of this contract. Do not publish credentials, private chat transcripts or unrelated personal/work material.

## Learned User Preferences

- Installing local toolchains on this machine (arduino-cli and the pinned Teensy core, emscripten, the CAD virtualenv) is approved; install what the work needs instead of asking first.
- Desktop tooling must be something the user can open and try by hand; when a workflow is judged unusable, research established patterns before redesigning rather than iterating on the current approach.

## Learned Workspace Facts

- On macOS, processes started from an agent shell (`nohup ... &`, subshells) are killed when that shell call ends and `setsid` does not exist; run long jobs such as `make hero` in the foreground with a long wait, or start servers with `subprocess.Popen(..., start_new_session=True)` and verify with `pgrep`.
- System Python has no Pillow; sprite generation, gallery contact sheets and CAD/hero work use the disposable `build/venv` (`build/venv/bin/python`), which `make clean` removes.
