# Final R1.1 shopping list

This list matches the current CAD and wiring. Buy the bench electronics first, or source one complete set for CAD fit verification. Product links point to live retail listings checked November 2026; stock and prices can change.

## Quick vendor consolidation

- **SparkFun** (official Teensy manufacturer/retailer): Teensy 4.1 unpinned (`DEV-16771`, or `DEV-20359` without Ethernet), Teensy 4 Audio Shield Rev D (`DEV-15845`), 2 × red panel buttons (`COM-11992`), 2.54 mm breakaway male + female headers (`PRT-00116` + `PRT-00115`, trimmed to two 14-pin rows), USB Micro-B data cable (`CAB-10215`).
- **Adafruit**: 1.28" 240×240 round GC9A01A TFT (`#6178`). Do **not** substitute a square ST7789 breakout; the optional EYESPI flex cable is unused because VEX wires to the 0.1" pads.
- **Amplified Parts**: soft SPST momentary footswitch (`P-H605-BLK`) and Switchcraft `111X` enclosed ¼" mono jack (`W-SC-111X`, includes nut and washer).
- **DigiKey / Mouser / McMaster / shop stock**: Bourns `3386P-1-103LF` 10 kΩ trimmer, two `10 kΩ` and two `1 kΩ` 1% 0.25 W through-hole resistors, 3 mm amber LED, 2.54 mm FR4 protoboard, metric M2/M3 screws, nuts and M2 nylon spacers (schedule below), 2 mm aluminum sheet (`Foot_Plate_2mm_Aluminum.dxf`), and 1 mm clear polycarbonate (`Window_1mm_Polycarbonate.dxf`).

## Electronics and wiring

| ID | Qty | Buy | Purpose / fit note |
|---|---:|---|---|
| E01 | 1 | **[Teensy 4.1](https://www.sparkfun.com/teensy-4-1.html)** — SparkFun DEV-16771 (or DEV-20359 no-Eth); PJRC TEENSY41, unpinned, non-lockable | Processor, USB, saved progress (pinout: S1); Bench |
| E02 | 1 | **[Audio Adapter](https://www.sparkfun.com/teensy-4-audio-shield-rev-d.html)** — SparkFun DEV-15845; PJRC TEENSY4_AUDIO Rev D or D2 (not Rev C) | Line input and conversion (pinout: S2); Bench |
| E03 | 1 | **[Round display](https://www.adafruit.com/product/6178)** — Adafruit 6178; GC9A01A 1.28in 240x240 TFT (wired 0.1in pads; EYESPI cable unused) | Documented driver and mounting geometry; Bench |
| E04 | 2 | **[Red finger button](https://www.sparkfun.com/momentary-button-panel-mount-red.html)** — SparkFun COM-11992, SPST normally open, 6.75mm thread, nut/washer | Mode / Quest; solder lugs; Bench |
| E05 | 1 | **[Momentary footswitch](https://www.amplifiedparts.com/products/footswitch-spst-momentary-solder-lugs-soft-switch-black)** — Amplified Parts P-H605-BLK; SPST normally open soft switch, M12x1 | Start/pause; not an audio bypass switch; Bench |
| E06 | 1 | **[Input jack](https://www.amplifiedparts.com/products/14-jack-switchcraft-111x-mono-enclosed-open-circuit)** — Switchcraft 111X (Amplified Parts W-SC-111X), 1/4in mono enclosed, nut/washer | Buffered dry input only; 0.276in bushing; Fit gate |
| E07 | 1 | **[10k trimmer](https://www.bourns.com/docs/product-datasheets/3386.pdf)** — Bourns 3386P-1-103LF, 10k cermet single-turn (DigiKey/Mouser) | Input level adjustment, tool operated; Bench |
| E08 | 1 | **Resistor R1** — 10k ohm, 1%, 0.25W, through-hole metal film | Input attenuator series arm; Bench |
| E09 | 2 | **Resistors R2 / R3** — 1k ohm, 1%, 0.25W, through-hole metal film | Audio series resistor / LED limit; Bench |
| E10 | 1 | **Resistor R4** — 10k ohm, 1%, 0.25W, through-hole metal film | Hold display SDCS high; Bench |
| E11 | 1 | **Status LED** — 3mm amber diffused, standard 2V 20mA through-hole LED | Playing indicator; Bench |
| E12 | 1 | **Perfboard stock** — 2.54mm FR4 plated-through protoboard; cut 14x28mm for input board | Hand-wired attenuator; no custom PCB needed; Bench |
| E13 | 1 | **[Header / socket set](https://www.sparkfun.com/female-headers.html)** — Two 14-pin male + two 14-pin female 2.54mm strips (e.g. SparkFun PRT-00116 + PRT-00115 cut to 14p) | Socketed audio stack, nominal 9.4mm gap; measure; Fit gate |
| E14 | 1 | **Wire and insulation** — 26AWG stranded hookup wire, shielded 1-pair audio cable, heat shrink, 2 narrow zip ties | Harness and strain relief; Bench |
| E15 | 1 | **[USB data cable](https://www.sparkfun.com/usb-micro-b-cable-6-foot.html)** — USB-A to Micro-B data cable (e.g. SparkFun CAB-10215; not charge-only); USB 5V only | Power and firmware upload; measure plug envelope; Bench |

## Mechanical parts and finishing

| ID | Qty | Buy / make | Purpose / fit note |
|---|---:|---|---|
| M01 | 1 | **Printed parts set** — A base, B lid, C charcoal bezel, E tray, K spacers, W gold rim + crimson cheek; O cap optional | PETG; coupon before full print; After fit checks |
| M02 | 1 | **Clear window** — 40mm diameter x1mm polycarbonate | Flat captured window; no dome in R1.1; After fit checks |
| M03 | 1 | **Foot reinforcement** — 55x32x2mm aluminum, R3 corners, hole12.4mm | Distributes switch force to four columns; After fit checks |
| M04 | 1 | **Metric hardware set** — See fastener schedule; buy spares | All removable threaded joints; After fit checks |
| M05 | 1 | **Mounting consumables** — Thin peripheral foam, 0.1mm cosmetic adhesive, insulating washers, feet / hook-loop | Rattle control, electrical clearance, mounting; After fit checks |

E13 is two rows of 14 connections: **two 14-pin male strips and two matching 14-pin female socket strips**, 2.54 mm pitch. Buy longer breakaway strips if convenient. Select the seated height with the actual boards; the modeled gap is 9.4 mm. Ensure soldered access to every used Teensy pin. This is a fit-gated specification, not a claim that every generic header set fits.

Small electrical totals: **two 10 kΩ fixed resistors, two 1 kΩ fixed resistors, one 10 kΩ trimmer, one 3 mm amber LED**. Prefer 1% 0.25 W through-hole resistors. The trimmer does not replace either fixed 10 kΩ resistor.

## Exact fastener schedule

| Location | Qty | Screw | Mating parts |
|---|---:|---|---|
| Case corners | 4 | M3 x10 pan head | M3 nuts x4; pockets 5.9mm across flats |
| Tray to base | 4 | M3 x6 pan head | M3 nuts x4; pockets 5.9mm across flats |
| Charcoal bezel to lid | 4 | M3 x12 pan head | M3 nuts x4, below lid |
| Display to lid | 2 | M2 x10, 90-degree countersunk | M2 nuts x2; printed 2.0mm spacers x2; heads flush with faceplate |
| Audio board to tray | 3 | M2 x25 pan head | M2 nuts x3; 15mm nylon spacers x3 |
| Perfboard to tray | 2 | M2 x12 pan head | M2 nuts x2; 6mm nylon spacers x2 |
| Footswitch | 1 | Included M12 hardware | Lower nut, metal plate, lid, top washer, top nut |
| Buttons / input jack | 3 | Included retaining hardware | Confirm panel thickness and remaining thread |

**Totals to order (plus a few spares):**

- M3 pan-head screws: 4 × 10 mm, 4 × 6 mm, 4 × 12 mm; **12 M3 nuts**.
- M2 screws: 2 × 10 mm, 90° countersunk; 3 × 25 mm pan head; 2 × 12 mm pan head; **7 M2 nuts**.
- Nylon spacers for M2 screws: **3 × 15 mm**, **2 × 6 mm**. Print the two 2 mm display spacers, or substitute measured equivalents.
- Use the included nuts/washer for the footswitch, two buttons and input jack. Verify actual thread engagement before adding optional insulating washers.

## Fabrication orders

- PETG: charcoal base/lid/bezel, crimson base-band/cheek/optional cap, muted-gold thin rim; internal tray and spacers can be any suitable color. See [printing instructions](PRINTING.md).
- Clear polycarbonate: **one 40 mm diameter × 1 mm disc**. Flat, deburred; no fishbowl lens in this revision.
- Aluminum: **one 55 × 32 × 2 mm plate**, R3 corners, **12.4 mm center hole**. Use the DXF and deburr. This is the foot-load reinforcement.
- Perfboard: cut **14 × 28 mm**, two **2.2 mm holes on 20 mm centers**; check pad layout before drilling.
- Thin peripheral anti-rattle foam for the window, about 0.1 mm adhesive for the cosmetic inserts, insulating material as needed, four small rubber feet or pedalboard hook-and-loop.
- Optional: cream/white decal labels and paint for the shallow panel marks. Do not fill moving clearances with paint.

## Tools and budget

Have a temperature-controlled soldering iron, solder/flux, fume extraction, multimeter, calipers, cutters, wire stripper, drivers and small files on hand. Arrange drilling/cutting and a PETG printer or fabrication service. No custom PCB fabrication is required for R1.1.

Planning allowance: **$127–$258**, roughly **$130–260** for parts and basic prototype materials. These are budget allowances, not a live cart or fabrication quote. Excludes labour, tools, shipping/tax and any external buffered splitter. Buy quantities in the Qty column; CSV allowance columns are per line, not per individual item.

The player's rig must provide a verified **buffered dry feed** that remains live while playing. If it does not, use an external buffered splitter in front of the pedalboard; R1.1 has no output jack and cannot take a raw passive guitar pickup directly through its line-level attenuator.

Machine-readable versions: [BOM.csv](../hardware/data/BOM.csv), [fasteners.csv](../hardware/data/fasteners.csv), [wiring.csv](../hardware/data/wiring.csv). Canonical editable source: `hardware/data/project_data.py`.
