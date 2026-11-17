# Build and test locally before connecting hardware

## 1. Desktop tests and simulator

On macOS:

```sh
xcode-select --install
# If Python 3 is not installed, install it with your usual package manager.
git clone https://github.com/ash0ts/PedalPartner.git
cd PedalPartner
make test
make sim
```

Use Python 3.10+ and a C++17 compiler. Linux needs `python3`, `make` and `g++`. Host tests require no third-party Python packages and no Arduino installation. Open http://127.0.0.1:8765; `make sim PORT=8766` selects another port. Stop with Ctrl-C.

The simulator runs the same `vex::App` state machine the Teensy compiles (`firmware/VEX_Pocket_Rival/app.h`): debounce and 1.2 s holds, the three-second calibration median, notices, the idle auto-save and the ordered two-slot save. The page's MODE/QUEST/footswitch respond to press duration like the real controls. The first useful walkthrough:

1. Select stage 4, hold **QUEST** (or press **Calibrate** for a known quiet baseline), then tap the footswitch to start.
2. Choose **Playing**. Advance five minutes. Expect 50 participation XP, a full quest ring and one fragment with a three-second banner.
3. Keep playing: XP continues but that session does not grant a second fragment. Ten active minutes shows the level-up banner.
4. Choose silence, clipping or stale samples: practice time must stop. Tap MODE for the tuner: practice time also stops.
5. Tap the footswitch to pause: the session summary shows time and XP earned this session. Hold MODE to save, then Reboot. Total time/fragments return; the new boot is paused and uncalibrated. Corrupt latest save tests fallback to the previous valid record.
6. Press **Play test recording** or **Start Mac input** to run real audio through the shared analyzer (below).

The round image is the actual shared 240 × 240 renderer. Browser scaling makes it larger than the physical 32.4 mm screen. Use the printed footprint/actual display for readability decisions.

### Real audio on the desktop

`firmware/VEX_Pocket_Rival/analyzer.h` is the level meter and YIN pitch estimator the Teensy compiles. The simulator feeds it 128-sample blocks at 44.1 kHz from `test_audio/guitar_notes_left_only.wav` (the `wav` command), raw 16-bit mono PCM (`raw`), or the browser's microphone/line input (`/live`). The same block RMS/peak values gate practice time, so the recording's leading silence earns nothing and the −20 dBFS tones do. This is Mac audio: the SGTL5000 line-in gain, the attenuator, rig noise and real latency remain bench measurements. Define `VEX_PITCH_PJRC` in `config.h` to build the PJRC `AudioAnalyzeNoteFrequency` path for a bench comparison.

| Tested on desktop | Still needs hardware |
|---|---|
| Practice timing, thresholds, goal changes, one-time rewards | SGTL5000 capture, input levels and rig noise |
| Shared screen layout, sprites, ring, banners, summary and state transitions | GC9A01A SPI timing, orientation, glare and standing readability |
| Pitch-to-note/cents math; YIN detector on synthetic tones, harmonics, noise and the reference WAV | Real guitar pickups, octave behaviour on real strings, latency and CPU cost on the target |
| CRC records, reboot, corrupt-record fallback and power cuts at several bytes of the ordered write | EEPROM power-loss behavior and endurance |
| Debounce/hold logic, calibration median and all hold actions through the shared App | Switch bounce, GPIO wiring and actual stage-2 readings |

The simulator's two-slot save file is `build/sim-progress.bin`; stop the server and delete that file to start fresh, or press **Factory reset**. `make clean` removes the entire disposable `build/` directory. Nothing is uploaded by these commands.

### Scenario scripts and golden frames

`tests/scenarios/*.txt` drive `build/vex-sim` through its line protocol and are run by `make test`:

| Command | Effect |
|---|---|
| `press <mode or quest or foot> <ms>` | Hold a control for `ms`, then release and settle 30 ms |
| `audio <ms> <rms> <peak> [hz prob]` | Inject fresh level (and optional pitch) observations for `ms` |
| `wait <ms>` | Advance time with no fresh samples (they go stale) |
| `wav <path> [start_ms length_ms]`, `raw <path>` | Feed PCM through the shared analyzer |
| `serial <c, s, p or r>` | The sketch's USB debug commands |
| `calibrate [rms]` | Serial `c` plus 3.1 s of a quiet floor (default 0.001) |
| `stage <2, 3 or 4>`, `reboot`, `reset`, `corrupt`, `powercut <bytes>`, `save`, `state` | Device lifecycle; `powercut` kills storage after N byte writes |
| `expect key=value ...`, `golden <name>` | Test directives: field checks (`key~=` allows 200 ms slack) and pixel-exact frame comparison |

After an intentional UI change run `python3 tests/simulator_test.py --update-golden`, review the PNGs in `tests/golden/`, and commit them. `python3 tools/render_gallery.py` writes every key state to `assets/screens/` plus a contact sheet when Pillow is available.

## 2. Compile firmware with no board attached

Install [Arduino CLI](https://arduino.github.io/arduino-cli/). On macOS with Homebrew:

```sh
brew install arduino-cli
make setup-firmware
make firmware-all
# Later, to compile one stage:
make firmware STAGE=4
```

The script uses an isolated `.arduino/` directory, preserving your normal Arduino installation. Initial setup needs internet access. Pins are Teensy core **1.62.0**, Adafruit BusIO **1.17.4**, GFX **1.13.0**, GC9A01A **1.2.0**. The optional manual firmware workflow uses Arduino CLI **1.6.0**. Exact FQBN and versions live in `tools/firmware.py`; the Teensy package index is the official PJRC URL.

Outputs are `build/stage-N/output/` and `build/target_build_report.json`. The original sketch stays unchanged: each stage is compiled from a copy with `VEX_STAGE` set to 1, 2, 3 or 4. These targets invoke **compile only**, never upload. The manual **Firmware build (manual)** workflow can do the same on GitHub and retains reports/binaries for seven days. It does not run on pushes or pull requests. Automatic CI runs only `make test` on Linux (five-minute timeout); **CAD rebuild (manual)** is also opt-in. For everyday work, start with `make test` and `make sim`; install target or CAD tools only when you need them.

| Stage | Purpose | Physical success criterion |
|---|---|---|
| 1 | USB / serial / onboard LED | `VEX USB OK`, stable power, LED blink |
| 2 | Screen / buttons / status LED | Clear picture; each M/Q/F reading changes only for its own switch |
| 3 | Codec and level telemetry | `codec=1`; known low-level source produces RMS/peak; no clipping |
| 4 | Pet / practice / tuner / saves | Calibrated gating, quest, save/reboot and reference-note checks |

## 3. Upload only when the bench is ready

Install Arduino IDE 2 and Teensy support using [PJRC's instructions](https://www.pjrc.com/teensy/td_download.html). Install the same three display-library versions through Library Manager. Open `firmware/VEX_Pocket_Rival/VEX_Pocket_Rival.ino`, choose **Teensy 4.1**, USB type **Serial**, 600 MHz, and select the board's USB port.

Start with `VEX_STAGE 1` in `config.h`. Use **Verify** first, then **Upload** when the mentor has checked the unpowered wiring. Follow stages in order, editing the number and repeating Verify/Upload after each passing checkpoint. The physical Program button can recover entry to the Teensy loader. Use a data-capable Micro-B cable, not a charge-only cable.

The USB 5 V path powers the Teensy; no 9 V connection exists. Begin audio tests with the trimmer at minimum signal. Confirm terminal identities with a meter. Use the circuit and pin table in the [build guide](BUILD_GUIDE.md), not a generic Teensy tutorial with different hardware.

The [left-only test WAV](../test_audio/README.md) can exercise reference notes through a known line source. Start with the source volume down; digital WAV amplitude does not establish analog volts. Confirm actual amplitude before raising the trimmer. This is hardware-in-the-loop testing, not an emulator.

## Editing and regeneration

- Rules/persistence: edit `core.h`, add a relevant regression, `make test` and `make firmware-all`.
- Device behaviour (controls, calibration, notices, saves): edit shared `app.h`, add or extend a scenario in `tests/scenarios/`, `make test`, then compile the target.
- Audio analysis: edit shared `analyzer.h`, extend the synthetic cases in `tests/core_tests.cpp` or `tests/scenarios/real_audio.txt`, then compile all stages and read `yin_us`/`cpu` in the stage 3/4 serial telemetry on the bench.
- Layout: edit shared `ui.h`, inspect states with `make sim` or `python3 tools/render_gallery.py`, update goldens, then compile the target.
- Character: edit `tools/make_sprites.py`, run `python3 tools/make_sprites.py`, then `make sim-build`, `python3 tools/render_screen.py` and update goldens. Pillow is needed for sprite generation. `render_screen.py` creates the CAD texture from the actual shared renderer; a changed texture requires `make hero` so provenance stays valid.
- Plain Markdown or agent-guidance edits: edit the relevant files and run `make test`; no CAD or Arduino dependencies are needed.
- CAD geometry or generated hardware guides: create a Python virtualenv, install `requirements-cad.txt`, then regenerate only affected outputs with `make cad cad-check`, `make render` (Blender) and `make docs`.
- Presentation only: `make hero` renders the existing STEP using Blender and the CAD/Pillow environment; [details](MARKETING.md). It does not compile firmware or rebuild geometry.
- Mentor handoff: `make pack-mentor` creates the lean review ZIP using standard-library Python and existing files; [contents](MENTOR_PACK.md). It performs no builds.

Record target build results and actual bench measurements separately. A green host test does not prove firmware compilation, and successful compilation does not prove the electrical build.
