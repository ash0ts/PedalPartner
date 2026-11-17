# Fabrication and fit gate

## Print these parts

- `A_base.stl`: PETG, open side up. Charcoal body; optionally change filament after the first 3 mm for the crimson base band.
- `B_faceplate.stl`: PETG, flat; charcoal.
- `C_gold_bezel.stl`: PETG, pocket facing up; muted gold/ochre.
- `E_electronics_tray.stl`: PETG, flat.
- `K_display_spacer_1.stl`, `K_display_spacer_2.stl`: nominal 2.0 mm thickness; actual washer/spacer stack may replace these after measurement.
- `O_crimson_cap.stl`: optional fit experiment. Its 10.4 mm bore is not a guaranteed press fit. Omit it if loose or if it limits switch travel; a purchased compatible cap is an alternative.
- Print `Fit_Coupon.stl` first.

Starting print profile:0.2 mm layers,5 perimeters,6 solid top/bottom layers,35% infill. This profile has not been printed or load tested. Inspect layer adhesion, dimensions and holes with your mentor.

## Cut these parts, do not print them for their final function

- `M_foot_reinforcement`:2 mm aluminum,55x32 mm,3 mm corner radius,12.4 mm centre hole. The DXF profile is in `drawings/`.
- `D_clear_window`:1 mm clear polycarbonate,40 mm diameter. Deburr and leave the protective film on until assembly. Use the DXF profile; printer resin is not a substitute for an optically clear, impact-suitable window.
- `R_input_perfboard`: hand-cut 14x28 mm perfboard, two 2.2 mm holes on a20 mm centre spacing. Check pad locations before drilling; rotate/relocate components as required while preserving the schematic.

## Never print

The combined assembly, vendor PCBs, switch/jack envelopes, fasteners or `Wiring_Reserves.step` are references, not printable replacements.

## Measure before release

The Adafruit display mounting coordinates come from its PCB source, but purchased revisions still need checking. The Teensy/audio geometry is community-sourced. Header gap, exact RevD/D2 component heights, jack body/retention and button hardware are not guaranteed by the model.

The footswitch plate rests on four columns at x=+/-23 mm, y=-57/-35 mm. Confirm actual contact at all four pads. Test loading with electronics removed and a mentor-defined protocol before foot operation. No force rating is claimed for the printed enclosure.

## Editing CAD

`python build_cad.py` regenerates custom STEP/STL parts, combined STEP/GLB and the collision report. Use build 123d0.10.0. The Python model is the editable construction source; STEP is the interoperable solid assembly, not a native Fusion timeline.

The width/length values in `P` drive the outside shell, matching lid and case-post centres. Other layout values are explicit in the construction code; update related parts together. A larger case does not automatically relocate the screen or wiring. Every change needs geometry, fit and print checks again.

Print the templates at 100%, measure their 50 mm ruler, then compare with the actual part. Do not use a screenshot or a fit-to-page print as a drill template.
