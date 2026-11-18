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
        idle.write_bytes(encode_rgba(w-1,h,px[:(w-1)*h]));expect_error(f'expected {w}x32')
        idle.write_bytes(encode_rgba(w,h,px))
        manifest=art/'vex/manifest.json';original=json.loads(manifest.read_text())
        def with_manifest(edit):
            m=json.loads(json.dumps(original));edit(m['animations'][0]);manifest.write_text(json.dumps(m))
        with_manifest(lambda a:a.update(frame_ms=5));expect_error('frame_ms')
        # A sequence may only name drawn frames, and must not be empty or longer than the step table allows.
        with_manifest(lambda a:a.update(sequence=[0,a['frames']]));expect_error('sequence')
        with_manifest(lambda a:a.update(sequence=[]));expect_error('sequence')
        with_manifest(lambda a:a.update(sequence=[0]*(ms.MAX_STEPS+1)));expect_error('sequence')
        # A valid sequence reaches anim.h as the step table, and the step cost report follows it.
        with_manifest(lambda a:a.update(sequence=[0,1,0]));out2,_,_,anims2,_,frames2=ms.generate()
        assert 'VEX_ANIM_STEPS' in out2['anim.h'] and out2['anim.h'].count('{0,')>=1
        name,steps=ms.step_cost(anims2,frames2)[0];assert name==anims2[0]['name'] and [(i,j) for i,j,_,_ in steps]==[(0,1),(1,0),(0,0)],steps
        assert steps[2][2]==0,'identical frames must cost no rows'
        shutil.copy(ROOT/'art/vex/manifest.json',manifest)
        t=json.loads((art/'theme.json').read_text());del t['ui']['gold'];(art/'theme.json').write_text(json.dumps(t));expect_error("missing role 'gold'")
    print(f'PASS: art/ matches generated headers; importer rejects off-palette pixels, partial alpha, wrong strip size, bad timing, bad sequences and missing roles; sequences reach anim.h with a step cost')
if __name__=='__main__':main()
