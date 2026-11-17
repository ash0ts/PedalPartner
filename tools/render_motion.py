"""Record VEX's motion through the shared renderer at the real display cadence and encode a video.

Drives build/vex-sim through a storyboard (idle breathe and blink, strum, fragment cheer, level-up,
session summary, sleep, tuner listening and a tuned note), capturing one frame every DISPLAY_MS and
checking that each segment shows the animation it should (the simulator reports `anim`/`frame`) and
that looping animations advance exactly one frame per cadence. Writes assets/motion/vex-motion.mp4,
vex-motion.gif, storyboard.png (one frame per segment) and storyboard.json. Standard library plus
ffmpeg; run `make sim-build` first. Pixels come from the same code the Teensy runs; the video is
not evidence of SPI timing on the panel.
"""
from pathlib import Path
import json,re,shutil,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'simulator'))
from frames import ppm_rgb
sys.path.insert(0,str(ROOT/'tools'));from pngio import encode_rgba
import make_sprites as art
OUT=ROOT/'assets/motion';SIM=ROOT/'build/vex-sim'
MANIFEST={a['name']:a for a in json.loads((ROOT/'art/vex/manifest.json').read_text())['animations']}
ANIMS=list(MANIFEST)
STEP=int(re.search(r'DISPLAY_MS\s*=\s*(\d+)',(ROOT/'firmware/VEX_Pocket_Rival/config.h').read_text()).group(1))
FPS=1000//STEP
# caption, setup commands (not recorded), recorded step kind, recorded steps, animations allowed to appear.
# `fill <field> <value>` plays guitar until the reported field reaches the value (goal 5 min, level 2 at 10 active min).
STORY=[
 ('Idle: breathe, blink every 4 s',['calibrate','wait 3400'],'wait',22,{'idle','blink'}),
 ('Footswitch: strum while playing',['press foot 100'],'audio',24,{'strum'}),
 ('Quest done: fragment cheer',['fill session_ms 300000'],'audio',29,{'cheer'}),
 ('Pause: session summary',['press foot 100'],'wait',14,{'idle','blink'}),
 ('Level 2: sparkle',['press foot 100','fill active_ms 600000'],'audio',29,{'levelup'}),
 ('30 s idle: sleep',['press foot 100','wait 30800'],'wait',16,{'sleep'}),
 ('MODE: tuner listens',['press mode 100'],'wait',12,{'listen'}),
 ('Tuner: A4 at +12 cents',['audio 300 .1 .3 443 .99'],'audio-note',12,{'listen'}),
]
STEP_CMD={'wait':f'wait {STEP}','audio':f'audio {STEP} .1 .3','audio-note':f'audio {STEP} .1 .3 443 .99'}

BAR=30;FONT_GLYPHS,FONT_WIDTHS=art.load_font(ROOT/'art/fonts/display.png',*art.DISPLAY,proportional=True)
NIGHT=(0x18,0x24,0x28,255);INK=(0xE8,0xE4,0xB8,255)
def compose(ppm,caption):
    """240x240 frame over a caption bar drawn with the firmware's display font (the same glyphs ui.h uses)."""
    rgb=ppm_rgb(ppm);px=[(rgb[i],rgb[i+1],rgb[i+2],255) for i in range(0,len(rgb),3)]+[NIGHT]*(240*BAR)
    width=sum(FONT_WIDTHS[ord(c)-32]+1 for c in caption)-1;x=120-width//2;y=240+(BAR-12)//2
    for c in caption:
        g=ord(c)-32
        for col in range(8):
            bits=FONT_GLYPHS[g][col]
            for row in range(12):
                if bits>>row&1 and 0<=x+col<240:px[(y+row)*240+x+col]=INK
        x+=FONT_WIDTHS[g]+1
    return encode_rgba(240,240+BAR,px)
def run(*cmd):
    r=subprocess.run(cmd,capture_output=True,text=True)
    if r.returncode:sys.exit(f'{Path(cmd[0]).name} failed: {r.stderr[-800:]}')

def main():
    ffmpeg=shutil.which('ffmpeg')
    if not SIM.exists():sys.exit('build/vex-sim missing; run make sim-build')
    OUT.mkdir(parents=True,exist_ok=True);captions=[];seen={};sheet=[];problems=[];index=0
    with tempfile.TemporaryDirectory() as tmp:
        tmp=Path(tmp);frame=tmp/'f.ppm'
        p=subprocess.Popen([str(SIM),str(frame),str(tmp/'s.bin')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,text=True)
        last={}
        def send(line):
            nonlocal last
            if line.startswith('fill '):
                _,field,value=line.split();need=int(value)-int(last.get(field,0))
                if need<=0:sys.exit(f'{line}: already at {last.get(field)}')
                line=f'audio {need} .1 .3'
            p.stdin.write(line+'\n');p.stdin.flush();reply=p.stdout.readline()
            if not reply:sys.exit(f'simulator exited on {line!r}')
            last=json.loads(reply);return last
        for caption,setup,step,count,allowed in STORY:
            for line in setup:send(line)
            start=index;names=[]
            for _ in range(count):
                s=send(STEP_CMD[step]);names.append((ANIMS[s['anim']],s['frame']))
                (tmp/f"{index:04d}.png").write_bytes(compose(frame.read_bytes(),caption));index+=1
            shown={n for n,_ in names}
            if shown-allowed:problems.append(f'{caption}: showed {sorted(shown)}, allowed {sorted(allowed)}')
            for (a,fa),(b,fb) in zip(names,names[1:]):
                m=MANIFEST[a]
                if a==b and m['loop'] and m['frames']>1 and m['frame_ms']==STEP and fb!=(fa+1)%m['frames']:problems.append(f'{caption}: {a} went frame {fa} -> {fb}')
            seen[caption]=sorted(shown);captions.append((caption,start/FPS,index/FPS));sheet.append(start+count//2)
        p.stdin.close();p.wait(timeout=10)
        if problems:sys.exit('motion check failed:\n'+'\n'.join(problems))
        if ffmpeg:
            run(ffmpeg,'-y','-loglevel','error','-framerate',str(FPS),'-i',str(tmp/'%04d.png'),'-vf',f'scale=480:{2*(240+BAR)}:flags=neighbor','-pix_fmt','yuv420p','-c:v','libx264','-crf','20','-movflags','+faststart',str(OUT/'vex-motion.mp4'))
            run(ffmpeg,'-y','-loglevel','error','-framerate',str(FPS),'-i',str(tmp/'%04d.png'),'-vf','split[a][b];[a]palettegen=max_colors=32:stats_mode=diff[p];[b][p]paletteuse=dither=none',str(OUT/'vex-motion.gif'))
            run(ffmpeg,'-y','-loglevel','error','-i',str(tmp/'%04d.png'),'-vf',f"select='{'+'.join(f'eq(n,{i})' for i in sheet)}',tile={len(sheet)}x1:padding=4:color=0xF3EEE4",'-frames:v','1','-update','1',str(OUT/'storyboard.png'))
        else:print('ffmpeg missing: frames verified but no video written')
    total=index/FPS
    (OUT/'storyboard.json').write_text(json.dumps({'display_ms':STEP,'fps':FPS,'seconds':total,'segments':[{'caption':c,'start_s':a,'end_s':b,'animations':seen[c]} for c,a,b in captions]},indent=1))
    sizes=', '.join(f'{f.name} {f.stat().st_size//1024} KB' for f in sorted(OUT.glob('vex-motion.*')))
    print(f'PASS: {index} frames at {FPS} fps ({total:.1f} s) through {len(STORY)} segments; each showed only its animation and loops advanced one frame per cadence. {sizes}')
if __name__=='__main__':main()
