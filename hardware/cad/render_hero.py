"""Render the existing STEP meshes with studio lighting. No generative imagery.

Run via make hero. Intermediate PNG and Blender scene stay under build/hero/.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import runpy
import sys

import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--samples', type=int, default=160)
parser.add_argument('--resolution', type=int, default=1600)
args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else [])
out = ROOT / 'build/hero'
out.mkdir(parents=True, exist_ok=True)
source = json.loads((ROOT / 'hardware/generated/meshes/source.json').read_text())
assert hashlib.sha256((ROOT / source['source']).read_bytes()).hexdigest() == source['source_sha256']
for part in source['parts']:
    path = ROOT / 'hardware/generated/meshes' / (part['name'] + '.stl')
    assert hashlib.sha256(path.read_bytes()).hexdigest() == part['sha256'], path
os.environ['VEX_SCENE_ONLY'] = '1'
scene = runpy.run_path(str(ROOT / 'hardware/cad/render_cad.py'))
sc, cam = scene['sc'], scene['cam']
assert set(scene['parts']) == {p['name'] for p in source['parts']}
sc.cycles.samples = args.samples
sc.cycles.seed = 11
# Portable CPU renders also work in Blender builds without OpenImageDenoise.
sc.cycles.use_denoising = False
sc.cycles.device = 'CPU'
sc.render.threads_mode = 'FIXED'
sc.render.threads = 8
sc.render.resolution_x = sc.render.resolution_y = args.resolution
sc.view_settings.view_transform = 'AgX'
sc.view_settings.look = 'AgX - Medium High Contrast'
sc.render.film_transparent = True
sc.world.use_nodes = True
sc.world.node_tree.nodes['Background'].inputs[0].default_value = (.20, .23, .28, 1)
sc.world.node_tree.nodes['Background'].inputs[1].default_value = .38

# Finish choices change shading only, never the CAD mesh or part transforms.
for name in ('Warm charcoal', 'Charcoal with 3mm crimson base'):
    mat = bpy.data.materials.get(name)
    shader = mat.node_tree.nodes.get('Principled BSDF')
    shader.inputs['Roughness'].default_value = .34
    shader.inputs['Metallic'].default_value = .08
    shader.inputs['Base Color'].default_value = (.028, .030, .038, 1)
    for node in mat.node_tree.nodes:
        if node.type == 'MIX_RGB':
            node.inputs[1].default_value = (.028, .030, .038, 1)
            node.inputs[2].default_value = (.24, .016, .028, 1)
    noise = mat.node_tree.nodes.new('ShaderNodeTexNoise')
    noise.inputs['Scale'].default_value = 550
    bump = mat.node_tree.nodes.new('ShaderNodeBump')
    bump.inputs['Strength'].default_value = .13
    bump.inputs['Distance'].default_value = .025
    mat.node_tree.links.new(noise.outputs['Fac'], bump.inputs['Height'])
    mat.node_tree.links.new(bump.outputs['Normal'], shader.inputs['Normal'])
for name in ('Ochre bezel', 'Steel hardware'):
    shader = bpy.data.materials[name].node_tree.nodes.get('Principled BSDF')
    shader.inputs['Metallic'].default_value = .72
    shader.inputs['Roughness'].default_value = .25
bpy.data.materials['Crimson'].node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value = .32
bpy.data.materials['Crimson'].node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value = (.24, .016, .028, 1)
bpy.data.materials['Clear window'].node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value = .015
for obj in list(bpy.data.objects):
    if obj.type == 'LIGHT':
        bpy.data.objects.remove(obj, do_unlink=True)
for name, position, power, size, color in [
    ('Key softbox', (70, -110, 250), 500000, 170, (1, .89, .76)),
    ('Cool fill', (-160, -40, 150), 350000, 140, (.67, .79, 1)),
    ('Warm rear edge', (40, 200, 210), 600000, 120, (1, .69, .40)),
]:
    bpy.ops.object.light_add(type='AREA', location=position)
    lamp = bpy.context.object
    lamp.name = name
    lamp.data.energy, lamp.data.size, lamp.data.color = power, size, color
    lamp.rotation_euler = (Vector((0, 0, 25)) - lamp.location).to_track_quat('-Z', 'Y').to_euler()
cam.location = (130, -230, 300)
cam.rotation_euler = (Vector((0, 0, 23)) - cam.location).to_track_quat('-Z', 'Y').to_euler()
cam.data.ortho_scale = 196
sc.render.filepath = str(out / 'product.png')
bpy.ops.render.render(write_still=True)
bpy.ops.wm.save_as_mainfile(filepath=str(out / 'studio.blend'))
record = {
    'renderer': 'Blender ' + bpy.app.version_string + ' / Cycles CPU',
    'samples': args.samples, 'resolution': args.resolution, 'seed': 11,
    'assembly': source['source'], 'assembly_sha256': source['source_sha256'],
    'components': len(source['parts']),
    'mesh_manifest_sha256': hashlib.sha256((ROOT / 'hardware/generated/meshes/source.json').read_bytes()).hexdigest(),
    'screen': 'assets/screen.png',
    'screen_sha256': hashlib.sha256((ROOT / 'assets/screen.png').read_bytes()).hexdigest(),
    'scope': 'Existing STEP geometry; studio materials/lighting, existing decal labels and shared UI texture. Proposed finish, not a photograph or physical validation.',
    'generative_imagery': False,
}
(out / 'render.json').write_text(json.dumps(record, indent=2) + '\n')
print('PASS: CAD studio render and provenance saved.')
