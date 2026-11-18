# Building VEX without a mentor

The illustrated guide was first written for a hobbyist working beside an electronics mentor, so it says "mentor" wherever a second pair of eyes helps. Nothing in the build needs a mentor to be possible: it is USB 5 V, 3.3 V logic, through-hole parts and a line-level input. This page says which of those moments you can cover alone, which you should hand to someone else, and which you must not skip.

Where any document says **mentor**, read: an experienced second person, such as a mentor, a maker-space volunteer, a friend who solders, or a studio engineer. Where a CSV has a `checked_by` column, write your own initials when you did the check yourself.

## Do these alone

| Step | Guide page | Solo note |
|---|---|---|
| Run the software first | "Try the device before hardware arrives"; `docs/LOCAL_DEVELOPMENT.md` | `make test`, `make sim`; you can learn every control and screen before any part arrives |
| Order the bench electronics | "Buy: the bench electronics"; `docs/SHOPPING.md` | Exact identities matter (Audio Adapter Rev D/D2, not C; Adafruit 6178, not a square ST7789). No substitutions on the first build |
| Stage 1 and 2 | "Upload 1", "Upload 2"; `docs/BRINGUP.md` session 1 | USB, LED, screen, buttons. The worst outcome of a wiring mistake here is a dead display or Teensy. Use `make bench STAGE=1`, then the `t` test pattern and `-` SPI step before suspecting wiring |
| Continuity checks | "Wire the controls" | A meter in continuity mode is the second pair of eyes; the guide says exactly what should and should not beep |
| Calibration, practice, tuner, save | "Upload 4"; `docs/BRINGUP.md` session 3 | Software only once stage 3 passes; the acceptance rows are observations you can make yourself |
| Measure the parts | `hardware/data/measurements.csv`; `make measure-check` | Calipers and the printed `Fit_Coupon.stl`. The checker tells you which parts are safe to print now and which wait |
| Print the enclosure | `docs/PRINTING.md` | Your printer or a print service; follow the measurement gate, not the render |

## Get a second person or a service for these

| Step | Why | Who |
|---|---|---|
| The attenuator before it meets your rig | It is three parts, but it is the one circuit between your guitar signal chain and the codec. A second look at the divider orientation and the LINE IN ground costs five minutes | Anyone who has soldered; a maker space; a photo posted to a forum works |
| Audio-stack header soldering | Short aligned headers carry the I2S clocks; a bridge is hard to see | Magnification plus a second look, or a maker space |
| Cutting the 2 mm aluminium foot plate and the 1 mm polycarbonate window | Metal and sheet cutting are tool and safety questions, not design questions | Send `hardware/generated/drawings/*.dxf` to a laser- or water-jet service, or a maker space with the right tools |
| Drilling the input perfboard | Small; a drill press makes it square | A maker space, or go slowly with a hand drill and the 1:1 template PDF |

## Do not skip these, alone or not

- **Unpowered checks before every new wiring group.** USB out; no steady low-ohm short between 3.3 V and GND; the guide's checkpoints say what to measure.
- **Lowest gain first.** Codec `INPUT_LEVEL` 0 and the trimmer at minimum; raise with `g` while watching peaks. 0.98 is clipping.
- **Hand operation only until the load test.** The printed enclosure has no force rating. Test the foot plate on its four columns with the electronics removed before standing on it. If you have nobody to define the test with, treat the first version as a desk device and keep it that way.
- **Record what you saw, not what you expected.** `hardware/data/acceptance_log.csv` and `measurements.csv` stay blank until you measure. `make bench-report` summarises a session's serial log for you.
- **Keep your existing tuner and dry path.** The companion has no output and no bypass; if its dry feed disappears in a preset, that is a rig fact to log, not a firmware bug to fix.

## When you are stuck

Use the troubleshooting page ("Troubleshoot by the failed checkpoint") first; it is organised by the earliest failing stage. The bench loop in `docs/LOCAL_DEVELOPMENT.md` §4 shows the real panel beside the simulator in the browser so a display problem is visible rather than inferred. Open a GitHub issue with the `make bench-report` output and a photo; the serial telemetry says more than a description.
