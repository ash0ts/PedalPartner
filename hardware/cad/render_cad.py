"""Blender renders of the exported assembly meshes. Run blender -b --python render_cad.py."""
import bpy,math,json,os
from pathlib import Path
from mathutils import Vector
from bpy_extras.object_utils import world_to_camera_view
ROOT=Path(__file__).resolve().parents[2]
R=ROOT/'hardware/generated'
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
sc=bpy.context.scene;sc.render.engine='CYCLES';sc.cycles.samples=48;sc.cycles.use_denoising=False
sc.render.resolution_x=1300;sc.render.resolution_y=1150;sc.render.resolution_percentage=100
sc.render.image_settings.file_format='PNG';sc.render.film_transparent=True
sc.world.color=(.5,.5,.5);sc.view_settings.view_transform='Standard'
def material(n,c,metal=0,rough=.55):
 m=bpy.data.materials.new(n);m.diffuse_color=(*c,1);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*c,1);p.inputs['Metallic'].default_value=metal;p.inputs['Roughness'].default_value=rough;return m
black=material('Warm charcoal',(.065,.064,.075));red=material('Crimson',(.38,.045,.065));gold=material('Ochre bezel',(.70,.46,.16));metal=material('Steel hardware',(.50,.52,.54),.55);green=material('PCB',(.08,.22,.18));cream=material('Warm print',(.86,.83,.74));active=material('New part',(.76,.23,.10));gray=material('Nylon',(.60,.61,.62));glass=material('Clear window',(.9,.94,.95),0,.12)
p=glass.node_tree.nodes.get('Principled BSDF');p.inputs['Transmission Weight'].default_value=1;p.inputs['IOR'].default_value=1.49
parts={};defaults={};meta=json.loads((R/'parts.json').read_text());projections=json.loads((R/'renders/projections.json').read_text()) if (R/'renders/projections.json').exists() else {}
for f in (R/'meshes').glob('*.stl'):
 bpy.ops.wm.stl_import(filepath=str(f));o=bpy.context.object;o.name=f.stem
 k=meta[o.name]['kind'];m=metal if k=='hardware' else green if o.name.startswith(('F_','G_','H_','R_input')) else gray if 'spacer' in o.name else black
 if o.name.startswith(('O_','L_')) and 'nut' not in o.name:m=red
 if o.name=='W_gold_insert':m=gold
 if o.name=='W_crimson_cheek':m=red
 if o.name.startswith(('T_case_screw','V_bezel_screw','N_')):m=black
 if o.name.startswith('D_'):m=glass
 if o.name.startswith('M_'):m=metal
 if o.name.startswith('P_'):m=gold
 o.data.materials.clear();o.data.materials.append(m);defaults[o.name]=m;parts[o.name]=o
 if o.name=='A_base':
  band=black.copy();band.name='Charcoal with 3mm crimson base';nodes=band.node_tree.nodes;links=band.node_tree.links
  geom=nodes.new('ShaderNodeNewGeometry');sep=nodes.new('ShaderNodeSeparateXYZ');less=nodes.new('ShaderNodeMath');less.operation='LESS_THAN';less.inputs[1].default_value=3.01
  mix=nodes.new('ShaderNodeMixRGB');mix.inputs[1].default_value=(.065,.064,.075,1);mix.inputs[2].default_value=(.38,.045,.065,1)
  links.new(geom.outputs['Position'],sep.inputs[0]);links.new(sep.outputs['Z'],less.inputs[0]);links.new(less.outputs[0],mix.inputs[0]);links.new(mix.outputs[0],nodes.get('Principled BSDF').inputs['Base Color'])
  o.data.materials[0]=band;defaults[o.name]=band
  o.data.materials.append(red)
  for face in o.data.polygons:
   if face.center.z<3.01:face.material_index=1
decor={}
def label(n,text,loc,size,mat=cream):
 c=bpy.data.curves.new(n,'FONT');c.body=text;c.align_x='CENTER';c.size=size;c.extrude=0
 o=bpy.data.objects.new(n,c);bpy.context.collection.objects.link(o);o.location=loc;c.materials.append(mat);decor[n]=o
label('labelVEX','V E X',(0,-64,45.06),4.3)
label('labelMODE','MODE',(-17,-21,45.06),2.7)
label('labelQUEST','QUEST',(17,-21,45.06),2.7)
label('labelUSB','USB 5V',(-12,65,45.06),2.0)
label('labelIN','DRY IN',(18,65,45.06),2.0)
img=bpy.data.images.load(str(ROOT/'assets/screen.png'));img.pack()
mesh=bpy.data.meshes.new('round screen');vertices=[(0,32,41.77)]+[(16.2*math.cos(i*2*math.pi/96),32+16.2*math.sin(i*2*math.pi/96),41.77) for i in range(96)]
faces=[(0,i+1,(i+1)%96+1) for i in range(96)];mesh.from_pydata(vertices,[],faces);mesh.update();uv=mesh.uv_layers.new()
for p in mesh.polygons:
 for li in p.loop_indices:
  co=mesh.vertices[mesh.loops[li].vertex_index].co;uv.data[li].uv=(co.x/32.4+.5,(co.y-32)/32.4+.5)
o=bpy.data.objects.new('screenUI',mesh);bpy.context.collection.objects.link(o)
m=bpy.data.materials.new('Actual firmware sprite');m.use_nodes=True;nodes=m.node_tree.nodes;nodes.clear();out=nodes.new('ShaderNodeOutputMaterial');em=nodes.new('ShaderNodeEmission');tex=nodes.new('ShaderNodeTexImage');tex.image=img;tex.interpolation='Closest';em.inputs['Strength'].default_value=.8;m.node_tree.links.new(tex.outputs['Color'],em.inputs['Color']);m.node_tree.links.new(em.outputs[0],out.inputs['Surface']);o.data.materials.append(m);decor[o.name]=o
for loc,energy,size in [((80,-120,250),300000,190),((-180,-20,150),220000,190),((50,200,200),160000,170)]:
 bpy.ops.object.light_add(type='AREA',location=loc);l=bpy.context.object;l.data.energy=energy;l.data.size=size;l.rotation_euler=(Vector((0,0,30))-l.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add();cam=bpy.context.object;cam.data.type='ORTHO';sc.camera=cam
def render(name,visible=None,offsets=None,highlight=None,view='iso',scale=190,target=(0,0,25),labels=False,translations=None):
 if os.environ.get('RENDER_ONLY') and name not in os.environ['RENDER_ONLY'].split(','):return
 offsets=offsets or {};translations=translations or {}
 for n,o in parts.items():
  o.hide_render=visible is not None and not any(n.startswith(x) for x in visible)
  xy=next((v for pre,v in translations.items() if n.startswith(pre)),(0,0))
  o.location=(*xy,next((v for pre,v in offsets.items() if n.startswith(pre)),0));o.data.materials[0]=active if highlight and n.startswith(highlight) else defaults[n]
 for n,o in decor.items():
  o.hide_render=not labels;o.location.z += offsets.get('H_',0) if n=='screenUI' else offsets.get('B_',0)
  if n=='screenUI':o.location.x+=translations.get('H_',(0,0))[0];o.location.y+=translations.get('H_',(0,0))[1]
 cam.location={'iso':(190,-240,270),'top':(0,0,330),'under':(180,-200,-220),'side':(230,-200,120)}[view]
 cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=scale
 sc.render.filepath=str(R/'renders'/f'{name}.png');bpy.ops.render.render(write_still=True)
 projections[name]={}
 for n,o in parts.items():
  if not o.hide_render:
   world=sum((o.matrix_world @ Vector(p) for p in o.bound_box),Vector())/8
   uv=world_to_camera_view(sc,cam,world);projections[name][n]=[uv.x,uv.y]
 (R/'renders/projections.json').write_text(json.dumps(projections,indent=2))
 for n,o in decor.items():
  o.location.z -= offsets.get('H_',0) if n=='screenUI' else offsets.get('B_',0)
  if n=='screenUI':o.location.x-=translations.get('H_',(0,0))[0];o.location.y-=translations.get('H_',(0,0))[1]
 print('DONE',name,flush=True)
render('01_closed',labels=True)
render('02_exploded',offsets={'E_':32,'F_':45,'G_':64,'I_':32,'J_':48,'R_':32,'S_':32,'H_':88,'K_':88,'B_':115,'L_':115,'M_':115,'N_':115,'O_':115,'P_':115,'C_':155,'W_gold':155,'W_crimson':115,'D_':140,'T_case_screw':115,'U_':32,'V_':155},scale=310,target=(0,0,105),labels=True,translations={'H_':(-145,0),'K_':(-145,0)})
render('03_base',visible=['A_','Q_','T_case_nut','U_tray_nut'],highlight='A_')
render('04_tray',visible=['E_','F_','G_','I_','J_','R_','S_'],offsets={'F_':14,'J_':14,'G_':30,'I_audio_nut':30},scale=140,target=(-4,25,27))
render('05_loaded',visible=['A_','E_','F_','G_','I_','J_','Q_','R_','S_','U_','T_case_nut'],view='top',scale=170)
render('06_lid_under',visible=['B_','H_','K_','L_','M_','N_','P_'],view='under',scale=180,target=(0,0,35))
render('07_window_stack',visible=['B_','C_','D_','H_','K_','V_','W_gold'],offsets={'C_':45,'W_gold':45,'D_':26,'H_':-20,'K_':-20,'V_':45},scale=190,target=(0,15,48))
render('08_top',view='top',scale=166,labels=True)
# Local cutaway of the real faceplate mesh, to reveal the footswitch hardware.
bpy.ops.mesh.primitive_cube_add(size=1,location=(0,-46,56));cutter=bpy.context.object;cutter.name='foot_detail_crop';cutter.dimensions=(66,46,10);cutter.hide_render=True
bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
mod=parts['B_faceplate'].modifiers.new('Cutaway for assembly illustration','BOOLEAN');mod.operation='INTERSECT';mod.object=cutter
render('09_foot_stack',visible=['B_','M_','N_','O_'],offsets={'B_':12,'M_':5,'N_switch_washer':22,'N_switch_nut':29,'O_':35},view='side',scale=108,target=(0,-46,60))
parts['B_faceplate'].modifiers.remove(mod);bpy.data.objects.remove(cutter,do_unlink=True)
for n,o in parts.items():o.hide_render=False;o.location=(0,0,0);o.data.materials[0]=defaults[n]
for o in decor.values():o.hide_render=False
cam.location=(190,-240,270);cam.rotation_euler=(Vector((0,0,25))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=190
bpy.ops.wm.save_as_mainfile(filepath=str(R/'VEX_Pocket_Rival_Render.blend'))
