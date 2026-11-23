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

`make sim` compiles the shared firmware headers (`core.h`, `app.h`, `analyzer.h`, `notes.h`, `drills.h`, `drill_*.h`, `ui.h`) plus the unified simulator interpreter and state schema (`simulator/protocol.h`, `simulator/state.h`) to WebAssembly through `simulator/web/vex_web.cpp`, copies all 14 `tests/scenarios/*.txt` and 45 `tests/golden/*.png` into `build/web/` with a manifest, serves `build/web/` on `127.0.0.1:8765`, rebuilds when any tracked source changes and reloads the page. Nothing round-trips through a server while you use it: the page owns a millisecond clock and steps the real `vex::App` in the sketch's loop order (observe, controls, tick), so debounce, 1.2 s holds, the calibration median, the 40 ms smoothed level, the 500 ms activity hold, onset/note tracking, the five guitar drills, the idle auto-save and the ordered two-slot `Record` + `Skills` saves behave exactly as on the Teensy and in `make test`.

- **Controls & display**: press and hold **MODE**, **QUEST** and **FOOTSWITCH** with the pointer or keys **M**, **Q**, **Space**; the held time is shown and each button carries a dynamic 1-action caption for the current state (`MODE` cycles `Pet → Tuner → Drill`, hold saves; `QUEST` steps the paused quest goal or drill preset, hold calibrates; `FOOTSWITCH` starts/pauses, hold starts a new session). The shared 240 × 240 renderer blits every `DISPLAY_MS` (100 ms) like the pedal (toggle for every frame), with a true-size 32.4 mm option and a `test pattern` toggle (`t`). Stage 2 and 3 show a banner explaining that controls are inert there by design.
- **Clock & lifecycle**: 1×, 10× or 100× real time, Pause (**P**), Step 1 s, Advance 5 min, Reboot and Factory reset (zeroes all four EEPROM slots across `Record` v1 and `Skills` v1). A header pill shows the Teensy bridge status (`bridge off`, `waiting`, `live`) and opens **Inspect → Device**.
- **Guitar input, 30 s activity strip & WAV clip recorder**: boots on **Not playing** (quiet cable, `0.0010` RMS) so holding **QUEST** calibrates immediately without hitting the "Too loud" guard. Player-language input choices select **Not playing**, **Playing**, **Strumming** (harmonic plucks with 280 ms chord-change gaps fed through the real `Analyzer`), **Too hot** (clipping), **Cable unplugged** (stale samples), plus **Synth note (tuner)**, **24 s reference WAV**, **Mac / interface input** (AudioWorklet at 44.1 kHz) and **Custom level**. Below the level meter, a scrolling 30 s **Activity strip** plots raw block RMS, the 40 ms smoothed level, the dashed gate line, green credited spans, gold 500 ms hold tails, crimson clip ticks and a 20 px **Note lane** (onset ticks + settled MIDI note pills), while reporting the active `App::pitchMode()` (`PITCH_OFF`, `PITCH_TUNER`, `PITCH_FAST` or `PITCH_RUN`). **Record clip** captures dry 16-bit 44.1 kHz mono WAV takes straight from the active guitar input for `test_audio/drills/` (`make eval`).
- **Step-by-step guide (`session.js`)**: progressive one-step-at-a-time onboarding across **First Run** (6 steps: calibrate, start, strum across chord gaps, finish quest, check tuner, save & replug) and five 4-step guitar drill guides (**Timing**, **Note Finder**, **Bend & Vibrato**, **Scale Run**, **Dynamics**; see [DRILLS.md](DRILLS.md)). Completed and upcoming steps collapse to single-line rows while the active step highlights the exact pedal button (**MODE**, **QUEST** or **FOOTSWITCH**) with a gold ring and live tap countdown (`TAP ×2`, `HOLD 1.2s`, `PAUSE`), plus **Do it for me** when you want the page to drive the inputs. Step 4 of each drill opens an inline playground with plain-English mode pills, one-click clean and mistake takes (`playDrillPerformance`), and a 4-stat scoreboard.
- **Screens gallery (`gallery.js` + `scenario.js`)**: collapsible `<details>` drawer listing all 45 committed golden screens across 14 scenarios (Core pet & quest, Tuner & hardware stages, Guitar drills). Clicking any thumbnail replays its `tests/scenarios/*.txt` script up to that exact line on the live WebAssembly device, pauses there so you can inspect or press **Run** to continue from that state, and reports the pixel diff against `tests/golden/<name>.png`.
- **Inspect internals (`<details>`)**: collapsible drawer (remembers open/closed state in `localStorage`) holding **State**, **EEPROM** (256 bytes: two 64-byte `Record` v1 slots at `0/64` + two 64-byte `Skills` v1 slots at `128/192`, decoded + hex-dumped, corrupt-latest and power-cut-after-N-writes), **Log** (per-second JSON telemetry + notice history), **Scenario** (recorder + repository scenario picker + in-browser golden check via `scenario.js`) and **Device** (the Teensy bench mirror described in [section 4](#4-bench-loop-with-a-teensy-attached)).

Walkthrough:

1. Open http://127.0.0.1:8765 on **First Run**. Hold the highlighted **QUEST** button (or press **Q** for 1.2 s) and wait 3 s (or switch speed to **10×**): calibration finishes at `gate = 0.0040` and Step 1 collapses with a check mark. (Select **Playing** and hold **QUEST** if you want to see calibration refuse a loud floor.)
2. Tap **FOOTSWITCH** (**Space**) for Step 2, then click **Strumming** for Step 3: watch VEX strum while the 30 s **Activity strip** shows the 40 ms smoothed curve and gold **500 ms hold** spans bridging the 280 ms chord gaps continuously, with `pitch off (pet · 0 windows)`.
3. Click **Advance 5 min** (or **Do it for me** on Step 4): expect 50 participation XP, a full quest ring and one fragment with a three-second banner. Ten active minutes shows the level-up banner.
4. Tap **MODE** (**M**) for Step 5 (**Tuner**) and click **Synth note (tuner)**: practice credit pauses while `PITCH_TUNER` runs (`midi 69`, `A4`), detune shows flat/sharp/in-tune, **Not playing** shows the 1.5 s stale fade, and lowering the level enters the uncertain band. Tap **MODE** back to **Pet**, hold **MODE** to save to EEPROM, and click **Reboot** to complete Step 6 (`6 / 6 done`, `1 fragment` restored while booting paused and uncalibrated).
5. Pick any of the five drill tabs (**Timing**, **Note Finder**, **Bend & Vibrato**, **Scale Run**, **Dynamics**): follow the gold button spotlight onto the drill screen (`MODE` → `QUEST` → `FOOTSWITCH`), then use Step 4's clean and mistake take buttons (or play through **Mac / interface input**) to watch the live readout and saved personal best update. Open **Screens gallery (45 reference states)** to jump to any golden (`0 / 57,600 px differ`), or **Inspect internals** to test torn EEPROM writes (`cut power after 10 byte writes`).

### Real audio on the desktop

`firmware/VEX_Pocket_Rival/analyzer.h` (level meter, O(1) ISR block-energy onset ring and per-activity YIN pitch profiles) and `notes.h` (`NoteTracker` note/bend/vibrato tracker) are the exact headers the Teensy compiles. Both simulators feed them 128-sample blocks at 44.1 kHz: the native one from `test_audio/guitar_notes_left_only.wav` (the `wav` command), raw 16-bit mono PCM (`raw`), or streamed `tone`/`notes`/`bend`/`vib` phrases (`simulator/protocol.h`); the browser from the same inputs or the Mac audio interface. `tools/eval_audio.py` (`make eval`, included in `make test`) scores onset, MIDI pitch, bend cents and level accuracy across `test_audio/drills/`. This is Mac audio: the SGTL5000 line-in gain, the attenuator, rig noise and real latency remain bench measurements. Define `VEX_PITCH_PJRC` in `config.h` to build the PJRC `AudioAnalyzeNoteFrequency` path for a bench comparison.

| Tested on desktop | Still needs hardware |
|---|---|
| Practice timing, thresholds, goal changes, one-time rewards | SGTL5000 capture, input levels and rig noise |
| Shared screen layout, sprites, ring, banners, summary and state transitions | GC9A01A SPI timing, orientation, glare and standing readability |
| Pitch-to-note/cents math; YIN detector on synthetic tones, harmonics, noise and the reference WAV | Real guitar pickups, octave behaviour on real strings, latency and CPU cost on the target |
| CRC records, reboot, corrupt-record fallback and power cuts at several bytes of the ordered write | EEPROM power-loss behavior and endurance |
| Debounce/hold logic, calibration median and all hold actions through the shared App | Switch bounce, GPIO wiring and actual stage-2 readings |

The browser keeps its 256-byte EEPROM (`Record` v1 + `Skills` v1) in `localStorage` (**Factory reset** clears all four slots); the native simulator's save file is `build/sim-progress.bin`. `make clean` removes the entire disposable `build/` directory. Nothing is uploaded by these commands.

### Scenario scripts and golden frames

`tests/scenarios/*.txt` drive `build/vex-sim` (the native oracle) through `simulator/protocol.h` and are run by `make test` alongside `make eval` (`tools/eval_audio.py`). `make test` also runs `tools/web.py check` (`make web-check`), which compiles the WebAssembly module and replays all 14 scenarios through `simulator/web/scenario.js` (forwarding non-`wav` lines straight to `vex_command`) under Node, comparing every one of the 45 golden frames pixel for pixel against `tests/golden/*.png`; it prints `SKIP` where `em++` or `node` is absent (CI). The protocol:

| Command | Effect |
|---|---|
| `press <mode or quest or foot> <ms>` | Hold a control for `ms`, then release and settle 30 ms |
| `audio <ms> <rms> <peak> [hz prob]` | Inject fresh level (and optional pitch) observations for `ms` |
| `wait <ms>` | Advance time with no fresh samples (they go stale) |
| `wav <path> [start_ms length_ms]`, `raw <path>` | Feed PCM through the shared analyzer |
| `tone <ms> <hz> <dbfs> [decay_ms] [gap_ms]` | Synthesise a harmonic tone or gapped plucks at 44.1 kHz through the shared analyzer |
| `notes <bpm[x2\|x4]> <jitter_ms> <drift_ms> <dbfs> <note[@db]\|~note\|_ ...>` | Stream picked notes, `~note` hammer-on steps or `_` rests with deterministic timing jitter/drift |
| `bend <ms> <note\|hz> <target_cents> [hold_ms] [dbfs]` | Stream a plucked fretted note bending smoothly to `+target_cents` and holding the apex |
| `vib <ms> <note\|hz> <rate_hz> <width_cents> [dbfs]` | Stream a held note with sinusoidal vibrato (`width_cents` peak-to-peak) |
| `drill <preset-slug>`, `pitch <off\|tuner\|fast\|run>`, `events [clear]`, `seed <u>` | Jump to a paused `DRILL_PRESETS[]` slug, override `PitchMode`, print/clear `NoteEvent`s, or set the RNG seed |
| `serial <c, s, p or r>`, `test <on\|off>` | The sketch's USB debug commands and the display test pattern |
| `calibrate [rms]` | Serial `c` plus 3.1 s of a quiet floor (default 0.001) |
| `stage <2, 3 or 4>`, `reboot`, `reset`, `corrupt [skills]`, `powercut <bytes>`, `save`, `state` | Device lifecycle; `corrupt skills` flips a `Skills` CRC bit; `powercut` kills storage after N byte writes |
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
| 4 | Pet / practice / tuner / guitar drills / saves | Calibrated gating, quest, reference-note tuner, five drills and save/reboot checks |

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
{"stage":4,"up_ms":42310,"codec":1,"activity":"pulse-8th","onsets":32,"bpm":96,"rms":0.05012,"peak":0.201,"gate":0.00400,"active":1,"running":1,"session_s":42,"total_s":642,"fragments":2,"level":2,"credit_drops":0,"cpu":11.3,"memory":12,"tx_bytes":28800,"tx_max_bytes":115200,"draw_us":720,"draw_max_us":910,"flush_us":3400,"flush_max_us":17100,"loop_max_us":18240,"cadence_ms":100,"diff":1,"full":0,"link":0,"test":0,"spi_hz":16000000,"in_level":0,"anim":2,"frame":3,"pitch":"yin","hz":0.00,"prob":0.00,"yin_us":0,"yin_max_us":2210}
```

| Field | Meaning |
|---|---|
| `activity`, `onsets`, `bpm` | Current foreground screen or drill preset slug (`pet`, `tuner`, `pulse-8th`, `hunt-str`, …), cumulative note events detected since boot, and active drill BPM (`0` when not in a tempo drill). |
| `draw_us`, `draw_max_us` | `micros()` around rendering into the RAM canvas plus the dirty-band row diff (`vex::bands`), before SPI begins. |
| `flush_us`, `flush_max_us` | `micros()` around the SPI band transfer (`Panel::begin/writeBand/end` + shadow copy) for the last cadence (`0` if no rows changed), and the maximum since boot. Must stay well under `cadence_ms × 1000`; the DMA gate compares `flush_max_us` directly against SPI budget. |
| `credit_drops` | Practice ticks rejected since boot because a loop pass exceeded 100 ms (`dt > 100` while running with fresh unclipped audio). Should stay `0`. |
| `loop_max_us` | Longest pass through `loop()` in the last second (includes YIN when active, draw and SPI flush). |
| `tx_bytes`, `tx_max_bytes` | Bytes sent to the panel this cadence (dirty bands) and the largest so far. 115200 is a full frame. |
| `up_ms` | Milliseconds since boot; `tools/bench_report.py` counts reboots when it goes backwards and reports run time for acceptance row 18. |
| `cadence_ms`, `diff`, `full`, `link`, `test` | Current switch settings (below). |
| `spi_hz`, `in_level` | The two live knobs: panel SPI clock and codec line-in level. Boot values are `TFT_HZ` and `INPUT_LEVEL` in `config.h`; serial steps are not remembered across a reset. |
| `anim`, `frame` | The animation and step the renderer chose, in `art/vex/manifest.json` order — the same numbers the native simulator and scenarios use. |
| `yin_us`, `yin_max_us`, `cpu`, `memory` | Pitch-analysis time (`App::pitchMode()` is `PITCH_OFF` — `yin_us = 0` — on stage-4 Pet, `PULSE` and `TOUCH`; `PITCH_TUNER` 2048/2048 on stage 3, Tuner and `HUNT`; `PITCH_FAST` 1024/512 on `BEND`; `PITCH_RUN` 2048/1024 on `RUN`, stepping back to 2048 hop if `yin_us > 14,000`) and PJRC audio-library CPU/memory maxima (stage 3+). |

`?` prints the switch settings as a one-line JSON object. Numbers in this document are examples of shape, not measurements; record real values in `docs/HANDOFF.md`.

### Serial switches

Single characters, stages 2–4 (stage 4 also keeps the older `c s p r`):

| Key | Effect | Use it to |
|---|---|---|
| `f` | Toggle **full-frame flush** every cadence | Compare `flush_us` for a whole frame against dirty bands; see whether band seams are visible |
| `b` | Toggle the **dirty-band diff** (default on) | Same as above from the other side |
| `1` / `2` | Redraw every 1× / 2× `DISPLAY_MS` | Judge whether 100 ms motion is needed or 200 ms reads the same on the panel |
| `l` | Toggle the **bench link** (binary band + state packets between the JSON lines) | Mirror the panel in the Device tab; off by default so plain serial stays readable |
| `m` `q` `k` | Tap MODE / QUEST / FOOT for 100 ms | Drive the device from the workbench (`m` cycles `Pet → Tuner → Drill`; `q` steps paused goal or drill preset; `k` starts/pauses) |
| `M` `Q` `K` | Hold MODE / QUEST / FOOT for 1.3 s | Save `Record` + `Skills`, calibrate noise floor, start a new session |
| `-` / `+` | Panel **SPI clock** one step down / up: 8, 16, 24, 30, 40, 60 MHz (boots at `TFT_HZ`, 16) | Find the fastest clock the wiring carries cleanly before considering a DMA driver: a full frame at 16 MHz is about 58 ms of the 100 ms cadence; at 30 MHz about 31 ms. Noise or a frozen panel means step back down. A full frame is re-sent after each step so a bad clock shows at once |
| `g` / `G` | Codec **line-in level** down / up, 0–15 (boots at `INPUT_LEVEL`, 0; stage 3–4 with a codec) | Sweep sensitivity during the trimmer check (acceptance row 06) without recompiling; the attenuator stays the documented path, this only moves the codec's full-scale |
| `t` | Toggle the **display test pattern** (full flush while on) | Border, four differently coloured corner marks and a "TOP" label catch a mirrored or rotated panel; seven theme-colour bars show colour order and depth; a tick at every 16th row on both edges shows a dropped or shifted band; a frame counter and a marker that walks one cell per cadence show whether the flush keeps up |
| `?` | Print switch settings and knob values | Confirm what the device is doing |

Taps go through `App::controls` exactly like the GPIO lines, so debounce and hold timing are the device's.

### The Device tab

```sh
python3 -m pip install -r requirements-bench.txt     # pyserial, bench only
make sim DEVICE=auto                                 # or DEVICE=/dev/cu.usbmodemXXXX
```

`tools/web.py serve --device` opens the port in a thread, splits the stream with the same decoder as `make monitor` (`tools/link.py`, stdlib), assembles the 240 × 240 RGB565 frame from band packets, keeps the last state packet and sixty telemetry rows, and exposes them to the page as `/device` (long-poll), `/device/frame` (raw pixels) and `/device/send?c=` (one character). Unplugging shows "waiting" and reconnects when the port returns.

In the browser, open **Device**:

- Left canvas: the real panel, built from the rows the Teensy actually sent. Right canvas: this page's WebAssembly frame. Between them a count of differing pixels — zero means the device and the simulator are drawing the same thing for the same state.
- Buttons send the switches and taps above; **also tap this page** mirrors taps into the browser device so the two stay in step. **Adopt device record** writes the Teensy's `Record` v1 progress (fragments, active and session time, goal) from `LinkState` into the page's EEPROM slot 0 and reboots it, so both start from the same practice record.
- The **bench** row sends the knobs (`SPI −/+`, `level −/+`, `test pattern`) and shows the current `spi_hz`, `in_level`, `test`, cadence and diff state from the latest telemetry line.
- Sparklines: `tx_bytes`, `flush_us`, `yin_us` and audio `cpu` over the last minute, with the maximum printed in the corner. Below them the raw telemetry lines.

### Bench logs and the report

Both `make monitor` and the bridge append everything the device says to `build/bench/<start>.jsonl` (`build/bench/latest.jsonl` points at the current one; the status line in the Device tab names it). Each line is `{"at": seconds, "telemetry": {...}}`, `{"at", "state": {...}}` or `{"at", "text": "..."}`. Nothing under `build/` is committed.

```sh
make bench-report                       # summarise build/bench/latest.jsonl
make bench-report LOG=build/bench/20261118-091500.jsonl
```

The report prints run time and reboots, the knob and switch states seen, `flush_us` median / p95 / max against the cadence budget, `tx_max_bytes`, `loop_max_us`, audio CPU and memory, `yin_us`, the level range and seconds at or above the 0.98 clip threshold, gate, running and active seconds, and the fragment and total deltas. It ends with the `acceptance_log.csv` rows those numbers inform (03, 04, 05, 06, 07, 08, 09, 18) and the DMA-gate verdict below. It only reads; the measured fields in the CSV are yours to fill after looking at the panel and the meter.

`make bench STAGE=n [SERIAL=...]` chains `make firmware`, `make upload` and `make sim DEVICE=auto` and stops at the first failure. It is still the only path besides `make upload`/`make monitor` that touches a port.

A pseudo-terminal fed by a script that writes the same packet format exercised the whole path end to end (link, mirror, diff, taps, adopt, sparklines) in headless Chrome; the protocol itself is round-tripped by `tests/core_tests.cpp` and `tests/link_test.py` in `make test`. What has **not** been done without a board: the upload, the real serial throughput (band packets at 115200 bytes per full frame share the USB CDC link with the JSON lines; USB serial on Teensy 4.1 is high-speed, so a full frame should take a few milliseconds, but that is a prediction), and the measurements themselves.

### First bench session checklist

1. `make firmware STAGE=2`, `make upload STAGE=2`, `make monitor`: send `t` (`make monitor SEND=t`, or the Device tab's **test pattern** button) and check the test pattern's border, corner colours and "TOP" before anything else: a white or rotated panel is a wiring or `setRotation` question, not a renderer one. Toggle `t` off, confirm the stage 2 picture and that each `M Q F` reading follows its own switch. Step `+` until the pattern degrades, then back one step; note `spi_hz`, `flush_us` and `flush_max_us`.
2. `make firmware STAGE=4`, `make upload STAGE=4`, `make sim DEVICE=auto`, open **Device**, press **Link**. The left canvas should fill with the boot screen within a second and match the right canvas after **Adopt device record**.
3. Calibrate (`Q` hold), start a session (`k`), play: watch `flush_us` while the six-step strum runs. Press `f` and compare; press `2` and judge whether 200 ms reads as smoothly. During the trimmer sweep (row 06) use `g`/`G` only to understand headroom; the shipped value stays `INPUT_LEVEL` until a measurement argues otherwise. Afterwards run `make bench-report` and copy its numbers into the handoff table.
4. Look for tearing at band seams during the strum and the breath (band edges fall on sprite row boundaries). If seams show, the fix is a DMA panel driver behind the `Panel` adapter in the sketch, not a renderer change.
5. Leave the link on for a minute: the pixel-diff count should stay at zero whenever both devices are in the same state; any persistent difference is a renderer or timing bug worth a scenario.

### Measured-flush gate for a DMA driver

The Adafruit GC9A01A library writes pixels blocking on SPI. The sketch isolates that behind `struct Panel { begin(); writeBand(y, h, rows); end(); }` so a DMA-capable driver (`ILI9341_T4`-style) can replace it without touching the renderer, the band diff or the link. Switch only on evidence from step 3: if `flush_max_us` stays under about a third of the cadence with the dirty-band diff on and no seams are visible, the blocking path is adequate for R1; if it does not, record the numbers and open the DMA work in `docs/PLAN.md`.

## Editing and regeneration

- Rules/persistence: edit `core.h`, add a relevant regression, `make test` and `make firmware-all`.
- Device behaviour (controls, calibration, notices, saves): edit shared `app.h`, add or extend a scenario in `tests/scenarios/`, `make test`, then compile the target.
- Audio analysis & note events: edit shared `analyzer.h` or `notes.h`, extend `tests/core_tests.cpp`, `tests/scenarios/real_audio.txt` and `test_audio/drills/` (`make eval`), then compile all stages and read `yin_us`/`cpu`/`onsets` in the stage 3/4 serial telemetry on the bench.
- Guitar drills (`drills.h`, `drill_view.h`, `drill_*.h`, [DRILLS.md](DRILLS.md)): follow the 5-step recipe in `docs/DRILLS.md` (`Skills` v1 slot, header, `DRILL_PRESETS[]` row, `session.js` + `app.js` guide/takes, scenario + goldens), then run `make test` and `make firmware-all`.
- Layout: edit shared `ui.h`, watch it live with `make sim` (saves rebuild and reload the page) or render every state with `python3 tools/render_gallery.py`, update goldens, then compile the target.
- Browser adapter only (`simulator/web/`): `make sim` while editing; `make test` confirms the module still matches native goldens. No target compile is needed when the shared headers are untouched.
- Character, colours, fonts, motion timing: edit the PNG/JSON sources in `art/` (any pixel editor), run `python3 tools/make_sprites.py` (standard library only; it validates, regenerates the headers and prints what each animation step costs the panel), then `make sim` to watch it or `make sim-build` + `python3 tools/render_gallery.py`, and update goldens.
- Bench instrumentation (`link.h`, the sketch's switches, knobs and telemetry, `tools/link.py`, `tools/bench_report.py`, the Device tab): keep `LinkState` and `tools/link.py` `STATE` in step (the `static_assert` and `assert` on its size catch drift), extend `linkTests()`, `tests/link_test.py` and `tests/bench_report_test.py`, update the switch table above and the `.ino` header comment, `make test`, then `make firmware-all`. Details and drawing guidance in [ART.md](ART.md). `python3 tools/render_screen.py` creates the CAD texture from the actual shared renderer; a changed texture requires `make hero` so provenance stays valid.
- Plain Markdown or agent-guidance edits: edit the relevant files and run `make test`; no CAD or Arduino dependencies are needed.
- CAD geometry or generated hardware guides: run `make measure-check` first so the edit answers a measured deviation (it names the `build_cad.py` constant and the parts each row gates), create a Python virtualenv, install `requirements-cad.txt`, then regenerate only affected outputs with `make cad cad-check`, `make render` (Blender) and `make docs`. The order of operations with a board and parts arriving together is in [BRINGUP.md](BRINGUP.md).
- Presentation only: `make hero` renders the existing STEP using Blender and the CAD/Pillow environment; [details](MARKETING.md). It does not compile firmware or rebuild geometry.
- Handoff ZIPs: `make pack-mentor` (hobbyist or mentor review) and `make pack-studio` (adds the product brief, decisions, motion and screen references for a design team) use standard-library Python and existing files; [contents](MENTOR_PACK.md). Neither performs builds.

Record target build results and actual bench measurements separately. A green host test does not prove firmware compilation, and successful compilation does not prove the electrical build.
