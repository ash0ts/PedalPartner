# R1.1 CAD review and modification map

Start with [VEX_Pocket_Rival_Assembly.step](../hardware/generated/VEX_Pocket_Rival_Assembly.step). Units are millimetres. The enclosure is centered on X/Y, bottom at Z=0; +Y is the rear connector wall, -Y the foot end, +Z up. Nominal body: 86 × 146 × 45 mm; buttons/bezel extend above it.

## Assembly organization

| Group | Ownership and purpose |
|---|---|
| 01_Chassis | Base and captive case/tray nuts |
| 02_Front_Panel | Lid, bezel/window, display and its fixings, finger controls/LED, cosmetic inserts, case screws |
| 03_Electronics_Carrier | Removable tray, Teensy/headers, Audio Adapter and spacers, perfboard/attenuator and fixings |
| 04_Rear_IO | Input jack and retaining nut |
| 05_Footswitch | Switch, metal load plate, nuts/washer and optional cap |

The hierarchy nests related mounting hardware with the part it services. Every physical component appears once. The [assembly tree JSON](../hardware/generated/assembly_tree.json) and [part manifest](../hardware/generated/parts.json) make names, ownership and model provenance searchable. Five separate [subassembly STEPs](../hardware/generated/subassemblies) allow focused review. [Flat STEP](../hardware/generated/VEX_Pocket_Rival_Flat.step) is a compatibility fallback if an importer discards nested assembly structure.

`C_gold_bezel` retains its R1 identifier for continuity; **its R1.1 finish is charcoal**. Gold is now the separate `W_gold_insert`. `W_crimson_cheek` supplies the small side accent. Shallow chevrons and printed/decal labels add character without changing the controls. No oversized screen, protruding ears, custom dome or load-bearing decorative shell is implied.

## Which file to edit or fabricate

| Need | Correct source |
|---|---|
| Parametric construction | `hardware/cad/build_cad.py` |
| Assembly ownership | `hardware/cad/assembly_tree.py` |
| Interoperable review/direct solid editing | Grouped STEP; individual custom STEP parts in `hardware/generated/cad/` |
| 3D printing | Selected parts in `hardware/generated/stl/`; see PRINTING.md |
| Metal foot plate / clear window | DXFs in `hardware/generated/drawings/`, with stated material thickness |
| Clearance review | Separate `hardware/generated/cad/Wiring_Reserves.step` |
| Pictures | Blender renders of actual meshes; not dimensioned drawings |

STEP preserves solids and assembly names; **it does not contain a Fusion/FreeCAD feature timeline or constrained mechanical joints**. The Python construction is the editable source. A Fusion import can be directly edited, but those changes do not flow back into Python automatically. Record and transfer them before regeneration.

The `P` parameters drive shell dimensions and matching case posts. Several component placements remain explicit coordinates in the model. Changing a shell parameter does not automatically redesign board placement, mounting holes or the harness. Edit related interfaces together and rerun validation. Do not scale a purchased board or the entire assembly to make it fit.

## Repeatable review

```sh
python3 -m venv .venv
. .venv/bin/activate
python3 -m pip install -r requirements-cad.txt
make cad cad-check
```

The build checks solid validity and all 2,556 physical-part pairs. The re-import check verifies all 72 component labels, the five groups, each component's bounds/volume/solid count. Inspect [CAD report](../hardware/generated/checks/cad_validation.json) and [STEP report](../hardware/generated/checks/step_roundtrip.json). No overlap is modeled in R1.1. Checks exclude the intentional translucent wire/plug reserves from the physical solids.

Then inspect cross-sections and disassembly paths: rear jack lugs and inserted plug, USB reach, solder tails, audio-header stack, window capture, display screws, foot plate contact and lid service loop. Zero solid overlap does **not** prove tolerance clearance, connector insertion, wire bend radius, thread engagement or strength. Purchased switches/jack are simplified envelopes; community board models are not supplier certifications.

Before printing: measure the actual boards and connector hardware, complete `hardware/data/measurements.csv`, print the fit coupon, test the real screen at board angle, and review the foot-load path with the mentor. Build the device in the order shown in the guide.
