"""The committed firmware art headers must match art/, and the importer must reject bad art precisely."""
from pathlib import Path
import json,shutil,sys,tempfile
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
import make_sprites as ms
from pngio import decode,encode_rgba

def expect_error(fragment):
    try:ms.generate()
    except SystemExit as e:
        assert fragment in str(e),f'expected error mentioning {fragment!r}, got: {e}';return
    raise AssertionError(f'importer accepted art that should fail ({fragment})')

def main():
    out,*_=ms.generate()
    stale=[n for n,t in out.items() if (ms.FW/n).read_text()!=t]
    assert not stale,f'generated headers are stale: {stale}; run python3 tools/make_sprites.py'
    # Round trip of the PNG codec on every frame strip.
    for path in (ROOT/'art/vex').glob('*.png'):
        w,h,px=decode(path.read_bytes());assert decode(encode_rgba(w,h,px))[2]==px,path.name
    with tempfile.TemporaryDirectory() as tmp:
        art=Path(tmp)/'art';shutil.copytree(ROOT/'art',art);ms.ART=art
        idle=art/'vex/idle.png';w,h,px=decode(idle.read_bytes())
        bad=list(px);bad[5*w+5]=(1,2,3,255);idle.write_bytes(encode_rgba(w,h,bad));expect_error('not a theme colour')
        bad=list(px);bad[5*w+5]=(bad[5*w+5][0],bad[5*w+5][1],bad[5*w+5][2],128);idle.write_bytes(encode_rgba(w,h,bad));expect_error('alpha 128')
        idle.write_bytes(encode_rgba(w-1,h,px[:(w-1)*h]));expect_error('expected 64x32')
        idle.write_bytes(encode_rgba(w,h,px))
        m=json.loads((art/'vex/manifest.json').read_text());m['animations'][0]['frame_ms']=5;(art/'vex/manifest.json').write_text(json.dumps(m));expect_error('frame_ms')
        shutil.copy(ROOT/'art/vex/manifest.json',art/'vex/manifest.json')
        t=json.loads((art/'theme.json').read_text());del t['ui']['gold'];(art/'theme.json').write_text(json.dumps(t));expect_error("missing role 'gold'")
    print(f'PASS: art/ matches generated headers; importer rejects off-palette pixels, partial alpha, wrong strip size, bad timing and missing roles')
if __name__=='__main__':main()
