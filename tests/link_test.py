"""The Python link decoder reads the stream the C++ encoder wrote (build/link_sample.bin from core_tests):
text line, 100-row band, state packet, 8-row band; survives arbitrary chunking and a corrupted byte."""
from pathlib import Path
import random,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
import link
sample=ROOT/'build/link_sample.bin'
if not sample.exists():sys.exit('build/link_sample.bin is missing; run build/core-tests first (make test does).')
data=sample.read_bytes()
def decode(chunks):
    d=link.Decoder();out=[]
    for c in chunks:out+=d.feed(c)
    return out
whole=decode([data])
kinds=[k for k,_ in whole];assert kinds==['text','band','state','band'],kinds
assert whole[0][1].startswith('{"stage":4'),whole[0][1]
y,h,px=whole[1][1];assert (y,h,len(px))==(36,100,100*240*2)
state=whole[2][1];assert state['flush_us']==4321 and state['loop_max_us']==12000 and state['running'] and state['anim_name']=='strum' and state['activity_name']=='pet',state
assert whole[3][1][:2]==(200,8)
frame=link.Frame();frame.apply(y,h,px);assert frame.generation==1 and not frame.complete and len(frame.rgb())==240*240*3
# Any chunking yields the same packets.
rng=random.Random(7)
for _ in range(5):
    cuts=sorted(rng.sample(range(1,len(data)),12));chunks=[data[a:b] for a,b in zip([0]+cuts,cuts+[len(data)])]
    assert [k for k,_ in decode(chunks)]==kinds
# One flipped pixel byte drops only that band; the rest still decodes.
bad=bytearray(data);bad[len(whole[0][1])+1+link.HEADER.size+7]^=1
assert [k for k,_ in decode([bytes(bad)])]==['text','state','band']
print('PASS: link decoder (text + bands + state, chunking, CRC rejection)')
