"""Render every key VEX screen state through the shared renderer into assets/screens/ and a contact sheet.

Standard library only (uses build/vex-sim). Run `make sim-build` first. Pillow, if installed, is used
only for the contact sheet; without it the individual PNGs are still written.
"""
from pathlib import Path
import subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'simulator'))
from frames import ppm_png,ppm_rgb
OUT=ROOT/'assets/screens';OUT.mkdir(exist_ok=True)
WAV=ROOT/'test_audio/guitar_notes_left_only.wav'
# name -> protocol lines executed on a fresh device before the frame is captured
SCENES={
 'boot_uncalibrated':['state'],
 'calibrating':['serial c','wait 500'],
 'ready':['calibrate'],
 'listening_riff':['calibrate','press foot 100','audio 3000 .1 .3'],
 'paused_summary':['calibrate','press foot 100','audio 125000 .1 .3','press foot 100'],
 'quest_half':['calibrate','press foot 100','audio 150000 .1 .3','audio 200 .1 .3'],
 'fragment_banner':['calibrate','press foot 100','audio 300500 .1 .3'],
 'fragment_done':['calibrate','press foot 100','audio 300000 .1 .3','wait 3500','audio 100 .1 .3'],
 'levelup_banner':['calibrate','press foot 100','audio 600500 .1 .3'],
 'sleeping':['calibrate','wait 31000'],
 'tuner_waiting':['calibrate','press mode 100','wait 200'],
 'tuner_in_tune':['calibrate','press mode 100','audio 500 .1 .3 440 .99'],
 'tuner_flat':['calibrate','press mode 100','audio 500 .1 .3 325 .99'],
 'tuner_sharp':['calibrate','press mode 100','audio 500 .1 .3 84.5 .99'],
 'tuner_clipping':['calibrate','press mode 100','audio 500 .5 1 440 .99'],
 'tuner_listening':['calibrate','press mode 100','audio 500 .1 .3 440 .5'],
 'tuner_uncertain':['calibrate','press mode 100','audio 500 .1 .3 440 .8'],
 'tuner_stale':['calibrate','press mode 100','audio 500 .1 .3 84.5 .99','wait 600'],
 'tuner_wav_g3':['calibrate','press mode 100',f'wav {WAV} 12300 1200'],
 'drill_pulse':['calibrate','drill pulse-qtr','press foot 100','notes 92 2 0 -18 E2 E2 E2 E2 A2 A2 A2 A2 D3 D3 D3 D3 G3 G3 G3 G3'],
 'drill_hunt':['calibrate','drill hunt-str','press foot 100','notes 140 0 0 -18 F#3'],
 'drill_bend':['calibrate','drill bend-half','press foot 100','vib 1200 E4 5.6 42 -16'],
 'drill_run':['calibrate','drill run-penta','press foot 100','notes 112x2 2 0 -18 A2 C3 D3 E3 G3 A3 G3 E3 D3 C3 A2 ~C3 D3 ~E3 G3 A3'],
 'drill_touch':['calibrate','drill touch-acc3','press foot 100','notes 110x2 2 0 -20 A2@+6 A2 A2 D3@+6 D3 D3 E3@+6 E3 E3 A2@+6 A2 A2'],
 'stage2_screen_check':['stage 2','state'],
 'stage3_audio_meter':['stage 3','audio 200 .02 .45'],
 'stage3_clipping':['stage 3','audio 200 .3 1'],
}
def render(name,lines):
    with tempfile.TemporaryDirectory() as tmp:
        frame=Path(tmp)/'f.ppm'
        subprocess.run([str(ROOT/'build/vex-sim'),str(frame),str(Path(tmp)/'s.bin')],input='\n'.join(lines)+'\n',text=True,check=True,capture_output=True)
        data=frame.read_bytes()
    (OUT/f'{name}.png').write_bytes(ppm_png(data));return ppm_rgb(data)
def main():
    frames={name:render(name,lines) for name,lines in SCENES.items()}
    try:
        from PIL import Image
    except ImportError:
        print(f'{len(frames)} screens written to assets/screens/ (install Pillow for the contact sheet)');return
    cols=5;rows=(len(frames)+cols-1)//cols;sheet=Image.new('RGB',(cols*250,rows*262),(243,238,228))
    from PIL import ImageDraw
    d=ImageDraw.Draw(sheet)
    for i,(name,rgb) in enumerate(frames.items()):
        im=Image.frombytes('RGB',(240,240),rgb);x,y=(i%cols)*250+5,(i//cols)*262+5
        mask=Image.new('L',(240,240),0);ImageDraw.Draw(mask).ellipse((0,0,239,239),fill=255)
        sheet.paste(im,(x,y),mask);d.text((x+4,y+244),name,fill=(41,37,43))
    sheet.save(OUT/'contact_sheet.png');print(f'{len(frames)} screens and contact sheet written to assets/screens/')
if __name__=='__main__':main()
