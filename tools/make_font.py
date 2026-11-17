"""Generate the common desktop/device 5x7 glyphs from the bundled font."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[1]
font=ImageFont.truetype(str(ROOT/'assets/fonts/DejaVuSansMono.ttf'),9)
glyphs=[]
for code in range(32,127):
    im=Image.new('L',(5,7));ImageDraw.Draw(im).text((0,-2),chr(code),font=font,fill=255)
    glyphs.append([sum((im.getpixel((x,y))>=90)<<y for y in range(7)) for x in range(5)])
out='#pragma once\n// Rasterized from bundled DejaVu Sans Mono; see assets/fonts/LICENSE.txt.\nstatic const unsigned char VEX_FONT[95][5]={\n'
out+=',\n'.join('{'+','.join(map(str,g))+'}' for g in glyphs)+'\n};\n'
(ROOT/'firmware/VEX_Pocket_Rival/tinyfont.h').write_text(out)
