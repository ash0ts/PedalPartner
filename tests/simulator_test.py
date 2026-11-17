"""Replay scripted scenarios through the compiled shared firmware state machine and renderer.

Each tests/scenarios/*.txt file holds simulator protocol lines plus two test directives:
  expect key=value ...   compare fields of the last JSON reply (floats within 1e-6, strings exact);
                         key~=value allows 200 ms slack for counters that run on while a held sample is fresh
  golden <name>          compare the current frame with tests/golden/<name>.png pixel for pixel
Run with --update-golden to rewrite golden frames after an intentional UI change.
"""
from pathlib import Path
import json,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'simulator'))
from frames import ppm_rgb,rgb_png,png_rgb,diff_count
SCENARIOS=ROOT/'tests/scenarios';GOLDEN=ROOT/'tests/golden'
UPDATE='--update-golden' in sys.argv

def parse(value):
    if value in ('true','false'):return value=='true'
    try:return float(value) if ('.' in value or 'e' in value) else int(value)
    except ValueError:return value.replace('_',' ')

def run(path):
    failures=[];updated=0
    with tempfile.TemporaryDirectory() as tmp:
        frame=Path(tmp)/'frame.ppm';save=Path(tmp)/'save.bin'
        p=subprocess.Popen([str(ROOT/'build/vex-sim'),str(frame),str(save)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,text=True)
        last={}
        for number,line in enumerate(path.read_text().splitlines(),1):
            line=line.strip()
            if not line or line.startswith('#'):continue
            where=f'{path.name}:{number}'
            if line.startswith('expect '):
                for pair in line.split()[1:]:
                    key,value=pair.split('=',1);approx=key.endswith('~');key=key.rstrip('~');want=parse(value);got=last.get(key)
                    numeric=isinstance(got,(int,float)) and not isinstance(got,bool) and isinstance(want,(int,float))
                    if approx:ok=numeric and abs(got-want)<=200          # held-sample freshness and debounce add tens of ms
                    elif isinstance(want,float):ok=numeric and abs(got-want)<=1e-6
                    else:ok=got==want
                    if not ok:failures.append(f'{where}: {key} expected {want!r}, got {got!r}')
            elif line.startswith('golden '):
                name=line.split()[1];rgb=ppm_rgb(frame.read_bytes());target=GOLDEN/f'{name}.png'
                if UPDATE or not target.exists():
                    GOLDEN.mkdir(exist_ok=True);target.write_bytes(rgb_png(rgb));updated+=1
                elif (d:=diff_count(png_rgb(target.read_bytes()),rgb)):
                    failures.append(f'{where}: frame differs from golden {name} in {d} pixels (run with --update-golden if intended)')
            else:
                p.stdin.write(line+'\n');p.stdin.flush();reply=p.stdout.readline()
                if not reply:failures.append(f'{where}: simulator exited on "{line}"');break
                last=json.loads(reply)
        p.stdin.close();code=p.wait(timeout=10)
        if code!=0:failures.append(f'{path.name}: simulator exit code {code}')
    return failures,updated

def main():
    sample=bytes(range(256))*675
    assert png_rgb(rgb_png(sample))==sample,'PNG helper round trip failed'
    paths=sorted(SCENARIOS.glob('*.txt'));assert paths,'no scenarios found'
    failures=[];updated=0
    for path in paths:
        f,u=run(path);failures+=f;updated+=u
    if failures:
        raise SystemExit('\n'.join(failures))
    note=f' ({updated} golden frames written)' if updated else ''
    print(f'PASS: {len(paths)} simulator scenarios through the shared App, protocol, saves and golden frames{note}')
if __name__=='__main__':main()
