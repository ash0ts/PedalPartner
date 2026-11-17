"""VEX Pocket Rival R1.1. Units mm. Python/build123d is the parametric master.
Geometric prototype; purchased-component envelopes are identified in parts.json.
Run from any directory: python build_cad.py
"""
from build123d import *
from pathlib import Path
import json, itertools, math, csv, importlib.metadata
ROOT=Path(__file__).resolve().parents[2]
R=ROOT/'hardware/generated'
VENDOR=ROOT/'hardware/vendor'
R.mkdir(parents=True,exist_ok=True)
for d in ['cad','stl','meshes','checks','drawings','renders']: (R/d).mkdir(exist_ok=True)
P=dict(width=86.,length=146.,base_h=42.,lid_t=3.,wall=3.,floor=3.,corner=12.,display_y=32.,display_plane=40.0,stack_x=-12.,button_y=-12.,foot_y=-46.)
parts={}; meta={}
def box(w,l,h,x=0,y=0,z=0):return Pos(x,y,z)*Box(w,l,h,align=(Align.CENTER,Align.CENTER,Align.MIN))
def cy(r,h,x=0,y=0,z=0):return Pos(x,y,z)*Cylinder(r,h,align=(Align.CENTER,Align.CENTER,Align.MIN))
def rr(w,l,h,r=3,x=0,y=0,z=0):return Pos(x,y,z)*extrude(RectangleRounded(w,l,r),amount=h)
def ring(ro,ri,h,x=0,y=0,z=0):return cy(ro,h,x,y,z)-cy(ri,h+.2,x,y,z-.1)
def hexagon(af,h,x,y,z):return Pos(x,y,z)*extrude(RegularPolygon(af/math.sqrt(3),6,rotation=30),amount=h)
def add(n,s,kind,note='',group='base'):
 s.label=n; parts[n]=s;meta[n]=dict(kind=kind,note=note,group=group);return s
def screw(n,x,y,head_z,length=10,diam=3):
 return add(n,cy(diam/2,length,x,y,head_z-length)+cy(diam,1.8,x,y,head_z),'hardware','Smooth nominal fastener envelope; thread not modeled.','fasteners')
def nut(n,x,y,z,diam=3):
 af=5.5 if diam==3 else 4.;h=2.4 if diam==3 else 1.6
 return add(n,hexagon(af,h,x,y,z)-cy(diam/2,h+.2,x,y,z-.1),'hardware','Nominal metric nut envelope.','fasteners')
CASE=[(x,y) for x in [-(P["width"]/2-9),P["width"]/2-9] for y in [-(P["length"]/2-9),P["length"]/2-9]]
TRAY=[(x,y) for x in [-26,26] for y in [-6,60]]
BEZEL=[(x,y) for x in [-19,19] for y in [13,51]]
DISPLAY=[(-11.43,14.22),(11.43,14.22)]
AUDIO=[(-29.42,36.71),(.88,24.11),(.88,54.11)]
# A: base has floor, captive nuts, removable tray support, and four foot-load columns.
base=rr(P['width'],P['length'],42,12)-rr(P["width"]-6,P["length"]-6,42,9,z=3)
for x,y in CASE:
 base+=cy(5,39,x,y,3);base-=cy(1.7,12,x,y,31);base-=hexagon(5.9,2.7,x,y,39.3)
for x,y in TRAY:
 base+=cy(4.5,2,x,y,3);base-=cy(1.7,6,x,y);base-=hexagon(5.9,2.7,x,y,2.3)
for x in [-23,23]:
 for y in [-57,-35]:base+=box(8,8,37,x,y,3)
# Rear service portal: USB cable reaches the Teensy's own connector. No panel USB assumed.
base-=box(18,10,12,-12,72,10)
# Switchcraft 111X, 9.8mm nominal panel hole. Coupon verifies actual thread and retention.
base-=Pos(18,68,20)*Rot(-90,0,0)*Cylinder(4.9,12,align=(Align.CENTER,Align.CENTER,Align.MIN))
add('A_base',base,'printed','PETG geometric prototype. Four columns support the reinforcement plate; strength not validated.')
# B: faceplate. The TFT itself projects into the central clearance opening.
lid=rr(P["width"],P["length"],3,12,z=42)-cy(18.3,5,0,32,41)
for x,y in CASE:lid-=cy(1.7,5,x,y,41)
for x,y in BEZEL:lid-=cy(1.7,5,x,y,41)
for x,y in DISPLAY:
 lid-=cy(1.2,5,x,y,41)
 lid-=Pos(x,y,44)*Cone(1.2,2.2,1.001,align=(Align.CENTER,Align.CENTER,Align.MIN))
for x in [-17,17]:lid-=cy(3.6,5,x,-12,41)
lid-=cy(6.2,5,0,-46,41)
lid-=cy(1.6,5,0,-27,41)
# Shallow chevron grooves, paint-fill optional; no change to control mounting stack.
for x,angle in [(-17,-12),(17,12)]:
 lid-=Pos(x,-22,44.65)*Rot(0,0,angle)*Box(20,0.7,0.5,align=(Align.CENTER,Align.CENTER,Align.MIN))
add('B_faceplate',lid,'printed','3mm flat faceplate. Hand control only until the mentor verifies mechanical load path.','lid')
# C: bezel captures a flat 40 x 1 mm clear disc using a 40.6 x 1.3 mm underside pocket.
bezel=Pos(0,32,45)*extrude(Polygon((-23,-31),(23,-31),(31,-23),(31,23),(23,31),(-23,31),(-31,23),(-31,-23)),amount=4)
bezel-=cy(17.3,6,0,32,44)
bezel-=cy(20.3,1.3,0,32,44.999)
# A thin gold insert sits in a shallow adhesive-backed pocket.
bezel-=ring(20.2,17.1,1.0,0,32,48.1)
for x,y in BEZEL:bezel-=cy(1.7,6,x,y,44)
add('C_gold_bezel',bezel,'printed','Charcoal clipped bezel, 62x62; aperture34.6. Bottom captures flat window; top pocket accepts thin gold insert.','lid')
add('D_clear_window',cy(20,1,0,32,45.15),'cut_sheet','40mm diameter, 1mm clear polycarbonate disc. Floating capture; no clamp load on LCD.','lid')
add('W_gold_insert',ring(20.0,17.3,0.8,0,32,48.2),'printed','Gold PETG,0.8mm. Thin adhesive at pocket bottom; keep optical aperture clear.','cosmetics')
accent=Pos(34,29,45.1)*extrude(Polygon((-2.5,-14),(2.5,-11),(2.5,11),(-2.5,14)),amount=0.9)
for y in [25,33]:accent-=box(3,0.7,0.35,34,y,45.75)
add('W_crimson_cheek',accent,'printed','Flat crimson accent;0.1mm adhesive gap. Cosmetic only.','cosmetics')
# Separate electronics carrier: cable-tie slots retain the Teensy below the audio shield.
tray=rr(72,78,3,4,y=25,z=5)
for x,y in CASE:tray-=cy(5.6,5,x,y,4)
for x,y in TRAY:tray-=cy(1.7,5,x,y,4)
for x,y in AUDIO:tray-=cy(1.1,5,x,y,4)
tray-=rr(15,43,5,2,x=-12,y=20,z=4)
for x in [-26,2]:tray-=box(2.5,15,5,x,18,4)
for y in [2,22]:tray-=cy(1.1,5,24,y,4)
add('E_electronics_tray',tray,'printed','72x78x3. Four M3 fixings. Slots for two narrow nylon ties around the Teensy, without crushing components.','stack')
teensy=Pos(-12,26,12)*Rot(90,0,0)*import_step(VENDOR/'teensy41_community.step')
add('F_teensy41',teensy,'community','Prior community model. Verify actual board revision, headers, USB plug and solder tails.','stack')
audio=Pos(-32.32,56.31,23)*Rot(0,0,-90)*import_step(VENDOR/'audio_shield_community.step')
add('G_audio_adapter',audio,'community','Prior community Rev D envelope. D2 is electrically compatible but mounting/component geometry needs measurement.','stack')
display=Pos(0,32,40.0)*Rot(0,180,0)*import_step(VENDOR/'adafruit_6178.step')
add('H_round_display_6178',display,'manufacturer','Adafruit STEP SHA 66f39106daee442a08a0b5f994fe7cbfb830bd64; PCB mounting holes confirmed in Eagle at +/-11.43,-17.78.','display')
for i,(x,y) in enumerate(AUDIO,1):
 add(f'I_audio_spacer_{i}',ring(2.5,1.1,15,x,y,8),'hardware','M2 spacer, 15mm; check actual stack.','stack')
 screw(f'I_audio_screw_{i}',x,y,5,25,2)
 # Screw modeled downward by helper: reverse this below to point up from beneath tray.
 parts[f'I_audio_screw_{i}']=cy(1,25,x,y,5)+cy(1.9,1.6,x,y,3.4);parts[f'I_audio_screw_{i}'].label=f'I_audio_screw_{i}'
 nut(f'I_audio_nut_{i}',x,y,24.6,2)
for x in [-19.62,-4.38]:
 add('J_header_'+str(x),box(2.54,35.56,9.4,x,38.7,13.6),'envelope','14-pin header stack reserve; select sockets by measured seated height.','stack')
for i,(x,y) in enumerate(DISPLAY,1):
 add(f'K_display_spacer_{i}',ring(2.3,1.1,2.,x,y,40.),'printed','2.0mm spacer; front PCB surface to faceplate underside.','display')
 add(f'K_display_screw_{i}',cy(1,9,x,y,35)+Pos(x,y,44)*Cone(1,2,1,align=(Align.CENTER,Align.CENTER,Align.MIN)),'hardware','M2x10 90-degree countersunk screw, flush at z45. Verify actual head diameter and seating.','display')
 nut(f'K_display_nut_{i}',x,y,36.83,2)
# Controls represented with conservative purchased-part envelopes, not invented vendor detail.
for x,n in [(-17,'MODE'),(17,'QUEST')]:
 s=cy(4.5,15.9,x,-12,21.1)+cy(3.375,8,x,-12,37)+cy(4.5,3,x,-12,45)
 add('L_'+n+'_button',s,'envelope','SparkFun COM-11992: 6.75mm thread, 26.9mm overall. Conservative simplified body; measure nut and cap.','controls')
 add('L_'+n+'_nut',ring(5.5,3.6,2,x,-12,40),'hardware','Use included button hardware; envelope only.','controls')
plate=rr(55,32,2,3,y=-46,z=40)-cy(6.2,4,0,-46,39)
add('M_foot_reinforcement',plate,'cut_sheet','55x32x2 aluminum; sits on four base columns. Hole12.4. Not a strength certification.','controls')
foot=box(25.5,13,14,0,-46,23)+cy(6,14.3,0,-46,37)+cy(5,7.7,0,-46,51.3)
add('N_momentary_footswitch',foot,'envelope','Amplified Parts P-H605-BLK, M12x1, 25.5x13x14 body. Shaft travel and solder tails require physical clearance check.','controls')
add('N_switch_lower_nut',hexagon(14,3,0,-46,37)-cy(6,3.2,0,-46,36.9),'hardware','Lower retaining nut bears on reinforcement plate.','controls')
add('N_switch_washer',ring(8.5,6.2,1,0,-46,45),'hardware','Use included washer.','controls')
add('N_switch_nut',hexagon(14,3,0,-46,46)-cy(6,3.2,0,-46,45.9),'hardware','Use included retaining nut; nominal envelope.','controls')
cap=cy(8.5,5,0,-46,56)-cy(5.2,3.8,0,-46,55.9)
add('O_crimson_cap',cap,'printed','Optional removable finger cap, 10.4mm bore. Print coupon first; omit if it binds travel.','controls')
add('P_status_LED',cy(1.5,5,0,-27,42),'envelope','3mm amber LED; leads and resistor insulated below lid.','controls')
# Input jack is a conservative reserve; manufacturer STEP not available in this package.
jack=box(17.5,31,17.5,18,54,11.25)+Pos(18,69.5,20)*Rot(-90,0,0)*Cylinder(4.75,9,align=(Align.CENTER,Align.CENTER,Align.MIN))
jack-=Pos(18,69,20)*Rot(-90,0,0)*Cylinder(3.2,12,align=(Align.CENTER,Align.CENTER,Align.MIN))
add('Q_input_jack_111X',jack,'envelope','Switchcraft111X selected. Body envelope17.5x31x17.5 is a design reserve, not verified manufacturer geometry. Verify before printing base.','base')
add('Q_jack_nut',Pos(18,74,20)*Rot(-90,0,0)*(hexagon(14,2,0,0,0)-cy(4.9,2.2,z=-.1)),'hardware','Nominal rear retaining hardware. Verify purchased jack stack.','base')
# A drilled piece of perfboard carries only the attenuator. Hand-built, not a PCB fabrication file.
perf=box(14,28,1.6,24,12,14)
for y in [2,22]:perf-=cy(1.1,3,24,y,13)
add('R_input_perfboard',perf,'cut_sheet','14x28x1.6 perfboard cut/drilled by mentor. M2 holes x0,y+/-10 from board centre.','stack')
for i,y in enumerate([2,22],1):
 add(f'R_perf_spacer_{i}',ring(2.5,1.1,6,24,y,8),'hardware','M2 6mm nylon spacer.','stack')
 screw(f'R_perf_screw_{i}',24,y,15.6,12,2);nut(f'R_perf_nut_{i}',24,y,3.4,2)
add('S_trim_10k',box(10,10,7,24,12,15.6),'envelope','Bourns3386P-1-103LF, conservative envelope. Board pad arrangement is documented separately.','stack')
for i,(x,y) in enumerate(CASE,1):screw(f'T_case_screw_{i}',x,y,45,10);nut(f'T_case_nut_{i}',x,y,39.3)
for i,(x,y) in enumerate(TRAY,1):screw(f'U_tray_screw_{i}',x,y,8,6);nut(f'U_tray_nut_{i}',x,y,2.3)
for i,(x,y) in enumerate(BEZEL,1):screw(f'V_bezel_screw_{i}',x,y,49,12);nut(f'V_bezel_nut_{i}',x,y,39.6)
# Explicit reserved space for wiring. Export separately; not a physical component.
reserves={'USB_plug':box(14,19,9,-12,65,10),'display_wire_service':box(30,10,5,0,7,31),'right_wire_lane':box(5,70,12,36,14,10)}
export_step(Compound(children=list(reserves.values())),R/'cad/Wiring_Reserves.step')
# Fit coupon. Independent from assembly; has the same nominal holes and nut pocket.
coupon=rr(72,28,3,3)
for x,r in [(-26,1.7),(-16,3.6),(0,6.2),(20,4.9)]:coupon-=cy(r,5,x,0,-1)
coupon-=hexagon(5.9,2.7,30,0,.3)
coupon-=cy(1.2,5,-26,9,-1)
coupon-=Pos(-26,9,2)*Cone(1.2,2.2,1.001,align=(Align.CENTER,Align.CENTER,Align.MIN))
export_step(coupon,R/'cad/Fit_Coupon.step');export_stl(coupon,R/'stl/Fit_Coupon.stl')
colors={'printed':(.12,.13,.15),'manufacturer':(.13,.27,.25),'community':(.12,.27,.25),'envelope':(.27,.28,.30),'hardware':(.5,.51,.53),'cut_sheet':(.55,.57,.59)}
facts=[]
for n,s in parts.items():
 c=(.65,.40,.12) if n=='W_gold_insert' else (.36,.035,.06) if n.startswith(('O_','L_','W_crimson')) and 'nut' not in n else colors[meta[n]['kind']]
 if meta[n]['kind']=='hardware' and n.startswith(('T_case_screw','V_bezel_screw','N_')):c=(.10,.11,.12)
 s.color=Color(*c);meta[n]['color']=c
 b=s.bounding_box(); facts.append(dict(name=n,valid=s.is_valid,solids=len(s.solids()),volume_mm3=round(s.volume,3),min=list(b.min),max=list(b.max),**meta[n]))
 export_stl(s,R/'meshes'/f'{n}.stl',tolerance=.08,angular_tolerance=.15)
 if meta[n]['kind'] in ['printed','cut_sheet']:
  export_step(s,R/'cad'/f'{n}.step')
  print_shape=Rot(180,0,0)*s if n=='C_gold_bezel' else s
  pb=print_shape.bounding_box();center=pb.center()
  print_shape=Pos(-center.X,-center.Y,-pb.min.Z)*print_shape
  export_stl(print_shape,R/'stl'/f'{n}.stl',tolerance=.05,angular_tolerance=.1)
from assembly_tree import build_tree
assembly,tree=build_tree(parts,meta)
(R/'assembly_tree.json').write_text(json.dumps(tree,indent=2))
export_step(assembly,R/'VEX_Pocket_Rival_Assembly.step')
export_step(Compound(label='VEX_R1_1_Flat',children=[s.moved(Location()) for s in parts.values()]),R/'VEX_Pocket_Rival_Flat.step')
(R/'subassemblies').mkdir(exist_ok=True)
for group in assembly.children:export_step(group,R/'subassemblies'/f'{group.label}.step')
export_gltf(assembly,R/'VEX_Pocket_Rival_Assembly.glb',binary=True)
check=list(parts)
collisions=[];pairs=0
for a,b in itertools.combinations(check,2):
 pairs+=1
 ba,bb=parts[a].bounding_box(),parts[b].bounding_box()
 if any(getattr(ba.max,k)<getattr(bb.min,k)-1e-6 or getattr(bb.max,k)<getattr(ba.min,k)-1e-6 for k in ['X','Y','Z']):continue
 if parts[a].distance_to(parts[b])>1e-5:continue
 inter=parts[a]&parts[b];vol=inter.volume if inter is not None else 0
 if vol>.02:collisions.append(dict(a=a,b=b,volume_mm3=round(vol,3)))
result=dict(parameters=P,parts=facts,checked_part_pairs=pairs,collisions=collisions,
 clearances_mm={'display_to_audio':round(display.distance_to(audio),3),'display_to_window':round(display.distance_to(parts['D_clear_window']),3),'jack_to_audio':round(jack.distance_to(audio),3),'teensy_to_tray':round(teensy.distance_to(tray),3)},
 scope='All component pairs, using broad-phase bounds then distance/intersection. Smooth envelopes, not threads. Vendor internal contacts are not separate parts. Does not validate physical fit, wiring, heat, optics or strength.',
 cad_version=importlib.metadata.version('build123d'))
(R/'checks/cad_validation.json').write_text(json.dumps(result,indent=2))
(R/'parts.json').write_text(json.dumps(meta,indent=2))
(R/'parameters.json').write_text(json.dumps(P,indent=2))
print(json.dumps({k:v for k,v in result.items() if k!='parts'},indent=2),flush=True)
