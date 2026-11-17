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

The first useful walkthrough:

1. Select stage 4, press **Calibrate** (a simulated known quiet baseline), then the footswitch to start.
2. Choose **Playing**. Advance five minutes. Expect 50 participation XP and one fragment for the five-minute quest.
3. Keep playing: XP continues but that session does not grant a second fragment.
4. Choose silence, clipping or stale samples: practice time must stop. Switch to tuner: practice time also stops.
5. Pause, Save, then Reboot. Total time/fragments return; the new boot is paused and uncalibrated. Corrupt latest save tests fallback to the previous valid record.

The round image is the actual shared 240 × 240 renderer. Browser scaling makes it larger than the physical 32.4 mm screen. Use the printed footprint/actual display for readability decisions.

| Tested on desktop | Still needs hardware |
|---|---|
| Practice timing, thresholds, goal changes, one-time rewards | SGTL5000 capture, input levels and rig noise |
| Shared screen layout, sprite and state transitions | GC9A01A SPI timing, orientation, glare and standing readability |
| Pitch-to-note/cents math with injected Hz | Actual pitch detection, octave errors and latency |
| CRC records, reboot and corrupt-record fallback | EEPROM power-loss behavior and endurance |
| Debounce/hold logic unit tests | Switch bounce, GPIO wiring and actual stage-2 readings |

The simulator's two-slot save file is `build/sim-progress.bin`; stop the server and delete that file to start fresh. `make clean` removes the entire disposable `build/` directory. Nothing is uploaded by these commands.

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
- Layout: edit shared `ui.h`, inspect practice/tuner/clipping/calibration views with `make sim`, then compile the target.
- Character: edit `tools/make_sprites.py`, run `python3 tools/make_sprites.py`, then `make sim-build` and `python3 tools/render_screen.py`. Pillow is needed for sprite generation. The last command creates the CAD texture from the actual shared renderer.
- Plain Markdown or agent-guidance edits: edit the relevant files and run `make test`; no CAD or Arduino dependencies are needed.
- CAD geometry or generated hardware guides: create a Python virtualenv, install `requirements-cad.txt`, then regenerate only affected outputs with `make cad cad-check`, `make render` (Blender) and `make docs`.
- Presentation only: `make hero` renders the existing STEP using Blender and the CAD/Pillow environment; [details](MARKETING.md). It does not compile firmware or rebuild geometry.
- Mentor handoff: `make pack-mentor` creates the lean review ZIP using standard-library Python and existing files; [contents](MENTOR_PACK.md). It performs no builds.

Record target build results and actual bench measurements separately. A green host test does not prove firmware compilation, and successful compilation does not prove the electrical build.
