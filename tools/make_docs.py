"""Generate mentor workbook, 1:1 templates, CSVs and editable Markdown.
All assembly pictures come from build_cad.py -> meshes -> render_cad.py.
"""
from pathlib import Path
import csv,json,math,os,re
from reportlab.pdfgen import canvas
from reportlab.lib.colors import HexColor,Color,white
from reportlab.lib.utils import ImageReader
from reportlab.platypus import Paragraph,Table,TableStyle
from reportlab.lib.styles import ParagraphStyle
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'hardware/data'))
from project_data import *
R=Path(__file__).resolve().parents[1]
G=R/'hardware/generated'
D=R/'hardware/data'
(R/'docs').mkdir(exist_ok=True)
pdfmetrics.registerFont(TTFont('VexSans',str(R/'assets/fonts/DejaVuSans.ttf')))
pdfmetrics.registerFont(TTFont('VexBold',str(R/'assets/fonts/DejaVuSans-Bold.ttf')))
pdfmetrics.registerFont(TTFont('VexMono',str(R/'assets/fonts/DejaVuSansMono.ttf')))
W,H=841.89,595.28
BG=HexColor('#F5F0E7');INK=HexColor('#262832');MUTED=HexColor('#65696B');RED=HexColor('#98394A');GOLD=HexColor('#B58035');LINE=HexColor('#D5CFC4');PALE=HexColor('#E8E2D8')
PSTYLE=ParagraphStyle('body',fontName='VexSans',fontSize=10.6,leading=14.8,textColor=INK,spaceAfter=5)
SMALL=ParagraphStyle('small',parent=PSTYLE,fontSize=9,leading=12)
def clean(s):
 s=re.sub(r'([a-z]{2,})(\d)',r'\1 \2',str(s))
 s=re.sub(r'(\d)(mm|min|seconds|minutes|pins|wires|template|V\b|Hz\b)',r'\1 \2',s)
 return s
def para(c,text,x,y,w=300,size=None):
 style=PSTYLE if size is None else ParagraphStyle('x',parent=PSTYLE,fontSize=size,leading=size*1.4)
 p=Paragraph(clean(text),style);_,h=p.wrap(w,1000);p.drawOn(c,x,y-h);return y-h
def text(c,s,x,y,size=10,color=INK,font='VexSans'):
 c.setFillColor(color);c.setFont(font,size);c.drawString(x,y,clean(s))
def center(c,s,x,y,size=10,color=INK,font='VexSans'):
 c.setFillColor(color);c.setFont(font,size);c.drawCentredString(x,y,s)
def imagebox(c,path,x,y,w,h):
 from PIL import Image
 im=Image.open(path);iw,ih=im.size;scale=min(w/iw,h/ih);dw,dh=iw*scale,ih*scale
 c.drawImage(ImageReader(im),x+(w-dw)/2,y+(h-dh)/2,dw,dh,mask='auto')
def header(c,n,title,kicker):
 c.setFillColor(BG);c.rect(0,0,W,H,fill=1,stroke=0)
 text(c,'VEX / POCKET RIVAL',36,H-32,10,RED,'VexBold');text(c,'MENTOR BUILD EDITION  R1.1',W-238,H-32,9,MUTED)
 text(c,title,36,H-70,23,INK,'VexBold');text(c,kicker,36,H-91,10,MUTED)
 c.setStrokeColor(LINE);c.line(36,49,W-36,49)
 text(c,'GEOMETRIC PROTOTYPE  |  16 NOV 2026  |  mm unless stated',36,30,8,MUTED)
 text(c,f'{n:02}',W-59,29,11,RED,'VexBold')
def checkpoint(c,s):
 c.setFillColor(PALE);c.roundRect(36,62,W-72,38,6,fill=1,stroke=0)
 text(c,'CHECK',48,77,9,RED,'VexBold');para(c,s,102,89,W-156,9.5)
def sections(c,items,x=36,y=480,w=298):
 for title,body in items:
  text(c,title,x,y,11,RED,'VexBold');y-=10;y=para(c,body,x,y,w)-13
 if y<108:raise RuntimeError(f'Page text overflow: {y}, {items[0][0]}')
def box(c,label,x,y,w=120,h=42,color=PALE):
 c.setFillColor(color);c.setStrokeColor(LINE);c.roundRect(x,y,w,h,5,fill=1,stroke=1)
 pp=Paragraph(label,ParagraphStyle('node',parent=SMALL,alignment=1));_,ph=pp.wrap(w-12,h);pp.drawOn(c,x+6,y+(h-ph)/2)
def arrow(c,x1,y1,x2,y2,color=RED):
 c.setStrokeColor(color);c.setFillColor(color);c.setLineWidth(1.4);c.line(x1,y1,x2,y2)
 a=math.atan2(y2-y1,x2-x1);p=c.beginPath();p.moveTo(x2,y2);p.lineTo(x2-7*math.cos(a-.4),y2-7*math.sin(a-.4));p.lineTo(x2-7*math.cos(a+.4),y2-7*math.sin(a+.4));p.close();c.drawPath(p,fill=1,stroke=0)
def figure(c,name):imagebox(c,G/'renders'/f'{name}.png',345,105,461,389)
def table(c,rows,widths,x=36,y=480,fs=9.3):
 st=ParagraphStyle('cell',parent=PSTYLE,fontSize=fs,leading=fs*1.35)
 cells=[[Paragraph(clean(v),st) for v in row] for row in rows]
 t=Table(cells,colWidths=widths,hAlign='LEFT');t.setStyle(TableStyle([('BACKGROUND',(0,0),(-1,0),PALE),('VALIGN',(0,0),(-1,-1),'TOP'),('LEFTPADDING',(0,0),(-1,-1),8),('RIGHTPADDING',(0,0),(-1,-1),8),('TOPPADDING',(0,0),(-1,-1),7),('BOTTOMPADDING',(0,0),(-1,-1),7),('LINEBELOW',(0,0),(-1,-1),.4,LINE)]));_,h=t.wrap(W,1000);t.drawOn(c,x,y-h)
 if y-h<105:raise RuntimeError(f'Table overflow: {y-h}')
 return y-h
def routing(c):
 x=361;y=404
 box(c,'GUITAR',x,y,95);box(c,'G3S / dry split',x+130,y,145);box(c,'Existing rig',x+310,y,125);arrow(c,x+95,y+21,x+130,y+21);arrow(c,x+275,y+21,x+310,y+21)
 arrow(c,x+202,y,x+202,y-49);box(c,'BUFFERED DRY FEED<br/>test every preset',x+130,y-100,145,50)
 arrow(c,x+202,y-100,x+202,y-132);box(c,'J1 / input attenuator',x+130,y-174,145)
 arrow(c,x+202,y-174,x+202,y-204);box(c,'Audio Adapter<br/>LINE IN L',x+130,y-254,145,50)
 arrow(c,x+130,y-229,x+101,y-229);box(c,'Teensy 4.1',x,y-250,101)
 arrow(c,x+50,y-208,x+50,y-170);box(c,'Round display<br/>and controls',x,y-170,101,50)
 text(c,'USB 5V -> Teensy only',x+5,122,10,RED,'VexBold')
def circuit(c):
 x=369; y=390
 text(c,'INPUT ATTENUATOR / ELECTRICAL SCHEMATIC',x,468,10,RED,'VexBold')
 text(c,'J1 TIP',x,y+15,10);c.setStrokeColor(INK);c.setLineWidth(1.5);c.line(x,y,x+70,y);c.rect(x+70,y-9,58,18);c.line(x+128,y,x+180,y)
 text(c,'R1 10k',x+72,y+19,9);c.line(x+180,y,x+180,y-42);c.rect(x+171,y-105,18,63);text(c,'RV1',x+202,y-58,10);text(c,'10k',x+202,y-75,10)
 c.line(x+180,y-105,x+180,y-151);c.line(x,y-151,x+180,y-151);text(c,'J1 SLEEVE / GND',x,y-170,10)
 for dy,ww in [(0,20),(-5,13),(-10,6)]:c.line(x+180-ww/2,y-151+dy,x+180+ww/2,y-151+dy)
 arrow(c,x+254,y-83,x+188,y-83,INK);c.line(x+254,y-83,x+277,y-83);c.rect(x+277,y-92,45,18);c.line(x+322,y-83,x+414,y-83)
 text(c,'R2 1k',x+277,y-63,9);text(c,'LINE IN L',x+344,y-64,9);text(c,'wiper / 2',x+208,y-105,9)
 text(c,'3',x+159,y-36,9);text(c,'1',x+159,y-123,9)
 para(c,'Terminal numbers follow Bourns 3386P. Verify with the meter; the physical pin layout is not drawn here.',x,203,420,10)
 box(c,'MAXIMUM: about 0.5 x input before codec loading<br/>MINIMUM: near zero at the wiper',x,118,420,48)
def window_section(c):
 x=380;y=420;ww=385
 layers=[('C  bezel / underside pocket',4,INK),('D  clear disc, 1.0mm',1,HexColor('#BAD3D4')),('B  faceplate',3,INK),('Air gap to screen',.245,white),('H  display screen front',1.73,HexColor('#486B60')),('Display PCB',1.57,HexColor('#36584B'))]
 for label,th,col in layers:
  h=22 if th<1 else th*11;c.setFillColor(col);c.rect(x,y-h,ww*.47,h,fill=1,stroke=0);text(c,label,x+195,y-h/2-3,9);y-=h+12
 para(c,'SECTION SCHEMATIC, vertical spacing exaggerated. Actual CAD: 3.395mm from display to cover; 2.0mm mounting spacers. The cover rests in the bezel pocket, not on the display.',x,184,400,10)
def controls_diagram(c):
 x=378;y=410
 for i,(label,pin) in enumerate([('MODE',0),('QUEST',1),('FOOT',2)]):
  yy=y-i*90;box(c,f'Teensy {pin}<br/>INPUT_PULLUP',x,yy,125);c.setStrokeColor(INK);c.line(x+125,yy+21,x+180,yy+21);c.circle(x+180,yy+21,2);c.circle(x+218,yy+21,2);c.line(x+182,yy+22,x+216,yy+35);c.line(x+220,yy+21,x+280,yy+21);text(c,'GND',x+286,yy+18,11);text(c,label,x+177,yy-14,10,RED,'VexBold')
 para(c,'Each switch only connects a GPIO to GND. No guitar audio or 9V reaches these controls. Released = HIGH. Pressed = LOW.',x,135,400,10)
def cad_top(c):
 scale=2.25;ox=574;oy=291
 c.setFillColor(PALE);c.setStrokeColor(INK);c.roundRect(ox-43*scale,oy-73*scale,86*scale,146*scale,12*scale,fill=1,stroke=1)
 c.roundRect(ox-31*scale,oy+1*scale,62*scale,62*scale,6*scale);c.circle(ox,oy+32*scale,17.3*scale)
 for x,y,r in [(-17,-12,3.6),(17,-12,3.6),(0,-46,6.2),(0,-27,1.6)]:c.circle(ox+x*scale,oy+y*scale,r*scale)
 for x in [-34,34]:
  for y in [-64,64]:c.circle(ox+x*scale,oy+y*scale,1.7*scale)
 arrow(c,ox,oy,ox+44,oy,RED);arrow(c,ox,oy,ox,oy+44,RED);text(c,'+X',ox+47,oy-4,9);text(c,'+Y rear',ox-24,oy+48,9)
 text(c,'86 mm',ox-18,oy-73*scale-20,10);text(c,'146 mm',ox+43*scale+8,oy,10)
 text(c,'Front / foot',ox-30,oy-73*scale-39,9,MUTED)
def sprite_frame(name,index):
 """One 32x32 frame from an art/vex strip, upscaled without smoothing on the screen background."""
 from PIL import Image
 strip=Image.open(R/'art/vex'/f'{name}.png').convert('RGBA');frame=strip.crop((index*32,0,index*32+32,32))
 im=Image.new('RGBA',(32,32),(24,36,40,255));im.alpha_composite(frame);return im.resize((128,128),Image.NEAREST)
def ui(c):
 for i,(n,idx,label) in enumerate([('idle',0,'IDLE'),('strum',1,'STRUM'),('sleep',0,'SLEEP')]):
  im=sprite_frame(n,idx);iw,ih=im.size;c.drawImage(ImageReader(im),355+i*145+6,256,iw,ih,mask='auto');center(c,label,425+i*145,225,10,RED,'VexBold')
 box(c,'Participation XP = active time<br/>Quest reward = one fragment per completed session',374,143,405,55)

def exploded_key(c):
 from PIL import Image
 path=G/'renders/02_exploded.png';x,y,w,h=200,105,440,389
 im=Image.open(path);scale=min(w/im.width,h/im.height);dw,dh=im.width*scale,im.height*scale;ix=x+(w-dw)/2;iy=y+(h-dh)/2
 imagebox(c,path,x,y,w,h)
 proj=json.loads((G/'renders/projections.json').read_text())['02_exploded']
 labels=[('A_base','A  Printed base',36,139,'left'),('E_electronics_tray','E  Removable tray',36,215,'left'),('F_teensy41','F  Teensy 4.1',36,293,'left'),('H_round_display_6178','H  Round display',36,387,'left'),('C_gold_bezel','C  Charcoal bezel',660,462,'right'),('D_clear_window','D  Clear window',660,417,'right'),('B_faceplate','B  Faceplate',660,350,'right'),('G_audio_adapter','G  Audio Adapter',660,272,'right')]
 for key,label,lx,ly,side in labels:
  u,v=proj[key];px,py=ix+u*dw,iy+v*dh
  text(c,label,lx,ly,11,RED,'VexBold');start=lx+146 if side=='left' else lx-10
  c.setStrokeColor(MUTED);c.setLineWidth(.7);c.line(start,ly+3,px,py);c.setFillColor(RED);c.circle(px,py,2,fill=1,stroke=0)
 text(c,'Display shifted left for visibility; controls stay grouped with their mounting parts.',36,482,9,MUTED)

PAGES=[]
def add(title,kicker,items=None,fig=None,check='',diagram=None,rows=None,widths=None):PAGES.append(dict(title=title,kicker=kicker,items=items,fig=fig,check=check,diagram=diagram,rows=rows,widths=widths))
add('Build a pocket rival','A practical learning project for Anish + an electronics / fabrication mentor',[
 ('THE OBJECT','A black, crimson and ochre practice companion with a round pixel pet, activity XP and an experimental tuner.'),
 ('THE FIRST BUILD','Teensy 4.1 + Audio Adapter + Adafruit 6178. USB power, one buffered audio input, two finger buttons and one momentary footswitch.'),
 ('THE STATUS','Real CAD and original firmware source are included. Geometry and desktop logic checks are separate from the physical tests you will do with your mentor.')],fig='01_closed',check='Start with pages 2-3. Buy the bench electronics first; print the final enclosure after measurement and screen tests.')
add('The first mentor session','Suggested 2-hour session; bring this pack and the actual parts',[
 ('00-20 MIN / REVIEW','Confirm the USB-only sensing architecture, display choice and source of buffered dry guitar. Review the input attenuator before connecting the rig.'),
 ('20-50 MIN / LEARN','Practise one solder joint. Identify switch lugs and trimmer wiper with a meter. Mark every power and signal wire before soldering.'),
 ('50-100 MIN / MAKE','Upload Stage 1. Add the display and buttons, then upload Stage 2. Get VEX and the control readings working on the desk.'),
 ('100-120 MIN / DECIDE','Measure screen readability, the purchased boards and connector bodies. Record changes in the measurement sheet and assign the next session.')],fig='08_top',check='First-session success = a working screen and controls, plus a measured list of remaining fit questions.')
add('Build in three milestones','Each milestone produces something usable and teaches one new layer',[
 ('1 / PET ON THE DESK','USB + screen + buttons. Learn power, ground, SPI, upload and button debouncing. No guitar connection is needed yet.'),
 ('2 / LISTEN TO GUITAR','Add the audio board and attenuator. Learn signal level, RMS, clipping and calibration. Keep the existing audible guitar path.'),
 ('3 / ENCLOSE + USE','Fit the real parts, assemble the case, route wires and verify readability and strength. Then test the companion alongside your rig.')],diagram='routing',check='R1 does not provide audio bypass, silent muting, 9V pedal power, MIDI control, daily streaks or technique assessment.')
add('The important design choices','Read this before ordering or printing',[
 ('REAL SCREEN SIZE','Adafruit 6178 has a 1.28-inch round screen: approximately 32.4mm visible diameter. It is smaller than the concept art, chosen for a documented SPI driver.'),
 ('REAL CASE SIZE','The body is 86 x146 x45mm; controls extend higher. This is not a drop-in replacement for the small Lock In tuner. Use a paper footprint on the SA550XDM first.'),
 ('PLAIN OPTICS + POWER','R1 uses a flat clear disc and direct USB power. A dome and protected pedalboard 9V conversion are later engineering changes.'),
 ('FOOT LOAD','An aluminum plate sits on four columns below the switch. The printed prototype starts with hand operation; the mentor verifies foot use before electronics are exposed to load.')],fig='01_closed',check='If the 32mm display fails the standing test, choose a larger screen before committing to this enclosure.')
add('Buy: the bench electronics','Exact identities matter; budget columns are allowances, not current quotations',rows=[['ID / qty','Part to order','Why / source','Allowance']] + [[f'{a} / {b}',d,e+' ['+f+']',f'${g:g}-{h:g}'] for a,b,c,d,e,f,g,h,stage in BOM[:7]],widths=[62,274,319,115],check='Do not substitute the old ST7789 square display or a Rev C audio board into these pin tables.')
add('Buy: wiring and small parts','Complete the bench circuits before ordering the enclosure',rows=[['ID / qty','Part / specification','Purpose','Allowance']]+[[f'{a} / {b}',d,e,f'${g:g}-{h:g}'] for a,b,c,d,e,f,g,h,stage in BOM[7:15]],widths=[62,300,293,115],check='Budget excludes mentor time, tools, a new buffered splitter, shipping and tax. BOM.csv contains all links and order stages.')
add('Buy: mechanical parts','Order after the measured-fit gate',rows=[['ID / qty','Part / specification','Purpose','Allowance']]+[[f'{a} / {b}',d,e,f'${g:g}-{h:g}'] for a,b,c,d,e,f,g,h,stage in BOM[15:]],widths=[62,300,293,115],check='Use hardware/generated/drawings/ for cut profiles and the fastener schedule on the next page. Test a coupon before any full enclosure print.')
add('Fasteners and tools','Use the included nuts and washers for the purchased controls',rows=[['Location','Qty','Screw / part','Mating hardware']]+[[a,b,c,d] for a,b,c,d in FASTENERS],widths=[150,40,195,385],check='Tools: temperature-controlled iron, solder/flux, fume extraction, meter, calipers, cutters, wire stripper, drivers and small files. Mentor handles drilling/cutting.')
add('Where guitar audio goes','The companion listens on a separate branch',[
 ('1 / FIND A DRY FEED','Test G3S TUNER/AUX availability in every normal preset. If it is switched off during playing, use a dedicated buffered splitter output.'),
 ('2 / KEEP THE MAIN ROUTE','Guitar -> G3S -> the existing OPUS / Orange routing. The companion has no audio output and does not replace that route.'),
 ('3 / CONNECT THE LISTENER','Buffered dry output -> J1 -> attenuator -> Audio Adapter LINE IN L. No raw passive-pickup input, amplifier speaker output or phantom-powered input.'),
 ('WHY','A clean branch avoids scoring reverb repeats or synthesized notes. It also keeps experimental firmware out of your audible guitar path.')],diagram='routing',check='With the companion disconnected, the original rig should work exactly as before. Log what G3S mute does to the dry feed.')
add('Wire the display','Use the board silkscreen names, not cable colors or a guessed pin order',rows=[['Wire','From','To','Detail']]+[[a,b,c,e] for a,b,c,d,e in WIRING[:10]],widths=[50,155,225,340],check='Teensy logic is 3.3V. Display VIN is an input; display 3Vo is an output and is left disconnected.')
add('Wire the controls','Test every switch before connecting it to a GPIO',[
 ('1 / IDENTIFY CONTACTS','Remove power. In continuity mode, the selected pair must be open when released and close only while pressed. Use SPST normally-open momentary switches.'),
 ('2 / CONNECT','MODE -> pin0; QUEST -> pin1; FOOT -> pin2. The other lug of each switch goes to GND. Insulate all solder lugs.'),
 ('3 / ADD THE LED','Pin9 -> 1k resistor R3 -> LED anode. Cathode -> GND. Identify polarity before trimming the leads.'),
 ('WHY','INPUT_PULLUP holds the released pin HIGH. A press connects it to ground, so firmware reads LOW.')],diagram='controls',check='Unpowered: the switch wire is not shorted to any power rail. Stage 2 must show exactly one changing input per press.')
add('Build the audio attenuator','This small circuit needs mentor review before connection to the rig',[
 ('1 / IDENTIFY THE TRIMMER','Bourns 3386P-1-103LF is 10k. Meter terminals1-3: approximately10k. Terminal2 is the wiper; its resistance to either end changes as you turn.'),
 ('2 / WIRE THE DIVIDER','TIP -> R1 10k -> terminal3. Terminal1 -> ground. Wiper2 -> R2 1k -> LINE IN L. Jack sleeve -> LINE IN ground.'),
 ('3 / BEGIN AT MINIMUM','Set the wiper near terminal1 using the meter. Check continuity and shorts, then begin Stage3 with a quiet known line-level source.'),
 ('WHY','The divider lowers the signal before conversion. Its maximum unloaded output is half the input; codec loading reduces it further. It is not a general overvoltage protector.')],diagram='circuit',check='The audio board has its own input coupling. Use LINE IN L/GND labels; neither MIC nor headphone ground is a substitute.')
add('Assemble and check the audio stack','Short aligned headers carry the fast audio clocks',[
 ('1 / ORIENT','Use the PJRC Rev D/D2 pin diagram. Align the shared 14-pin rows with the matching printed Teensy pin numbers; do not align by the board edges alone.'),
 ('2 / SOLDER + INSPECT','Socket the stack for service. Keep the audio clocks on short headers. Check every row for bridges with magnification and continuity.'),
 ('3 / CHECK POWER','USB unplugged: inspect the 3.3V/GND pair for a sustained short. USB on: black probe to GND, red to 3.3V; target3.20-3.40V. Stop if a board heats abnormally.'),
 ('RESERVED PINS','Audio7/8/20/21/23; I2C18/19. Pins6 and10 are held HIGH to deselect optional audio-board memory and SD. No SD cards in R1.')],fig='04_tray',check='Verify display + audio-board current against the Teensy regulator budget. Record the measured header gap; CAD currently reserves9.4mm.')
add('Upload 1: prove USB on your Mac','One sketch, four stages: change VEX_STAGE in config.h',[
 ('1 / INSTALL','Install Arduino IDE2 from arduino.cc. In Arduino IDE > Settings, add the PJRC boards URL listed in docs/LOCAL_DEVELOPMENT.md; install Teensy using Boards Manager.'),
 ('2 / LIBRARIES','Install Adafruit GC9A01A, Adafruit GFX and Adafruit BusIO using Library Manager. Audio, Wire, SPI and EEPROM come with Teensy support.'),
 ('3 / OPEN + SELECT','Open firmware/VEX_Pocket_Rival/VEX_Pocket_Rival.ino. Choose Teensy4.1, USB Type Serial, normal600MHz. Leave VEX_STAGE1 for the first upload.'),
 ('4 / VERIFY + UPLOAD','Click Verify, then Upload. Select the Teensy port. If prompted, press the physical Program button. Open Serial Monitor at115200.')],fig='04_tray',check='Onboard LED blinks and Serial says VEX USB OK. No .hex is supplied: the complete target build still needs this verification.')
add('Upload 2: screen and buttons','Disconnect USB before adding each wiring group',[
 ('1 / CONNECT','Add display wires W01-W10 and controls W11-W14. Keep SPI wires short, initially under100mm. Leave audio input disconnected.'),
 ('2 / UPLOAD','Set VEX_STAGE2, Verify and Upload. The display should show the bat and live M/Q/F button states.'),
 ('3 / EXERCISE','Press each control ten times. One state changes from0 to1 while held, and returns to0 on release. FOOT also lights the status LED.'),
 ('IF IT FAILS','White screen: check VIN/GND, CS/DC/reset and SPI pins. Stuck input: inspect the chosen lug pair. Rotated display: change setRotation, not wiring.')],diagram='ui',check='Put the display at the real pedalboard distance. Confirm legibility before fabricating the enclosure.')
add('Upload 3: audio and level','Begin with a known low-level line source; then test the buffered guitar feed',[
 ('1 / START SMALL','Set VEX_STAGE3. Codec input level stays0, the least sensitive setting (3.12Vpp full scale). Trimmer begins at minimum.'),
 ('2 / OBSERVE','Serial must report codec=1. Slowly raise the trimmer while playing. Aim for peaks roughly0.10-0.80 during strong playing; 0.98 means clipping.'),
 ('3 / COMPARE GUITARS','Test the FGN with Fluence Moderns and the Duo-Sonic separately. Record trim position, peak and quiet RMS. Choose a setting that avoids clipping both.'),
 ('IF IT FAILS','codec=0: inspect3.3V, GND and I2C18/19. Flat-zero audio: inspect LINE IN L, input ground and short I2S headers. Low signal: confirm source before increasing gain.')],diagram='circuit',check='Confirm audible rig noise does not change with USB connected. Avoid correcting hum by disconnecting protective mains earth.')
add('Upload 4: bring VEX to life','Participation XP and one reward per completed practice session',[
 ('1 / CALIBRATE','Set VEX_STAGE4 and upload. Mute the strings, then hold QUEST for1.2s. Remain quiet for3s. Calibration pauses counting and runs again after every boot.'),
 ('2 / PRACTISE','Tap FOOT to start. Detected playing earns10XP per active minute. Silence, pause, clipping and stale audio do not earn time. A completed goal earns one fragment.'),
 ('3 / SAVE','Tap FOOT to pause. Hold MODE until Saved appears before unplugging. The two-record save scheme checks CRC and retains the previous valid record if one write tears.'),
 ('LIMIT','Automatic saves wait for quiet gaps after60s. Abrupt power loss can lose the unsaved portion of a continuous session. This is activity detection, not proof of musical quality.')],diagram='ui',check='Complete a5-minute active goal, verify one fragment, save and reboot. Confirm totals persist and startup is paused.')
add('Controls, tuning and pet behavior','Keep the foot action predictable',rows=[['Control','Tap','Hold 1.2 seconds'],['MODE','Practice / tuner; pauses practice','Save current progress'],['QUEST','While paused: choose5/10/20min; resets session','Calibrate noise; pauses practice'],['FOOT','Start / pause (practice mode, calibrated)','New session; keeps lifetime XP and fragments'],['Tuner','Large note and cents, A440, single notes only','No audio muting; G3S/Lock In handles mute'],['VEX','Riffs while active; rests after30s without activity','No death, debt or punishment for missed days']],widths=[115,330,325],check='Streaks, timed schedules, rhythm scores, bend challenges and battles are future work. No clock/calendar or technique judging is implied.')
add('Understand the CAD','A real assembly, with explicit limits on purchased-part models',[
 ('OPEN','Import VEX_Pocket_Rival_Assembly.step into Fusion. It contains separate named components. STEP preserves geometry, not the Python feature history.'),
 ('EDIT','build_cad.py is the parametric source. Base/lid width and length are linked; component positions and cutouts also appear as explicit construction values. Re-run checks after edits.'),
 ('DATUM','Origin is the footprint centre on the underside. +Y points toward rear sockets; +Z points up. The faceplate spans z42-45mm.'),
 ('EVIDENCE','Display model is manufacturer CAD. Teensy/audio models are community references. Button, footswitch, jack, headers and trimmer include labeled simplified envelopes.')],diagram='cad_top',check='Use named part files for fabrication. Do not print the combined assembly, commercial electronics, wire reserves or the clear-window STL.')
add('Measure and print a coupon first','A cheap fit check before a full case print',[
 ('1 / MEASURE','Record body envelopes, threaded shafts, nut thickness, plug size and solder tails. The jack body is a reserve, not a verified model. Confirm its nut grips the3mm rear wall.'),
 ('2 / PRINT','Print Fit_Coupon.stl flat at100% scale. Try the M3 clearance,7.2mm button hole,12.4mm switch hole,9.8mm jack hole and M3 nut pocket.'),
 ('3 / ADJUST','Use calipers and the coupon to set printer clearances. A nut should seat by hand; a threaded control should pass without force. Change the CAD if purchased parts differ.'),
 ('4 / PRINT THE CASE','Suggested starting point: PETG,0.2mm layers,5perimeters,6top/bottom layers,35% infill. This is a print trial, not a structural rating.')],fig='02_exploded',check='Only measured-fit approval releases the full print. Print base open-side up; faceplate/tray flat; bezel pocket facing up.')
add('Assembly 1: base and rear input','A_base + case/tray nuts + Q_input_jack',[
 ('PARTS','Base A;8M3 captive nuts; Switchcraft111X and included hardware; USB cable. File the test print gently; remove debris.'),
 ('1 / INSERT NUTS','Seat4case nuts at the top corner posts and4tray nuts near the floor. Confirm each screw starts freely before installing electronics.'),
 ('2 / FIT INPUT','Insert the jack from inside through the rear9.8mm hole. Add the stock washer/nut outside. Rotate lugs away from the board area and tighten without twisting wires.'),
 ('3 / TEST USB ACCESS','Feed the selected USB plug through the service opening and check that it can reach the Teensy connector. The opening is not a panel-mounted USB socket.')],fig='03_base',check='Jack cannot rotate; stock threads have proper engagement; cable and corner posts clear the measured jack body.')
add('Assembly 2: removable electronics','E_tray + F_Teensy + G_audio + R_input board',[
 ('PARTS','Tray E;3x15mm audio spacers;2x6mm perfboard spacers; associated M2 hardware;4M3x6 tray screws;2small nylon ties.'),
 ('1 / BUILD ON THE DESK','Attach the socketed Teensy/audio stack. Secure the Teensy through the tray slots with ties clear of components. Install the audio spacers at the three modeled holes.'),
 ('2 / INPUT BOARD','Cut/drill the14x28mm perfboard; fit the trimmer and resistors, then inspect wiring. Mount it on the6mm spacers. Keep solder tails off the tray and adjacent PCB.'),
 ('3 / INSTALL','Lower the tested carrier into the base and fasten the four tray screws. Preserve access to USB and Program; avoid clamping any wire under the tray.')],fig='04_tray',check='Rerun Stage3 before adding the lid. No carrier flex; no metal fastener touches a trace or solder joint.')
add('Assembly 3: display and finger controls','Build the lid assembly while its underside is accessible',[
 ('PARTS','Faceplate B; display H; two 2.0mm spacers; two M2x10 countersunk screws and nuts; two COM-11992 buttons; LED.'),
 ('1 / DISPLAY','Place the display behind the opening with its solder-header tail toward the buttons. Put the2mm spacers between the front PCB mounting pads and the faceplate underside.'),
 ('2 / RETAIN','Seat the M2 countersunk heads flush in the faceplate, then fit nuts below the PCB. Verify flushness before adding the window. Tighten gently; do not load the glass or flex tail.'),
 ('3 / CONTROLS','Fit MODE and QUEST with stock nuts. Insulate leads and leave service length. Fit the LED without adhesive covering its leads or adjacent contacts.')],fig='06_lid_under',check='Measured display holes should be22.86mm apart. Confirm screen centring and no contact between lid and display glass.')
add('Footswitch retention detail','Local cutaway of the faceplate; parts separated vertically for clarity',[
 ('FROM BELOW','Momentary switch N -> lower retaining nut -> aluminum plate M. The nut meets the underside of the plate.'),
 ('THROUGH THE LID','The threaded bushing passes through the plate and faceplate B. Fit the stock washer and upper nut above the lid.'),
 ('SUPPORT','When the lid closes, the plate rests on the four base columns. Verify contact at all four locations; the display is not part of this load path.'),
 ('OPTIONAL CAP','The crimson cap lifts off for service. Leave it out unless its fit and full switch travel are confirmed. Validate the structure before foot use.')],fig='09_foot_stack',check='This illustration explains assembly order. It is not a force rating; use the exact purchased switch and hardware for the fit test.')
add('Assembly 4: footswitch and window','Separate the screen cover from the foot-load path',[
 ('1 / FOOTSWITCH','On the switch bushing: lower nut ->2mm aluminum plate ->3mm lid ->top washer ->top nut. Position the plate so it rests on all four base columns when closed.'),
 ('2 / WINDOW','Place the40x1mm clear disc in the bezel pocket. It has0.3mm nominal radial and axial allowance. Use only a thin peripheral anti-rattle gasket, never pressure on the LCD.'),
 ('3 / BEZEL','Fasten charcoal C with4M3x12 screws and nuts. Bond W gold rim into its shallow recess and W crimson cheek beside C with thin adhesive. Keep the cap clear of full travel.'),
 ('WHY','The replaceable window protects the display. The plate spreads switch force into the base. Both need physical fit and load tests.')],fig='07_window_stack',check='No screen pressure when screws tighten. Hand-actuate the switch through its full travel before any foot test.')
add('Assembly 5: harness, close and test','Finish with service access and repeatable checks',[
 ('1 / ROUTE','Keep the shielded analog pair short and away from display/SPI wires. Use the right-side wire lane. Add heat shrink at every exposed solder lug and resistor lead.'),
 ('2 / SERVICE LOOP','Leave enough slack to lift the lid about50mm and rest it beside the base without pulling display wires. Add strain relief without loading a connector.'),
 ('3 / CLOSE','Inspect from every side while lowering the lid. If it resists, reopen and find the obstruction. Tighten4M3x10 case screws evenly; do not force the printed posts.'),
 ('4 / ACCEPT','Run the CSV acceptance log: electrical, software, optical, mechanical, thermal and rig-noise checks. Keep the Lock In/G3S tuner until the new tuner passes reference tests.')],fig='05_loaded',check='A valid STEP does not establish real wiring clearance or foot strength. The mentor signs the physical acceptance results.')
add('Develop it after the first working build','Small code changes with observable results',[
 ('1 / PERSONALITY','Edit the PNG/JSON art in art/ and run tools/make_sprites.py to regenerate the headers, then make sim-build and tools/render_screen.py. Shared ui.h renders the pet on desktop and device.'),
 ('2 / A NEW REWARD','Change quest rules in core.h. Add a test proving the reward happens once at the threshold and persists after save/restore. Run tests before uploading.'),
 ('3 / BETTER CHALLENGES','Add rhythm/bend scoring as separate exercises with known targets. Keep participation XP independent. Test noise, chords and wrong notes before trusting scores.'),
 ('4 / HARDWARE NEXT','Larger screen, dome, MIDI or9V power each needs a new wiring/CAD revision. Freeze the working R1 files and change one subsystem at a time.')],diagram='ui',check='No new feature should silently change pin ownership, saved-record layout or the meaning of a practice score.')
add('Troubleshoot by the failed checkpoint','Use the earliest failing stage, then repeat later checks',rows=[['Symptom','First checks','Do next'],['No USB port / upload','Known data cable; board selection; physical Program button','Return to Stage1 with other wiring unplugged'],['White / wrong screen','VIN/GND; CS/DC/RST; MOSI11 and SCK13; correct GC9A01A library','Stage2; shorten wires; lower SPI clock if needed'],['Button always pressed','Wrong lug pair or solder bridge to ground','Meter with power off; retest one button'],['codec=0 / no audio','Audio board revision;3.3V;I2C18/19;header alignment','Stage3; known low-level line source'],['False XP in silence','Threshold too low; wrong/wet source; ground noise','Fix source/noise, then recalibrate'],['No XP while playing','Paused / tuner mode; calibration missing; clipping; stale samples','Watch Serial; reduce level or inspect timing'],['Lid will not close','Jack body, solder tails, service loop, stack height','Reopen and measure; never pull shut with screws'],['Tuning unreadable / unstable','Screen size/angle, glare, pitch confidence, clipping, polyphonic input','Use one reference note; retain existing tuner']],widths=[150,330,290],check='Open issues and measured values belong in acceptance_log.csv and measurements.csv, not in assumptions.')
add('What has been checked','Understand what the package proves and what the mentor will prove',[
 ('COMPUTER CHECKS','Valid CAD solids and STEP re-import; all-component overlap checks including fasteners; source-derived display mounts; host tests of counting, rewards, pitch math, saves and button logic.'),
 ('NOT YET PROVEN','Actual display/audio behavior, component revisions, wiring clearance, electrical noise, temperature, lens readability and enclosure strength.'),
 ('TARGET BUILD EVIDENCE','Run make setup-firmware and make firmware-all without hardware. GitHub Actions compiles all stages. Check the run result for this commit; local preparation downloads were blocked by HTTP403.'),
 ('MENTOR RELEASE','Review all purchased parts and the input circuit; complete the log before guitar/foot use. A successful desktop test is useful evidence, not a hardware certificate.')],fig='02_exploded',check='See hardware/generated/checks/ and GitHub Actions. Compile success and physical acceptance are separate evidence.')

add('Review and edit the grouped CAD','Five service groups keep related hardware together',[
 ('OPEN THE ASSEMBLY','Use hardware/generated/VEX_Pocket_Rival_Assembly.step: Chassis, Front Panel, Electronics Carrier, Rear IO and Footswitch. Separate subassembly exports and a flat fallback are also supplied.'),
 ('EDIT THE SOURCE','hardware/cad/build_cad.py is the construction source. assembly_tree.py controls ownership. STEP has named solids, not a native Fusion feature timeline.'),
 ('CHECK THE RESULT','Run make cad cad-check. R1.1 passes72component validity,2556pairwise overlap checks and grouped STEP re-import. Real parts still require measurement.'),
 ('PERSONALITY','The charcoal clipped bezel, thin gold ring, crimson cheek and original pixel bat are small practical additions. Keep the actual32.4mm display size.')],fig='02_exploded',check='See docs/CAD_REVIEW.md for coordinates, file roles and modification constraints.')
add('Try the device before hardware arrives','Shared C++ practice logic and screen renderer run locally',[
 ('RUN','From the repo: make test, then make sim. Open http://127.0.0.1:8765. No Arduino toolchain or third-party Python packages are required for the simulator.'),
 ('EXERCISE','Calibrate, start, choose Playing and advance five active minutes. Try silence, clipping, stale samples, tuner, Save, Reboot and corrupt-record fallback.'),
 ('COMPILE','Install Arduino CLI; make setup-firmware; make firmware-all. This compiles all four stages with pinned dependencies and never uploads.'),
 ('UNDERSTAND THE LIMIT','The simulator injects RMS/peak/Hz; it does not emulate the ADC, pitch detector, USB noise, switches or physical screen optics.')],diagram='ui',check='See docs/LOCAL_DEVELOPMENT.md for Mac setup, dependencies and staged Verify/Upload instructions.')

for start in [0,9]:
 rows=[['ID','Primary source / purpose','Link']]
 for sid,title,url in SOURCES[start:start+9]: rows.append([sid,title,f'<link href="{url}" color="#98394A">Open source</link>'])
 add('Sources and design provenance' if start==0 else 'Sources and next-session handoff','Product references checked16Nov2026; exact source URLs also in SOURCES.md',rows=rows,widths=[45,590,135],check='Adafruit CAD/PCB attribution is retained in vendor/. Imported community models are references; verify the actual hardware.')

PAGES.insert(4,dict(title='How the pieces fit',kicker='Exploded view of the actual CAD assembly',items=None,fig=None,diagram='exploded_key',rows=None,widths=None,check='Letters match the part filenames. Display hardware is recessed; the clear window is captured separately above it.'))
def write_pdf():
 c=canvas.Canvas(str(R/'docs/VEX_Pocket_Rival_Mentor_Guide.pdf'),pagesize=(W,H));c.setTitle('VEX Pocket Rival - Mentor Build Guide R1.1');c.setAuthor('Anish Shah - Guitar project')
 for n,p in enumerate(PAGES,1):
  c.bookmarkPage(f'page{n}');c.addOutlineEntry(p['title'],f'page{n}',level=0)
  header(c,n,p['title'],p['kicker'])
  if p['items']:sections(c,p['items'])
  if p['fig']:figure(c,p['fig'])
  if p['diagram']:globals()[{'routing':'routing','circuit':'circuit','controls':'controls_diagram','window_section':'window_section','cad_top':'cad_top','ui':'ui','exploded_key':'exploded_key'}[p['diagram']]](c)
  if p['rows']:table(c,p['rows'],p['widths'],fs=8.5 if len(p['rows'])>10 else 9.3)
  checkpoint(c,p['check']);c.showPage()
 c.save()
def templates():
 mm=72/25.4; pw,ph=595.28,841.89;c=canvas.Canvas(str(R/'docs/VEX_Pocket_Rival_1to1_Templates.pdf'),pagesize=(pw,ph))
 for page,part in enumerate(['Faceplate','Rear wall / cut parts'],1):
  c.setFillColor(white);c.rect(0,0,pw,ph,fill=1,stroke=0);text(c,'VEX / '+part.upper(),36,ph-40,17,INK,'VexBold');text(c,'PRINT ACTUAL SIZE / 100%. Verify the50mm scale before use.',36,ph-61,10,RED)
  c.setStrokeColor(INK);c.setLineWidth(.6)
  if page==1:
   ox=pw/2;oy=ph/2;sc=mm;c.roundRect(ox-43*sc,oy-73*sc,86*sc,146*sc,12*sc)
   for x,y,r in [(-17,-12,3.6),(17,-12,3.6),(0,-46,6.2),(0,-27,1.6),(0,32,18.3)]+[(x,y,1.7) for x in [-34,34] for y in [-64,64]]+[(x,y,1.7) for x in [-19,19] for y in [13,51]]+[(x,14.22,1.2) for x in [-11.43,11.43]]:
    xx=ox+x*sc;yy=oy+y*sc;c.circle(xx,yy,r*sc);c.line(xx-3,yy,xx+3,yy);c.line(xx,yy-3,xx,yy+3)
   text(c,'Rear +Y',ox-25,oy+76*sc,10);text(c,'Front / foot',ox-33,oy-80*sc,10)
   para(c,'Nominal faceplate 86x146x3mm. Main aperture36.6mm; buttons7.2mm; foot12.4mm; LED3.2mm; display screws2.4mm with 4.4mm x90-degree countersinks on top; case/bezel screws3.4mm. Use CAD for the bezel pocket.',36,170,523,10)
  else:
   ox=pw/2;yy=610;c.rect(ox-43*mm,yy,86*mm,42*mm);c.circle(ox-18*mm,yy+20*mm,4.9*mm);c.rect(ox+3*mm,yy+10*mm,18*mm,12*mm)
   text(c,'REAR VIEW: looking from outside; +X is on the left.',36,590,9)
   text(c,'USB portal x=-12; input jack x=+18; z measured from bottom.',36,575,9)
   # rear view from outside reverses X relative to front/top. Mark datum clearly, not a drill mirror instruction.
   text(c,'Mirror horizontally only if working from the inside face.',36,560,9,RED)
   c.roundRect(80,340,55*mm,32*mm,3*mm);c.circle(80+27.5*mm,340+16*mm,6.2*mm);text(c,'2mm ALUMINUM PLATE',80,320,10,RED,'VexBold')
   c.circle(410,385,20*mm);text(c,'1mm CLEAR DISC / DIA40',330,310,10,RED,'VexBold')
   para(c,'Plate centre hole12.4mm;55x32mm outside, R3 corners. Window40mm diameter. DXF profiles are in hardware/generated/drawings/. Verify tool kerf and deburr all edges. Rear-wall drawing is a coordinate reference; test the purchased jack first.',36,260,523,10)
  c.line(36,79,36+50*mm,79);c.line(36,74,36,84);c.line(36+50*mm,74,36+50*mm,84);text(c,'50mm calibration line',36,61,9)
  text(c,'R1.1 / MEASUREMENT REQUIRED / NO AUTOMATIC FIT GUARANTEE',36,27,8,MUTED);c.showPage()
 c.save()
def export_data():
 with open(D/'BOM.csv','w',newline='') as f:
  w=csv.writer(f);w.writerow(['id','qty','part','specification','purpose','source_id','budget_low_usd','budget_high_usd','order_stage','source_url']);urls={a:c for a,b,c in SOURCES};w.writerows([list(r)+[urls.get(r[5],'Original design')] for r in BOM])
 for name,heads,rows in [('wiring.csv',['wire','from','to','suggested_color','note'],WIRING),('fasteners.csv',['location','quantity','screw','mating_hardware'],FASTENERS),('acceptance_log.csv',['id','test','pass_condition','actual','pass_fail','date','mentor'],[list(r)+['','','',''] for r in ACCEPTANCE])]:
  if name == 'acceptance_log.csv' and (D/name).exists(): continue
  with open(D/name,'w',newline='') as f:w=csv.writer(f);w.writerow(heads);w.writerows(rows)
 if not (D/'measurements.csv').exists():
  with open(D/'measurements.csv','w',newline='') as f:
   w=csv.writer(f);w.writerow(['item','CAD_assumption_mm','measured_mm','change_required','mentor_initials'])
   for row in [('Display mounting pitch','22.86'),('Display total thickness','5.21 in imported STEP'),('Audio shield revision','D community model; measure D2'),('Header seated gap','9.4'),('Display spacer thickness','2.0'),('Jack body reserve','17.5x31x17.5'),('Jack mounting hole','9.8'),('Button mounting hole','7.2'),('Foot mounting hole','12.4'),('Foot upper panel stack','2 plate +3 lid +1 washer'),('USB plug reserve','14x19x9'),('Board footprint available','86x146 plus cables'),('Screen readability','32.4 visible diameter'),('Wire service loop','lid lifts50')]:w.writerow(list(row)+['','',''])
 (R/'docs/SOURCES.md').write_text('# Sources\n\nChecked16 November2026. External files and source provenance are distinguished from original enclosure geometry.\n\n'+'\n'.join(f'- **{a} {b}:** {u}' for a,b,u in SOURCES)+'\n\nAdafruit display STEP SHA:66f39106daee442a08a0b5f994fe7cbfb830bd64. Mount coordinates extracted from manufacturer Eagle PCB. Designed by Limor Fried/Ladyada for Adafruit Industries. Manufacturer PCB license is retained in vendor/ADAFRUIT_LICENSE.txt. Prior community Teensy and audio models are carried over from the user\'s earlier pack, with unresolved physical revision differences.\n')
 md='# VEX Pocket Rival - Mentor Build Guide R1.1\n\n'
 for i,p in enumerate(PAGES,1):
  md+=f'\n## {i:02}. {p["title"]}\n\n{p["kicker"]}\n\n'
  for t,b in p.get('items') or []:md+=f'### {t}\n\n{b}\n\n'
  if p.get('rows'):
   rows=p['rows'];md+='|'+'|'.join(str(x) for x in rows[0])+'|\n|'+'|'.join('---' for x in rows[0])+'|\n';md+='\n'.join('|'+'|'.join(str(x) for x in r)+'|' for r in rows[1:])+'\n\n'
  md+='**Checkpoint:** '+p['check']+'\n'
 (R/'docs/BUILD_GUIDE.md').write_text(clean(md))
 (R/'docs/guide_content.json').write_text(json.dumps(PAGES,indent=2))
 print('Planning budget:',sum(r[6] for r in BOM),sum(r[7] for r in BOM))
if __name__=='__main__':
 export_data();write_pdf();templates()
 from shopping import write_shopping
 write_shopping(R)
 print('Wrote',len(PAGES),'guide pages and 2 template pages')
