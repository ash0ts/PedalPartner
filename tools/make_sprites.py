"""Code-defined firmware artwork. PNG previews and C++ arrays share the same pixels."""
from PIL import Image,ImageDraw,ImageFont
from pathlib import Path
import json
R=Path(__file__).resolve().parents[1];(R/'assets').mkdir(exist_ok=True)
palette=[(25,37,39),(35,30,39),(132,97,140),(205,166,84),(155,48,62),(236,226,191),(77,57,81)]
def make(pose):
 im=Image.new('P',(32,32),0);im.putpalette([c for col in palette for c in col]+[0]*(768-len(palette)*3));d=ImageDraw.Draw(im)
 if pose==2:
  d.polygon([(6,20),(8,12),(11,6),(13,14),(19,14),(22,6),(24,13),(26,22),(24,27),(8,27)],fill=1)
  d.polygon([(8,16),(4,21),(5,26),(9,24),(11,27),(13,23)],fill=2);d.polygon([(24,16),(28,21),(27,26),(23,24),(21,27),(19,23)],fill=2)
  d.rectangle((10,14,22,16),fill=3);d.line([(11,19),(13,20),(15,19)],fill=3,width=1);d.line([(18,19),(20,20),(22,19)],fill=3)
  d.polygon([(9,22),(17,24),(24,21),(24,26),(12,28),(8,25)],fill=4)
  d.line([(25,4),(29,4),(25,8),(29,8)],fill=5)
 else:
  d.polygon([(10,14),(6,15),(1,20),(1,24),(5,21),(7,24),(9,21),(11,22)],fill=2)
  d.polygon([(21,14),(25,15),(30,20),(30,24),(26,21),(24,24),(22,21),(20,22)],fill=2)
  d.polygon([(10,15),(7,8),(7,2),(13,8),(18,8),(24,2),(24,9),(21,16),(22,26),(9,26)],fill=1)
  d.polygon([(8,5),(12,9),(10,11)],fill=2);d.polygon([(23,5),(19,9),(21,11)],fill=2)
  d.rectangle((9,10,22,12),fill=3);d.polygon([(13,9),(14,6),(17,6),(19,9)],fill=3)
  d.rectangle((11,14,13,16),fill=3);d.rectangle((18,14,20,16),fill=3)
  d.line([(10,13),(14,14)],fill=1);d.line([(17,14),(21,13)],fill=1)
  d.point((17,18),fill=5);d.line([(15,17),(18,17)],fill=5)
  d.polygon([(9,19),(21,19),(21,21),(15,23),(10,22)],fill=4);d.polygon([(21,19),(27,17),(25,20),(29,22),(22,22)],fill=4)
  d.rectangle((10,26,13,28),fill=3);d.rectangle((18,26,21,28),fill=3)
  if pose==1:
   d.ellipse((11,21,19,27),fill=3);d.line((17,23,27,14),fill=3,width=3);d.line((13,25,27,14),fill=5,width=1);d.rectangle((26,12,29,15),fill=3)
   d.line([(26,7),(26,3),(30,2),(30,6)],fill=5);d.point((25,8),fill=5);d.point((29,7),fill=5)
 d.line((5,30,27,30),fill=6)
 return im
ims=[make(i) for i in range(3)]
for n,im in zip(['idle','riff','sleep'],ims):im.resize((512,512),Image.Resampling.NEAREST).convert('RGB').save(R/'assets'/f'vex_{n}.png')
rgb565=[((r>>3)<<11)|((g>>2)<<5)|(b>>3) for r,g,b in palette]
out='#pragma once\n#include <stdint.h>\n// Original code-defined 32x32 artwork. Edit make_sprites.py then regenerate.\n'
out+='static const uint16_t VEX_PALETTE[]={'+','.join(hex(x) for x in rgb565)+'};\n'
out+='static const uint8_t VEX_SPRITES[3][1024]={\n'
for im in ims:out+='{'+','.join(str(x) for x in im.getdata())+'},\n'
out+='};\n';(R/'firmware/VEX_Pocket_Rival/sprites.h').write_text(out)
# Actual firmware-sized layout for the CAD screen texture. Flat and readable.
im=Image.new('RGB',(240,240),palette[0]);im.paste(ims[0].resize((128,128),Image.Resampling.NEAREST).convert('RGB'),(56,54));d=ImageDraw.Draw(im)
f=str(R/'assets/fonts/DejaVuSansMono.ttf')
def text(s,y,size):
 font=ImageFont.truetype(f,size);bb=d.textbbox((0,0),s,font=font);d.text(((240-(bb[2]-bb[0]))/2,y),s,font=font,fill=palette[5])
text('VEX',24,19);text('LV1  0 XP',177,10);text('0:00 / 5m',191,10);text('PAUSED',205,10)
im.save(R/'assets/screen.png')
(R/'assets/sprites.json').write_text(json.dumps({'palette':palette,'width':32,'height':32,'poses':['idle','riff','sleep'],'pixels':[list(im.getdata()) for im in ims]}))
print('Sprites, firmware arrays and screen texture generated')
