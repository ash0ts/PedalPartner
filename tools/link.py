"""Python side of the bench link (firmware/VEX_Pocket_Rival/link.h).

Decodes the mixed stream a Teensy writes on USB serial: JSON telemetry lines interleaved with binary
band packets (rows of RGB565 pixels) and state packets. Stdlib only; shared by tools/firmware.py
monitor, the tools/web.py device bridge and tests/link_test.py.
"""
import struct,zlib
HEADER=struct.Struct('<2sBBHHI')          # 'VX', type, pad, a, b, crc32(payload)
FRAME_W=FRAME_H=240
STATE=struct.Struct('<5I5f6IhH4Bb3B')
STATE_FIELDS=('now_ms','active_ms','session_ms','fragments','sequence','gate','rms','peak','cents','cpu',
              'tx_bytes','flush_us','flush_max_us','yin_us','yin_max_us','loop_max_us','midi','goal_minutes',
              'stage','flags','anim','frame','saved_slot','cadence_mul','level','activity')
FLAGS=('running','calibrated','active','tuning','calibrating','claimed','codec','pitch_valid')
ANIMS=('idle','blink','strum','cheer','levelup','listen','sleep')
ACTIVITIES=('pet','tuner','pulse','hunt','bend','run','touch')
assert STATE.size==76,STATE.size
def parse_state(payload):
    values=dict(zip(STATE_FIELDS,STATE.unpack(payload)));flags=values.pop('flags')
    for i,name in enumerate(FLAGS):values[name]=bool(flags>>i&1)
    values['anim_name']=ANIMS[values['anim']] if values['anim']<len(ANIMS) else str(values['anim'])
    act=values['activity']
    values['activity_name']=ACTIVITIES[act] if act<len(ACTIVITIES) else str(act)
    return values
def parse(buf,at=0):
    """Parse one packet at buf[at:]. Returns (consumed, kind, value): consumed 0 = need more bytes,
    -1 = not a packet here. kind 'band' -> (y, h, bytes); kind 'state' -> dict."""
    if len(buf)-at<HEADER.size:return 0,None,None
    magic,kind,pad,a,b,crc=HEADER.unpack_from(buf,at)
    if magic!=b'VX' or pad or kind not in(ord('B'),ord('S')):return -1,None,None
    if kind==ord('B'):
        if b==0 or a+b>FRAME_H:return -1,None,None
        length=b*FRAME_W*2
    else:
        if a!=STATE.size:return -1,None,None
        length=a
    end=at+HEADER.size+length
    if len(buf)<end:return 0,None,None
    payload=buf[at+HEADER.size:end]
    if zlib.crc32(payload)&0xffffffff!=crc:return -1,None,None
    if kind==ord('B'):return end-at,'band',(a,b,bytes(payload))
    return end-at,'state',parse_state(payload)
class Decoder:
    """Incremental splitter: feed() bytes, get ('text', line) / ('band', (y,h,pixels)) / ('state', dict)."""
    def __init__(self):self.buf=bytearray();self.text=bytearray()
    def feed(self,chunk):
        self.buf+=chunk;out=[];at=0
        while at<len(self.buf):
            used,kind,value=parse(self.buf,at)
            if used>0:
                self._flush_text(out);out.append((kind,value));at+=used;continue
            if used==0:
                if self.buf[at:at+2]==b'VX'[:min(2,len(self.buf)-at)] :break   # possible packet start, wait for more
                used=-1
            byte=self.buf[at];at+=1
            if byte==10:self._flush_text(out)
            elif 32<=byte<127 or byte==9:self.text.append(byte)
            else:self.text.clear()                      # binary noise mid-line: drop the partial line
        del self.buf[:at];return out
    def _flush_text(self,out):
        if self.text:out.append(('text',self.text.decode('ascii','replace')));self.text.clear()
class Frame:
    """240x240 RGB565 little-endian image assembled from band packets."""
    def __init__(self):self.pixels=bytearray(FRAME_W*FRAME_H*2);self.generation=0;self.complete=False
    def apply(self,y,h,pixels):
        self.pixels[y*FRAME_W*2:(y+h)*FRAME_W*2]=pixels;self.generation+=1
        if y==0 and h==FRAME_H:self.complete=True
    def rgb(self):
        out=bytearray(FRAME_W*FRAME_H*3);px=self.pixels
        for i in range(FRAME_W*FRAME_H):
            v=px[2*i]|px[2*i+1]<<8;out[3*i]=(v>>11&31)*255//31;out[3*i+1]=(v>>5&63)*255//63;out[3*i+2]=(v&31)*255//31
        return bytes(out)
class BenchLog:
    """Append-only record of one bench session in build/bench/<start>.jsonl (build/bench/latest.jsonl
    points at it). One JSON object per line: {"at": seconds since start, "telemetry": {...}} for each
    JSON line the sketch printed, {"at", "state": {...}} for each decoded state packet, {"at", "text": ...}
    for any other line. tools/bench_report.py summarises a log; nothing under build/ is committed."""
    def __init__(self,root,port='',note=''):
        import json,time
        from pathlib import Path
        self.json=json;self.t0=time.monotonic();d=Path(root)/'build/bench';d.mkdir(parents=True,exist_ok=True)
        self.path=d/time.strftime('%Y%m%d-%H%M%S.jsonl');self.file=open(self.path,'a')
        latest=d/'latest.jsonl'
        try:
            if latest.is_symlink() or latest.exists():latest.unlink()
            latest.symlink_to(self.path.name)
        except OSError:pass
        self.write('session',{'started':time.strftime('%Y-%m-%dT%H:%M:%S'),'port':port,'note':note})
    def write(self,kind,item):
        import time
        self.file.write(self.json.dumps({'at':round(time.monotonic()-self.t0,3),kind:item},separators=(',',':'))+'\n');self.file.flush()
    def record(self,kind,item):
        """Route one Decoder item: text lines that parse as JSON become telemetry; state dicts and other text pass through."""
        if kind=='state':self.write('state',item)
        elif kind=='text':
            if item.startswith('{'):
                try:self.write('telemetry',self.json.loads(item));return
                except ValueError:pass
            self.write('text',item)
    def close(self):self.file.close()
