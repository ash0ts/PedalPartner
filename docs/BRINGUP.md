# Bring-up runbook: electronics and enclosure in parallel

The Teensy, audio adapter, display and small parts arrive before the enclosure exists. Run two tracks at once and let each gate the other at two points. Everything below uses commands that already exist; the measured fields in `hardware/data/acceptance_log.csv` and `hardware/data/measurements.csv` are filled by hand after looking, never by a script.

```
Track A  electronics on the desk                      Track B  enclosure
--------------------------------------------------    ------------------------------------------------
A1  make bench STAGE=1   (USB, LED, "VEX USB OK")     B1  print Fit_Coupon.stl, W_gold_insert.stl,
A2  STAGE=2   screen + buttons + test pattern  ----->      W_crimson_cheek.stl now (make measure-check
A3  STAGE=3   codec, trimmer, g/G level             |      says these wait on nothing)
A4  STAGE=4   practice, tuner, save            ---+ |  B2  measure boards, jack, buttons, foot, headers
A5  make bench-report after every session         | +->    with the coupon and calipers
A6  acceptance_log rows 03-11, 18 by hand         |    B3  measurements.csv -> make measure-check
                                                  |    B4  deviation? edit build_cad.py, make cad cad-check
                                                  +--> B5  print base, lid, tray, spacers; cut plate, window
```

Gate 1 (A2 -> B2): the header seated gap and the screen at the real board angle are measured with the electronics running, before any enclosure part that depends on them is printed. Gate 2 (A4 -> B5): `flush_max_us` from a stage 4 session decides whether R1 keeps the blocking display driver; a DMA driver would change nothing mechanical, but a 200 ms cadence or a different panel angle might, so settle it before the lid is final.

## Before the parts arrive

- `make test` and `make firmware-all` on the current tree; keep `build/target_build_report.json`.
- `python3 -m pip install -r requirements-bench.txt` (pyserial) and `make sim` once so Emscripten is known good.
- Print B1 now (`make measure-check` lists them as *print now*); read [PRINTING.md](PRINTING.md) for the profile.
- Read [LOCAL_DEVELOPMENT.md](LOCAL_DEVELOPMENT.md) §4 for the telemetry fields and serial switches you will be reading.

## Session 1: USB, screen, controls (stages 1-2, about an hour)

1. Wire nothing but USB. `make bench STAGE=1` (compiles, uploads, opens the workbench; the Device tab shows the port and `VEX USB OK` lines). Acceptance row 03.
2. Power off, add display wires W01-W10 and controls W11-W14 from the [build guide](BUILD_GUIDE.md). `make bench STAGE=2`.
3. In the Device tab press **test pattern** (`t`). Expect: ink border, gold corner top-left, red top-right, plum bottom-left, "TOP" at the top, seven colour bars, a tick at every 16th row, a marker walking one cell per cadence. White screen: VIN/GND, CS/DC/RST, MOSI 11, SCK 13. Rotated or mirrored: `setRotation` in the sketch, not wiring. Missing or shifted ticks: a band is being dropped; press `f` and compare.
4. Press **SPI +** until the pattern degrades, then one step back. Note `spi_hz`, `flush_us`, `flush_max_us`. Press `t` again to return to the stage 2 screen and check `M Q F` follow their own switches. Row 04.
5. `make bench-report`. Copy the display line into [HANDOFF.md](HANDOFF.md) "Bench observables".
6. **Gate 1 measurements now**, while the stack is on the desk: header seated gap, display total thickness and mounting pitch, screen readability at the board angle. Write them into `measurements.csv`; `make measure-check` tells you which parts this frees or blocks.

## Session 2: audio (stage 3)

1. Build the attenuator on the perfboard (cut now), trimmer at minimum. `make bench STAGE=3` with a known low-level line source first, then the buffered dry feed.
2. Expect `codec: 1`, `rms`/`peak` moving. Raise the trimmer slowly; use **level −/+** (`g`/`G`) only to learn how much headroom the codec has. Rows 05, 06. `INPUT_LEVEL` in `config.h` changes only with a written measurement.
3. `make bench-report`: codec seconds, peak range, seconds at or above 0.98, `in_level` values seen.

## Session 3: the companion (stage 4)

1. `make bench STAGE=4`. **Link**, then **Adopt device record** so the page and the device share a record. Calibrate (`Q` hold), start (`k`), play.
2. Watch `flush_us` during the six-step strum; press `f` and `2` to compare; look for seams at band edges. Leave Link on for a minute: the pixel diff should stay at zero while both are in the same state.
3. Rows 07 (gate), 08 (activity), 09 (one fragment per five active minutes), 10 (save and replug), 11 (tuner against reference notes), 18 (30 minutes running; `make bench-report` prints uptime and reboots).
4. **Gate 2**: the report's DMA line. `flush_max_us` under a third of the cadence with the dirty-band diff on and no visible seams keeps the pinned Adafruit path; otherwise open the DMA task in [PLAN.md](PLAN.md) with the numbers attached. If 200 ms (`2`) reads as well as 100 ms on the glass, say so in the handoff; `DISPLAY_MS` changes only with that observation.

## Enclosure track

1. Coupon first. Try the M3 clearance, 7.2 mm button hole, 12.4 mm switch hole, 9.8 mm jack hole and the nut pocket with the real parts. Enter jack, button, foot and cap-bore measurements in `measurements.csv`.
2. `make measure-check`. Each row names the `build_cad.py` constant it stands for and the parts it gates; the table at the end says *print now*, *ready* or *wait* with the reason. An out-of-tolerance row with an empty `change_required` is flagged; write the decision in the CSV before editing CAD.
3. For a deviation: edit `hardware/cad/build_cad.py`, `make cad cad-check`, `make measure-check` again; update the CSV's assumption column only when the CAD constant changed (the checker compares them).
4. Print in order: spacers and tray (depend on the display and header rows), faceplate and bezel, base; cut the plate and window from the DXFs last. Never pull a lid shut with screws; reopen and measure.
5. Row 14 (fit), 15 (readability), 16 (mentor load test without electronics) close the enclosure track.

## What stays unprovable until the bench or the mentor

Upload against a board, SPI clocks above 16 MHz, codec level steps, the test pattern on glass, USB throughput with the link on, tearing at band seams, perceived motion at 32.4 mm, and every row of the acceptance log. The printed enclosure has no force rating until the mentor-defined load test. Write what was seen, not what was expected.
