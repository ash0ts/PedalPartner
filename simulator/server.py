"""Loopback-only simulator. Browser controls drive the compiled shared firmware state machine."""
import argparse,json,subprocess,sys,threading
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'simulator'))
from frames import ppm_png

TAP_MS=100
HOLD_MS=1500   # longer than the firmware's 1200 ms hold threshold plus debounce
LIVE_RAW=ROOT/'build/live.raw'

class Device:
    def __init__(self):
        self.lock=threading.Lock()
        self.frame=ROOT/'build/screen.ppm'
        self.proc=subprocess.Popen([str(ROOT/'build/vex-sim'),str(self.frame),str(ROOT/'build/sim-progress.bin')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,text=True,bufsize=1)
    def command(self,*lines):
        with self.lock:
            reply=None
            for line in lines:
                self.proc.stdin.write(line+'\n');self.proc.stdin.flush()
                reply=self.proc.stdout.readline()
                if not reply:raise RuntimeError('Simulator process stopped')
            return json.loads(reply)
    def png(self):
        with self.lock:
            return ppm_png(self.frame.read_bytes())

def translate(d):
    """Map a browser action to simulator protocol lines. Raises ValueError for bad input."""
    action=d['action']
    if action=='step':
        ms=int(d.get('ms',100));rms=float(d.get('rms',0));peak=float(d.get('peak',0));hz=float(d.get('hz',0));fresh=bool(d.get('fresh',True))
        if not (0<=ms<=3600000 and 0<=rms<=1 and 0<=peak<=1 and 0<=hz<=20000):raise ValueError('Input out of range')
        return [f'audio {ms} {rms} {peak} {hz} 1' if fresh else f'wait {ms}']
    if action in {'foot','mode','quest'}:
        ms=int(d.get('ms',HOLD_MS if d.get('hold') else TAP_MS))
        if not 30<=ms<=5000:raise ValueError('Press duration out of range')
        return [f'press {action} {ms}']
    if action=='stage':
        stage=int(d.get('stage',4))
        if stage not in [2,3,4]:raise ValueError('Invalid stage')
        return [f'stage {stage}']
    if action=='wav':
        return [f'wav {ROOT/"test_audio/guitar_notes_left_only.wav"}']
    if action in {'state','calibrate','save','reboot','corrupt','reset'}:
        return [action]
    raise ValueError('Unknown action')

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--port',type=int,default=8765);args=parser.parse_args()
    device=Device();device.command('state')
    class Handler(BaseHTTPRequestHandler):
        def respond(self,body,kind='application/json',status=200):
            self.send_response(status);self.send_header('Content-Type',kind);self.send_header('Cache-Control','no-store');self.send_header('Content-Length',str(len(body)));self.end_headers();self.wfile.write(body)
        def do_GET(self):
            if self.path.split('?')[0]=='/frame.png':self.respond(device.png(),'image/png')
            elif self.path=='/':self.respond((ROOT/'simulator/index.html').read_bytes(),'text/html; charset=utf-8')
            else:self.respond(b'{}',status=404)
        def local(self):
            origin=self.headers.get('Origin')
            return not origin or origin in {f'http://127.0.0.1:{args.port}',f'http://localhost:{args.port}'}
        def do_POST(self):
            length=int(self.headers.get('Content-Length','0'))
            if not self.local():return self.respond(b'{}',status=403)
            try:
                if self.path=='/live':
                    # Raw 16-bit mono 44.1 kHz PCM from the browser microphone/line input, at most one second.
                    if length>2*44100 or length%2:raise ValueError('Bad live chunk')
                    LIVE_RAW.write_bytes(self.rfile.read(length))
                    return self.respond(json.dumps(device.command(f'raw {LIVE_RAW}')).encode())
                if self.path!='/api':return self.respond(b'{}',status=404)
                if length>4096:return self.respond(b'{}',status=413)
                d=json.loads(self.rfile.read(length))
                self.respond(json.dumps(device.command(*translate(d))).encode())
            except (ValueError,KeyError,RuntimeError) as e:self.respond(json.dumps({'error':str(e)}).encode(),status=400)
        def log_message(self,*_):pass
    server=ThreadingHTTPServer(('127.0.0.1',args.port),Handler)
    print(f'VEX simulator: http://127.0.0.1:{args.port} (Ctrl-C to stop)',flush=True)
    try:server.serve_forever()
    except KeyboardInterrupt:pass
    finally:server.server_close();device.proc.terminate();device.proc.wait(timeout=5)
if __name__=='__main__':main()
