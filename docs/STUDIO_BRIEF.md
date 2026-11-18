# VEX / Pocket Rival — brief for a design studio or fabrication partner

This brief explains what VEX is, what is locked in for the **current R1.1 prototype**, where you have full engineering latitude, and how R1.1 separates from a future V2 product revision. `make pack-studio` packages this brief alongside the STEP assembly, DXFs, BOM and media references.

## 1. What the device is

VEX is a small guitar practice companion (`86 × 146 × 45 mm` nominal) with a pixel-art bat on a round 1.28" (`32.4 mm` active, `240 × 240`) TFT display. While the player practises, VEX detects playing activity, awards participation XP, runs 5/10/20-minute quests, grants one fragment per completed session, and provides a single-note tuner.

In R1.1, VEX listens on a **buffered dry branch** of the player's rig (single ¼" input jack, USB 5 V power, no audio output or bypass switching). That keeps the first hardware build electrically simple while firmware, animations and game feel are refined on real guitars.

## 2. Current engagement: R1.1 prototype review + fabrication (Phase 1)

This round is a **3D-printed and laser-cut prototype enclosure** (not injection-moulding DFM or custom PCB layout), designed cleanly enough that we can reprint/recut a small handful of identical units if the first prototype goes well.

### Locked off-the-shelf electronics stack (`hardware/data/BOM.csv`)

Firmware across all four bring-up stages already compiles against these exact parts and pin assignments (`firmware/VEX_Pocket_Rival/config.h`):

| ID | Part | Exact SKU / specification | CAD model provenance |
|---|---|---|---|
| E01 | Teensy 4.1 | SparkFun `DEV-16771` (or `DEV-20359` no-Ethernet), unpinned | Community STEP — verify actual board, USB Micro-B plug reach and solder tails |
| E02 | Audio Adapter | SparkFun `DEV-15845` (PJRC Rev D / D2; not Rev C) | Community Rev D STEP — verify Rev D/D2 component heights and stacked header gap (`E13`, nominal `9.4 mm`) |
| E03 | Round TFT display | Adafruit `#6178` (`GC9A01A`, 1.28" `240 × 240`, wired 0.1" pads) | Official Adafruit STEP + Eagle PCB hole coordinates (`±11.43, -17.78 mm`) |
| E04 | 2 × Finger buttons | SparkFun `COM-11992` (red SPST momentary, `6.75 mm` thread) | Simplified envelope — verify nut, washer, lug tails and panel thickness |
| E05 | Footswitch | Amplified Parts `P-H605-BLK` (soft SPST momentary, `M12 × 1.0`, `25.5 × 13 × 14 mm` body) | Simplified envelope — verify travel, solder lugs and load transfer onto the 2 mm aluminum plate |
| E06 | Input jack | Switchcraft `111X` / Amplified Parts `W-SC-111X` (enclosed ¼" mono, `0.276"` / `7.01 mm` bushing) | Conservative `17.5 × 31 × 17.5 mm` reserve — verify bushing thread engagement, lugs and mated plug clearance |
| E07–E12 | Input attenuator + LED | Bourns `3386P-1-103LF` 10 kΩ trimmer, 2 × `10 kΩ` + 2 × `1 kΩ` resistors, 3 mm amber LED on `14 × 28 mm` FR4 protoboard | Nominal envelope on tray — verify clearance |

Purchasing shortcuts and exact M2/M3 screw, nut and nylon standoff counts are in `docs/SHOPPING.md` and `hardware/data/fasteners.csv`.

### Visual & functional intent to preserve

- **Palette and character:** charcoal body and octagonal bezel, thin muted-gold bezel ring (`W_gold_insert`), small crimson side cheek (`W_crimson_cheek`), red panel buttons, and a clear flat `40 × 1 mm` polycarbonate window (`D_clear_window`) captured without clamping directly on the bare LCD glass. Renders of the real STEP assembly are in `assets/marketing/`.
- **Controls and access:** two finger buttons (`MODE`, `QUEST`), one momentary footswitch supported by the internal `55 × 32 × 2 mm` aluminum reinforcement plate (`M_foot_reinforcement`), rear ¼" input jack, and rear USB Micro-B cable access to the Teensy.

### Where you have full engineering call in Phase 1

You have full authority to adjust enclosure geometry, wall thicknesses, bosses, split lines, tolerances, fastener sizes/locations, board/jack/switch offsets, wire-routing channels and the internal foot-load support path so that:

1. Every physical BOM component fits with real solder lugs, headers and cables installed (`hardware/generated/cad/Wiring_Reserves.step` shows the intended harness zones, including a ~`50 mm` lid-open service loop).
2. Footswitch presses transfer cleanly through the aluminum plate to the base structure without flexing the faceplate or stressing the electronics.
3. The enclosure assembles repeatably and reopens easily for firmware/wiring work without pinching wires or pulling self-threaded plastic.

### Phase 1 deliverables

1. **Fabricated prototype enclosure** (printed parts, laser/waterjet `2 mm` aluminum foot plate and `1 mm` clear polycarbonate window) shipped together with the sourced BOM hardware.
2. **Measured component dimensions** (`hardware/data/measurements.csv` or your own table).
3. **Updated STEP assembly / modified part STEPs** plus a short note listing what changed by part name, so we can keep the parametric CAD (`hardware/cad/build_cad.py`) and firmware/wiring docs in sync.

## 3. Later productisation (Phase 2 / V2 — out of scope for this quote)

Once firmware and daily playtesting on R1.1 settle, a separate V2 scope can address:

- **Custom carrier + guitar I/O PCB:** replacing the hand-wired protoboard and module stackup as appropriate, adding a studio-quiet `1 MΩ` analog input buffer + ¼" `OUT / THRU` jack (with unpowered relay pass-through and optional tuner-mute relay) plus standard 9 V center-negative pedalboard power conversion.
- **Small-batch or production enclosure DFM:** cast/machined aluminum or injection-moulded housing, optical window/lens refinement, and applicable EMC/safety compliance.

No V2 work is expected in the Phase 1 prototype review; if you spot an obvious small-batch pitfall while reviewing the STEP, a brief note is plenty.

## 4. File map

Paths below exist both in the repository (https://github.com/ash0ts/PedalPartner) and in `VEX_R1.1_Studio_Pack.zip`:

| Need | Open |
|---|---|
| Full 3D assembly | `hardware/generated/VEX_Pocket_Rival_Assembly.step` (five named groups, 72 solids, mm); `hardware/generated/cad/Wiring_Reserves.step` (cable/plug clearance zones); `docs/CAD_REVIEW.md` |
| Part provenance & dimensions | `hardware/generated/parts.json`, `assembly_tree.json`, `parameters.json`, `hardware/data/measurements.csv` |
| Cut profiles & print notes | `hardware/generated/drawings/Foot_Plate_2mm_Aluminum.dxf`, `Window_1mm_Polycarbonate.dxf`, `hardware/generated/stl/*.stl`, `docs/PRINTING.md`, `docs/VEX_Pocket_Rival_1to1_Templates.pdf` |
| Purchasing, fasteners & hookup | `docs/SHOPPING.md`, `hardware/data/BOM.csv`, `fasteners.csv`, `wiring.csv`, `docs/SOURCES.md`, `firmware/VEX_Pocket_Rival/config.h` |
| Assembly & bench sequence | `docs/VEX_Pocket_Rival_Mentor_Guide.pdf` (`docs/BUILD_GUIDE.md`), `docs/BRINGUP.md`, `docs/HANDOFF.md` |
| Visuals & UI behaviour | `assets/marketing/vex-hero.jpg`, `assets/motion/vex-motion.mp4`, `assets/motion/storyboard.png`, `assets/screens/contact_sheet.png` |
| Design rationale & V2 roadmap | `docs/DECISIONS.md`, `docs/EXTENSIBILITY.md` |
| Licences & CAD attribution | `LICENSE` (MIT for original work), `docs/THIRD_PARTY.md`, `hardware/vendor/PROVENANCE.md`, `hardware/vendor/ADAFRUIT_LICENSE.txt` |

## 5. Known prototype limits (what is not claimed yet)

- **Zero modeled solid overlap is not a physical fit guarantee:** `cad_validation.json` checks all 2,556 component pairs, but community board models and simplified switch/jack envelopes do not capture solder fillets, wire bend radii or real thread tolerances—hence the physical fit review.
- **No measured bench or load data yet:** `measurements.csv` and `acceptance_log.csv` are intentionally blank until physical parts are measured and tested; the printed shell has no force rating prior to load testing.
- **No PCB schematic file yet:** R1.1 electrical hookup is the point-to-point module table in `wiring.csv` plus the 3-component passive attenuator in the build guide.
- **Parametric STEP solids:** geometry is generated by `hardware/cad/build_cad.py` (`build123d`), so the STEP carries named solids and hierarchy rather than a native SolidWorks/Fusion feature timeline. Direct CAD edits on the STEP are completely fine for Phase 1.
