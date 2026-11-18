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

Use Python 3.10+ and a C++17 compiler. Linux needs `python3`, `make` and `g++`. `make test` requires no third-party Python packages and no Arduino installation. `make sim` additionally needs Emscripten (`brew install emscripten`; the workbench was built with 7.0.0) because the browser runs the firmware itself. Open http://127.0.0.1:8765; `make sim PORT=8766` selects another port. Stop with Ctrl-C.

### The in-browser workbench

`make sim` compiles the shared headers (`app.h`, `analyzer.h`, `ui.h`) to WebAssembly through the adapter `simulator/web/vex_web.cpp`, serves `simulator/web/` from `build/web/`, rebuilds when any of those files change and reloads the page. Nothing round-trips through a server while you use it: the page owns a millisecond clock and steps the real `vex::App` in the sketch's loop order (observe, controls, tick), so debounce, 1.2 s holds, the calibration median, the idle auto-save and the ordered two-slot save behave exactly as on the Teensy and exactly as in `make test`.

- **Controls**: press and hold MODE, QUEST and the footswitch with the pointer or keys **M**, **Q**, **Space**; the held time is shown. Stage 2 and 3 show a banner explaining that controls are inert there by design.
- **Clock**: 1×, 10× or 100× real time, Pause (**P**), Step 1 s, Advance 5 min. Reboot restarts at t = 0 like the device; Factory reset zeroes both EEPROM slots.
- **Display**: the shared 240 × 240 renderer blitted every `DISPLAY_MS` (100 ms) like the pedal (toggle for every frame), with a true-size 32.4 mm option for readability decisions. The pose name is shown under the device.
- **Audio** tab: synthetic observations (the same `audio` semantics as scenarios), a synth note with detune and level through the real YIN analyzer, the bundled 24 s test recording fed at simulated time, or Mac input through an AudioWorklet at 44.1 kHz. The input meter shows RMS against the gate, peak with clipping, the estimator's Hz/probability and the analysis time on this machine.
- **State**, **EEPROM**, **Log** tabs: the native simulator's JSON fields, both slots decoded and hex-dumped with validity, a power-cut-after-N-writes control, a telemetry line per simulated second in the serial-monitor shape and the notice history. Saved bytes persist in the browser like EEPROM.
- **Scenario** tab: a recorder turns your session into `press`/`audio`/`wait` protocol lines to paste into `tests/scenarios/`, and a runner replays scenario text (including `expect` lines) against the same module.
- **Device** tab: the bench mirror described in [section 4](#4-bench-loop-with-a-teensy-attached). Without `--device` it says so and stays quiet.

Walkthrough:

1. Hold **QUEST** with the Playing signal selected: calibration refuses ("Too loud"). Select Silence, hold QUEST again, then tap the footswitch and select Playing.
2. Advance five minutes. Expect 50 participation XP, a full quest ring and one fragment with a three-second banner. Ten active minutes shows the level-up banner.
3. Choose silence, clipping or stale: practice time stops. Tap MODE for the tuner: practice time also stops. Try the synth note with detune to see flat/sharp/in-tune, release it to see the reading fade, and lower the level until the tuner reports uncertain.
4. Tap the footswitch to pause: the session summary shows this session's time and XP. Hold MODE to save, then Reboot: totals return; the new boot is paused and uncalibrated. Arm a power cut, save again and reboot: the earlier slot still loads.
5. Open the Scenario tab, copy the recording, add `expect`/`golden` lines and run `make test`.

### Real audio on the desktop

`firmware/VEX_Pocket_Rival/analyzer.h` is the level meter and YIN pitch estimator the Teensy compiles. Both simulators feed it 128-sample blocks at 44.1 kHz: the native one from `test_audio/guitar_notes_left_only.wav` (the `wav` command) or raw 16-bit mono PCM (`raw`), the browser from the same recording, a generated note or the Mac input. The same block RMS/peak values gate practice time, so the recording's leading silence earns nothing and the −20 dBFS tones do. This is Mac audio: the SGTL5000 line-in gain, the attenuator, rig noise and real latency remain bench measurements. Define `VEX_PITCH_PJRC` in `config.h` to build the PJRC `AudioAnalyzeNoteFrequency` path for a bench comparison.

| Tested on desktop | Still needs hardware |
|---|---|
| Practice timing, thresholds, goal changes, one-time rewards | SGTL5000 capture, input levels and rig noise |
| Shared screen layout, sprites, ring, banners, summary and state transitions | GC9A01A SPI timing, orientation, glare and standing readability |
| Pitch-to-note/cents math; YIN detector on synthetic tones, harmonics, noise and the reference WAV | Real guitar pickups, octave behaviour on real strings, latency and CPU cost on the target |
| CRC records, reboot, corrupt-record fallback and power cuts at several bytes of the ordered write | EEPROM power-loss behavior and endurance |
| Debounce/hold logic, calibration median and all hold actions through the shared App | Switch bounce, GPIO wiring and actual stage-2 readings |

The browser keeps its two slots in `localStorage` (**Factory reset** clears them); the native simulator's save file is `build/sim-progress.bin`. `make clean` removes the entire disposable `build/` directory. Nothing is uploaded by these commands.

### Scenario scripts and golden frames

`tests/scenarios/*.txt` drive `build/vex-sim` (the native oracle) through its line protocol and are run by `make test`. `make test` also runs `tools/web.py check`, which compiles the WebAssembly module and compares two frames with native goldens under Node; it prints `SKIP` where `em++` or `node` is absent (CI). The protocol:

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

Two routes program the board; both need the mentor's unpowered wiring check first.

**From this repository** (same pinned core and libraries as `make firmware`):

```sh
make ports                      # lists Teensy serial ports (PJRC USB VID 16c0)
make firmware STAGE=1
make upload STAGE=1             # add SERIAL=/dev/cu.usbmodemXXXX when more than one board is attached
make monitor                    # read the serial output; Ctrl-C stops
```

`make upload` refuses to guess: it needs a compiled `build/stage-N/output/` image and either `SERIAL=` or exactly one Teensy on USB. It writes `build/upload_report.json`. `make test`, `make firmware` and `make sim` never touch the port.

**From Arduino IDE 2**: install Teensy support using [PJRC's instructions](https://www.pjrc.com/teensy/td_download.html) and the same three display-library versions through Library Manager. Open `firmware/VEX_Pocket_Rival/VEX_Pocket_Rival.ino`, choose **Teensy 4.1**, USB type **Serial**, 600 MHz, and select the board's USB port. Use **Verify** first, then **Upload**.

Either way, start with `VEX_STAGE 1` in `config.h`. Follow stages in order, editing the number and repeating compile/upload after each passing checkpoint. The physical Program button can recover entry to the Teensy loader. Use a data-capable Micro-B cable, not a charge-only cable.

The USB 5 V path powers the Teensy; no 9 V connection exists. Begin audio tests with the trimmer at minimum signal. Confirm terminal identities with a meter. Use the circuit and pin table in the [build guide](BUILD_GUIDE.md), not a generic Teensy tutorial with different hardware.

The [left-only test WAV](../test_audio/README.md) can exercise reference notes through a known line source. Start with the source volume down; digital WAV amplitude does not establish analog volts. Confirm actual amplitude before raising the trimmer. This is hardware-in-the-loop testing, not an emulator.

## 4. Bench loop with a Teensy attached

The firmware is instrumented so the questions the desktop cannot answer (how long a flush takes, whether bands tear, how the motion reads on the real panel) are measured instead of inferred, and the browser workbench can sit beside the real device while you develop.

### What the sketch reports

From stage 2 up, the sketch prints **one JSON line per second**, for example:

```json
{"stage":4,"codec":1,"rms":0.05012,"peak":0.201,"gate":0.00400,"active":1,"running":1,"session_s":42,"total_s":642,"fragments":2,"level":2,"cpu":11.3,"memory":12,"tx_bytes":28800,"tx_max_bytes":115200,"flush_us":4120,"flush_max_us":17890,"loop_max_us":18240,"cadence_ms":100,"diff":1,"full":0,"link":0,"anim":2,"frame":3,"pitch":"yin","hz":110.02,"prob":0.93,"yin_us":1480,"yin_max_us":2210}
```

| Field | Meaning |
|---|---|
| `flush_us`, `flush_max_us` | `micros()` around draw + band transfer for the last cadence, and the maximum since boot. Must stay well under `cadence_ms × 1000`; if `flush_max_us` approaches it, motion will stutter. |
| `loop_max_us` | Longest pass through `loop()` in the last second (includes YIN and the flush). |
| `tx_bytes`, `tx_max_bytes` | Bytes sent to the panel this cadence (dirty bands) and the largest so far. 115200 is a full frame. |
| `cadence_ms`, `diff`, `full`, `link` | Current switch settings (below). |
| `anim`, `frame` | The animation and step the renderer chose, in `art/vex/manifest.json` order — the same numbers the native simulator and scenarios use. |
| `yin_us`, `yin_max_us`, `cpu`, `memory` | Pitch-analysis time and PJRC audio-library CPU/memory maxima (stage 3+). |

`?` prints the switch settings as a one-line JSON object. Numbers in this document are examples of shape, not measurements; record real values in `docs/HANDOFF.md`.

### Serial switches

Single characters, stages 2–4 (stage 4 also keeps the older `c s p r`):

| Key | Effect | Use it to |
|---|---|---|
| `f` | Toggle **full-frame flush** every cadence | Compare `flush_us` for a whole frame against dirty bands; see whether band seams are visible |
| `b` | Toggle the **dirty-band diff** (default on) | Same as above from the other side |
| `1` / `2` | Redraw every 1× / 2× `DISPLAY_MS` | Judge whether 100 ms motion is needed or 200 ms reads the same on the panel |
| `l` | Toggle the **bench link** (binary band + state packets between the JSON lines) | Mirror the panel in the Device tab; off by default so plain serial stays readable |
| `m` `q` `k` | Tap MODE / QUEST / FOOT for 100 ms | Drive the device from the workbench without reaching for the switches |
| `M` `Q` `K` | Hold MODE / QUEST / FOOT for 1.3 s | Save, calibrate, new session |
| `?` | Print switch settings | Confirm what the device is doing |

Taps go through `App::controls` exactly like the GPIO lines, so debounce and hold timing are the device's.

### The Device tab

```sh
python3 -m pip install -r requirements-bench.txt     # pyserial, bench only
make sim DEVICE=auto                                 # or DEVICE=/dev/cu.usbmodemXXXX
```

`tools/web.py serve --device` opens the port in a thread, splits the stream with the same decoder as `make monitor` (`tools/link.py`, stdlib), assembles the 240 × 240 RGB565 frame from band packets, keeps the last state packet and sixty telemetry rows, and exposes them to the page as `/device` (long-poll), `/device/frame` (raw pixels) and `/device/send?c=` (one character). Unplugging shows "waiting" and reconnects when the port returns.

In the browser, open **Device**:

- Left canvas: the real panel, built from the rows the Teensy actually sent. Right canvas: this page's WebAssembly frame. Between them a count of differing pixels — zero means the device and the simulator are drawing the same thing for the same state.
- Buttons send the switches and taps above; **also tap this page** mirrors taps into the browser device so the two stay in step. **Adopt device record** writes the Teensy's progress (fragments, active and session time, goal) into the page's EEPROM slot 0 and reboots it, so both start from the same record.
- Sparklines: `tx_bytes`, `flush_us`, `yin_us` and audio `cpu` over the last minute, with the maximum printed in the corner. Below them the raw telemetry lines.

A pseudo-terminal fed by a script that writes the same packet format exercised the whole path end to end (link, mirror, diff, taps, adopt, sparklines) in headless Chrome; the protocol itself is round-tripped by `tests/core_tests.cpp` and `tests/link_test.py` in `make test`. What has **not** been done without a board: the upload, the real serial throughput (band packets at 115200 bytes per full frame share the USB CDC link with the JSON lines; USB serial on Teensy 4.1 is high-speed, so a full frame should take a few milliseconds, but that is a prediction), and the measurements themselves.

### First bench session checklist

1. `make firmware STAGE=2`, `make upload STAGE=2`, `make monitor`: confirm the screen picture and that each `M Q F` reading follows its own switch. Note `flush_us` and `flush_max_us` for the stage 2 screen.
2. `make firmware STAGE=4`, `make upload STAGE=4`, `make sim DEVICE=auto`, open **Device**, press **Link**. The left canvas should fill with the boot screen within a second and match the right canvas after **Adopt device record**.
3. Calibrate (`Q` hold), start a session (`k`), play: watch `flush_us` while the six-step strum runs. Record `flush_max_us`, `loop_max_us`, `tx_max_bytes` and `cpu` in the handoff. Press `f` and compare; press `2` and judge whether 200 ms reads as smoothly.
4. Look for tearing at band seams during the strum and the breath (band edges fall on sprite row boundaries). If seams show, the fix is a DMA panel driver behind the `Panel` adapter in the sketch, not a renderer change.
5. Leave the link on for a minute: the pixel-diff count should stay at zero whenever both devices are in the same state; any persistent difference is a renderer or timing bug worth a scenario.

### Measured-flush gate for a DMA driver

The Adafruit GC9A01A library writes pixels blocking on SPI. The sketch isolates that behind `struct Panel { begin(); writeBand(y, h, rows); end(); }` so a DMA-capable driver (`ILI9341_T4`-style) can replace it without touching the renderer, the band diff or the link. Switch only on evidence from step 3: if `flush_max_us` stays under about a third of the cadence with the dirty-band diff on and no seams are visible, the blocking path is adequate for R1; if it does not, record the numbers and open the DMA work in `docs/PLAN.md`.

## Editing and regeneration

- Rules/persistence: edit `core.h`, add a relevant regression, `make test` and `make firmware-all`.
- Device behaviour (controls, calibration, notices, saves): edit shared `app.h`, add or extend a scenario in `tests/scenarios/`, `make test`, then compile the target.
- Audio analysis: edit shared `analyzer.h`, extend the synthetic cases in `tests/core_tests.cpp` or `tests/scenarios/real_audio.txt`, then compile all stages and read `yin_us`/`cpu` in the stage 3/4 serial telemetry on the bench.
- Layout: edit shared `ui.h`, watch it live with `make sim` (saves rebuild and reload the page) or render every state with `python3 tools/render_gallery.py`, update goldens, then compile the target.
- Browser adapter only (`simulator/web/`): `make sim` while editing; `make test` confirms the module still matches native goldens. No target compile is needed when the shared headers are untouched.
- Character, colours, fonts, motion timing: edit the PNG/JSON sources in `art/` (any pixel editor), run `python3 tools/make_sprites.py` (standard library only; it validates, regenerates the headers and prints what each animation step costs the panel), then `make sim` to watch it or `make sim-build` + `python3 tools/render_gallery.py`, and update goldens.
- Bench instrumentation (`link.h`, the sketch's switches and telemetry, `tools/link.py`, the Device tab): keep `LinkState` and `tools/link.py` `STATE` in step (the `static_assert` and `assert` on its size catch drift), extend `linkTests()` and `tests/link_test.py`, `make test`, then `make firmware-all`. Details and drawing guidance in [ART.md](ART.md). `python3 tools/render_screen.py` creates the CAD texture from the actual shared renderer; a changed texture requires `make hero` so provenance stays valid.
- Plain Markdown or agent-guidance edits: edit the relevant files and run `make test`; no CAD or Arduino dependencies are needed.
- CAD geometry or generated hardware guides: create a Python virtualenv, install `requirements-cad.txt`, then regenerate only affected outputs with `make cad cad-check`, `make render` (Blender) and `make docs`.
- Presentation only: `make hero` renders the existing STEP using Blender and the CAD/Pillow environment; [details](MARKETING.md). It does not compile firmware or rebuild geometry.
- Mentor handoff: `make pack-mentor` creates the lean review ZIP using standard-library Python and existing files; [contents](MENTOR_PACK.md). It performs no builds.

Record target build results and actual bench measurements separately. A green host test does not prove firmware compilation, and successful compilation does not prove the electrical build.
