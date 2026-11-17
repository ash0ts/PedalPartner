"""Derive 1:1 XY cutting profiles directly from the exported solid parts."""
from pathlib import Path
from build123d import ExportDXF,section,Plane,import_step,Pos
ROOT=Path(__file__).resolve().parents[2]
R=ROOT/'hardware/generated'
for part,height,shift_y,name in [
 ('M_foot_reinforcement',41,46,'Foot_Plate_2mm_Aluminum'),
 ('D_clear_window',45.65,-32,'Window_1mm_Polycarbonate')]:
    solid=import_step(R/'cad'/f'{part}.step')
    profile=Pos(0,shift_y,-height)*section(solid,Plane.XY.offset(height))
    drawing=ExportDXF();drawing.add_shape(profile);drawing.write(R/'drawings'/f'{name}.dxf')
print('Cut profiles exported from the solid CAD; nominal mm, before tool kerf compensation.')
