"""Exercise the compiled shared core and shared display through its public protocol."""
from pathlib import Path
import subprocess,tempfile,json
ROOT=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as tmp:
    frame=Path(tmp)/'frame.ppm';save=Path(tmp)/'save.bin'
    p=subprocess.Popen([str(ROOT/'build/vex-sim'),str(frame),str(save)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,text=True)
    def cmd(s):
        p.stdin.write(s+'\n');p.stdin.flush();return json.loads(p.stdout.readline())
    assert not cmd('state')['calibrated']
    cmd('foot 1');assert cmd('step 1000 .1 .3 440 1 1')['active_ms']==0
    cmd('calibrate');cmd('foot 1')
    s=cmd('step 300000 .1 .3 440 1 1');assert (s['xp'],s['fragments'])==(50,1)
    s=cmd('step 60000 .1 1 440 1 1');assert s['active_ms']==300000
    s=cmd('step 60000 .1 .3 440 1 0');assert s['active_ms']==300000
    s=cmd('step 60000 0 0 440 1 1');assert s['active_ms']==300000
    cmd('save');cmd('step 60000 .1 .3 440 1 1');cmd('save')
    assert cmd('reboot')['active_ms']==360000
    assert not cmd('state')['running'] and not cmd('state')['calibrated']
    cmd('corrupt');assert cmd('reboot')['active_ms']==300000
    cmd('calibrate');cmd('mode');assert cmd('step 1000 .1 .3 440 1 1')['active_ms']==300000
    tuner=frame.read_bytes();assert tuner.startswith(b'P6\n240 240\n255\n') and len(tuner)==172815
    cmd('mode');pet=frame.read_bytes();assert pet!=tuner
    assert len(set(pet[15:]))>8
    p.stdin.close();assert p.wait(timeout=5)==0
print('PASS: simulator rewards, silence/clip/stale gates, save fallback, reboot and shared UI frames')
