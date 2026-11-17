"""Code-defined firmware artwork. PNG previews and C++ arrays share the same pixels.

Poses: idle, riff, sleep (original three), then blink, riff2 (second strum frame), cheer (fragment),
listen (tuner, ear up), levelup (arms raised). The order is the VEX_SPRITES index used by ui.h.
"""
from PIL import Image,ImageDraw,ImageFont
from pathlib import Path
import json
R=Path(__file__).resolve().parents[1];(R/'assets').mkdir(exist_ok=True)
palette=[(25,37,39),(35,30,39),(132,97,140),(205,166,84),(155,48,62),(236,226,191),(77,57,81)]
BG,BODY,WING,GOLD,RED,CREAM,SHADOW=range(7)
POSES=['idle','riff','sleep','blink','riff2','cheer','listen','levelup']

def canvas():
 im=Image.new('P',(32,32),0);im.putpalette([c for col in palette for c in col]+[0]*(768-len(palette)*3));return im,ImageDraw.Draw(im)

def wings(d,raised=False):
 if not raised:
  d.polygon([(10,14),(6,15),(1,20),(1,24),(5,21),(7,24),(9,21),(11,22)],fill=WING)
  d.polygon([(21,14),(25,15),(30,20),(30,24),(26,21),(24,24),(22,21),(20,22)],fill=WING)
  return
 # Raised wings for cheer/levelup: narrow membranes swept up beside the ears, tips at row 9.
 d.polygon([(10,17),(7,16),(4,13),(2,9),(4,11),(5,8),(6,12),(8,10),(8,14),(11,19)],fill=WING)
 d.polygon([(21,17),(24,16),(27,13),(29,9),(27,11),(26,8),(25,12),(23,10),(23,14),(20,19)],fill=WING)

def body(d,ear_left=(7,8,7,2),ear_right=(24,2,24,9)):
 lx,ly,lx2,ly2=ear_left;rx,ry,rx2,ry2=ear_right
 d.polygon([(10,15),(lx,ly),(lx2,ly2),(13,8),(18,8),(rx,ry),(rx2,ry2),(21,16),(22,26),(9,26)],fill=BODY)
 d.polygon([(8,5),(12,9),(10,11)],fill=WING);d.polygon([(23,5),(19,9),(21,11)],fill=WING)
 d.rectangle((9,10,22,12),fill=GOLD);d.polygon([(13,9),(14,6),(17,6),(19,9)],fill=GOLD)

def eyes(d,kind='open'):
 if kind=='open':d.rectangle((11,14,13,16),fill=GOLD);d.rectangle((18,14,20,16),fill=GOLD)
 elif kind=='closed':d.line([(11,15),(13,15)],fill=GOLD);d.line([(18,15),(20,15)],fill=GOLD)
 elif kind=='happy':d.line([(11,15),(12,14),(13,15)],fill=GOLD);d.line([(18,15),(19,14),(20,15)],fill=GOLD)
 elif kind=='focus':d.rectangle((11,15,13,16),fill=GOLD);d.rectangle((18,15,20,16),fill=GOLD)
 d.line([(10,13),(14,14)],fill=BODY);d.line([(17,14),(21,13)],fill=BODY)

def mouth(d,kind='smirk'):
 if kind=='smirk':d.point((17,18),fill=CREAM);d.line([(15,17),(18,17)],fill=CREAM)
 elif kind=='grin':d.line([(13,17),(18,17)],fill=CREAM);d.point((13,18),fill=CREAM);d.point((18,18),fill=CREAM)
 elif kind=='o':d.rectangle((15,17,16,18),fill=CREAM)

def scarf(d):
 d.polygon([(9,19),(21,19),(21,21),(15,23),(10,22)],fill=RED);d.polygon([(21,19),(27,17),(25,20),(29,22),(22,22)],fill=RED)

def feet(d):d.rectangle((10,26,13,28),fill=GOLD);d.rectangle((18,26,21,28),fill=GOLD)
def floor(d):d.line((5,30,27,30),fill=SHADOW)

def guitar(d,strum=0):
 d.ellipse((11,21,19,27),fill=GOLD);d.line((17,23,27,14),fill=GOLD,width=3);d.line((13,25,27,14),fill=CREAM,width=1);d.rectangle((26,12,29,15),fill=GOLD)
 if strum==0:d.line([(26,7),(26,3),(30,2),(30,6)],fill=CREAM);d.point((25,8),fill=CREAM);d.point((29,7),fill=CREAM)
 else:d.line([(28,9),(28,5),(31,4),(31,8)],fill=CREAM);d.point((27,10),fill=CREAM);d.point((24,6),fill=CREAM);d.point((26,4),fill=CREAM)

def make(pose):
 im,d=canvas()
 if pose=='sleep':
  d.polygon([(6,20),(8,12),(11,6),(13,14),(19,14),(22,6),(24,13),(26,22),(24,27),(8,27)],fill=BODY)
  d.polygon([(8,16),(4,21),(5,26),(9,24),(11,27),(13,23)],fill=WING);d.polygon([(24,16),(28,21),(27,26),(23,24),(21,27),(19,23)],fill=WING)
  d.rectangle((10,14,22,16),fill=GOLD);d.line([(11,19),(13,20),(15,19)],fill=GOLD,width=1);d.line([(18,19),(20,20),(22,19)],fill=GOLD)
  d.polygon([(9,22),(17,24),(24,21),(24,26),(12,28),(8,25)],fill=RED)
  d.line([(25,4),(29,4),(25,8),(29,8)],fill=CREAM)
 elif pose in('idle','blink','riff','riff2'):
  wings(d);body(d);eyes(d,'closed' if pose=='blink' else 'open');mouth(d);scarf(d);feet(d)
  if pose=='riff':guitar(d,0)
  if pose=='riff2':guitar(d,1)
 elif pose=='cheer':
  wings(d,raised=True);body(d);eyes(d,'happy');mouth(d,'grin');scarf(d);feet(d)
  d.point((4,3),fill=GOLD);d.point((27,3),fill=GOLD);d.point((1,20),fill=CREAM);d.point((30,20),fill=CREAM);d.point((7,5),fill=CREAM);d.point((24,5),fill=CREAM)
 elif pose=='listen':
  wings(d);body(d,ear_right=(25,0,25,9));eyes(d,'focus');mouth(d,'o');scarf(d);feet(d)
  d.line([(28,5),(30,5)],fill=CREAM);d.line([(29,3),(31,3)],fill=CREAM);d.point((28,7),fill=CREAM)
 elif pose=='levelup':
  wings(d,raised=True);body(d);eyes(d,'happy');mouth(d,'grin');scarf(d);feet(d)
  d.polygon([(15,0),(16,2),(18,2),(16,3),(17,5),(15,4),(13,5),(14,3),(12,2),(14,2)],fill=GOLD)
  d.point((4,3),fill=CREAM);d.point((27,3),fill=CREAM);d.point((1,20),fill=GOLD);d.point((30,20),fill=GOLD)
 floor(d)
 return im

ims=[make(p) for p in POSES]
for n,im in zip(POSES,ims):im.resize((512,512),Image.Resampling.NEAREST).convert('RGB').save(R/'assets'/f'vex_{n}.png')
sheet=Image.new('RGB',(32*len(POSES)*4,32*4),palette[0])
for i,im in enumerate(ims):sheet.paste(im.resize((128,128),Image.Resampling.NEAREST).convert('RGB'),(i*128,0))
sheet.save(R/'assets/vex_sheet.png')
rgb565=[((r>>3)<<11)|((g>>2)<<5)|(b>>3) for r,g,b in palette]
out='#pragma once\n#include <stdint.h>\n// Original code-defined 32x32 artwork. Edit make_sprites.py then regenerate.\n'
out+='// Poses: '+', '.join(f'{i}={p}' for i,p in enumerate(POSES))+'\n'
out+='enum VexPose{'+','.join('POSE_'+p.upper() for p in POSES)+',VEX_POSE_COUNT};\n'
out+='static const uint16_t VEX_PALETTE[]={'+','.join(hex(x) for x in rgb565)+'};\n'
out+=f'static const uint8_t VEX_SPRITES[{len(POSES)}][1024]={{\n'
for im in ims:out+='{'+','.join(str(x) for x in im.getdata())+'},\n'
out+='};\n';(R/'firmware/VEX_Pocket_Rival/sprites.h').write_text(out)
(R/'assets/sprites.json').write_text(json.dumps({'palette':palette,'width':32,'height':32,'poses':POSES,'pixels':[list(im.getdata()) for im in ims]}))
print('Sprites and firmware arrays generated; run tools/render_screen.py for the CAD texture')
