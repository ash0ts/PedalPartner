"""Generate a human shopping list from the canonical BOM and fastener schedule."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'hardware/data'))
from project_data import BOM, FASTENERS, SOURCES

def write_shopping(root):
    urls = {key: url for key, title, url in SOURCES}
    lines = ['# Final R1.1 shopping list', '',
        'This list matches the current CAD and wiring. Buy the bench electronics first, or source one complete set for CAD fit verification. Product links point to live retail listings checked November 2026; stock and prices can change.', '',
        '## Quick vendor consolidation', '',
        '- **SparkFun** (official Teensy manufacturer/retailer): Teensy 4.1 unpinned (`DEV-16771`, or `DEV-20359` without Ethernet), Teensy 4 Audio Shield Rev D (`DEV-15845`), 2 × red panel buttons (`COM-11992`), 2.54 mm breakaway male + female headers (`PRT-00116` + `PRT-00115`, trimmed to two 14-pin rows), USB Micro-B data cable (`CAB-10215`).',
        '- **Adafruit**: 1.28" 240×240 round GC9A01A TFT (`#6178`). Do **not** substitute a square ST7789 breakout; the optional EYESPI flex cable is unused because VEX wires to the 0.1" pads.',
        '- **Amplified Parts**: soft SPST momentary footswitch (`P-H605-BLK`) and Switchcraft `111X` enclosed ¼" mono jack (`W-SC-111X`, includes nut and washer).',
        '- **DigiKey / Mouser / McMaster / shop stock**: Bourns `3386P-1-103LF` 10 kΩ trimmer, two `10 kΩ` and two `1 kΩ` 1% 0.25 W through-hole resistors, 3 mm amber LED, 2.54 mm FR4 protoboard, metric M2/M3 screws, nuts and M2 nylon spacers (schedule below), 2 mm aluminum sheet (`Foot_Plate_2mm_Aluminum.dxf`), and 1 mm clear polycarbonate (`Window_1mm_Polycarbonate.dxf`).', '',
        '## Electronics and wiring', '', '| ID | Qty | Buy | Purpose / fit note |', '|---|---:|---|---|']
    for row in BOM:
        ident, qty, name, spec, purpose, source, low, high, stage = row
        if ident == 'M01':
            lines += ['', '## Mechanical parts and finishing', '', '| ID | Qty | Buy / make | Purpose / fit note |', '|---|---:|---|---|']
        title = f'[{name}]({urls[source]})' if source in urls else name
        lines.append(f'| {ident} | {qty} | **{title}** — {spec} | {purpose}; {stage} |')
    lines += ['', 'E13 is two rows of 14 connections: **two 14-pin male strips and two matching 14-pin female socket strips**, 2.54 mm pitch. Buy longer breakaway strips if convenient. Select the seated height with the actual boards; the modeled gap is 9.4 mm. Ensure soldered access to every used Teensy pin. This is a fit-gated specification, not a claim that every generic header set fits.', '',
        'Small electrical totals: **two 10 kΩ fixed resistors, two 1 kΩ fixed resistors, one 10 kΩ trimmer, one 3 mm amber LED**. Prefer 1% 0.25 W through-hole resistors. The trimmer does not replace either fixed 10 kΩ resistor.', '',
        '## Exact fastener schedule', '', '| Location | Qty | Screw | Mating parts |', '|---|---:|---|---|']
    lines += ['| ' + ' | '.join(map(str,row)) + ' |' for row in FASTENERS]
    lines += ['', '**Totals to order (plus a few spares):**', '',
        '- M3 pan-head screws: 4 × 10 mm, 4 × 6 mm, 4 × 12 mm; **12 M3 nuts**.',
        '- M2 screws: 2 × 10 mm, 90° countersunk; 3 × 25 mm pan head; 2 × 12 mm pan head; **7 M2 nuts**.',
        '- Nylon spacers for M2 screws: **3 × 15 mm**, **2 × 6 mm**. Print the two 2 mm display spacers, or substitute measured equivalents.',
        '- Use the included nuts/washer for the footswitch, two buttons and input jack. Verify actual thread engagement before adding optional insulating washers.', '',
        '## Fabrication orders', '',
        '- PETG: charcoal base/lid/bezel, crimson base-band/cheek/optional cap, muted-gold thin rim; internal tray and spacers can be any suitable color. See [printing instructions](PRINTING.md).',
        '- Clear polycarbonate: **one 40 mm diameter × 1 mm disc**. Flat, deburred; no fishbowl lens in this revision.',
        '- Aluminum: **one 55 × 32 × 2 mm plate**, R3 corners, **12.4 mm center hole**. Use the DXF and deburr. This is the foot-load reinforcement.',
        '- Perfboard: cut **14 × 28 mm**, two **2.2 mm holes on 20 mm centers**; check pad layout before drilling.',
        '- Thin peripheral anti-rattle foam for the window, about 0.1 mm adhesive for the cosmetic inserts, insulating material as needed, four small rubber feet or pedalboard hook-and-loop.',
        '- Optional: cream/white decal labels and paint for the shallow panel marks. Do not fill moving clearances with paint.', '',
        '## Tools and budget', '',
        'Have a temperature-controlled soldering iron, solder/flux, fume extraction, multimeter, calipers, cutters, wire stripper, drivers and small files on hand. Arrange drilling/cutting and a PETG printer or fabrication service. No custom PCB fabrication is required for R1.1.', '',
        f'Planning allowance: **${sum(r[6] for r in BOM):.0f}–${sum(r[7] for r in BOM):.0f}**, roughly **$130–260** for parts and basic prototype materials. These are budget allowances, not a live cart or fabrication quote. Excludes labour, tools, shipping/tax and any external buffered splitter. Buy quantities in the Qty column; CSV allowance columns are per line, not per individual item.', '',
        'The player\'s rig must provide a verified **buffered dry feed** that remains live while playing. If it does not, use an external buffered splitter in front of the pedalboard; R1.1 has no output jack and cannot take a raw passive guitar pickup directly through its line-level attenuator.', '',
        'Machine-readable versions: [BOM.csv](../hardware/data/BOM.csv), [fasteners.csv](../hardware/data/fasteners.csv), [wiring.csv](../hardware/data/wiring.csv). Canonical editable source: `hardware/data/project_data.py`.']
    (root/'docs/SHOPPING.md').write_text('\n'.join(lines)+'\n')

if __name__ == '__main__':
    write_shopping(Path(__file__).resolve().parents[1])
