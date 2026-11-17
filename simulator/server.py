"""Loopback-only simulator. Browser controls drive the compiled shared C++ core/UI."""
import argparse,json,struct,subprocess,threading,zlib
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
class Device:
    def __init__(self):
        self.lock=threading.Lock()
        self.frame=ROOT/'build/screen.ppm'
        self.proc=subprocess.Popen([str(ROOT/'build/vex-sim'),str(self.frame),str(ROOT/'build/sim-progress.bin')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,text=True,bufsize=1)
    def command(self,line):
        with self.lock:
            self.proc.stdin.write(line+'\n');self.proc.stdin.flush()
            reply=self.proc.stdout.readline()
            if not reply:raise RuntimeError('Simulator process stopped')
            return json.loads(reply)
    def png(self):
        with self.lock:
            data=self.frame.read_bytes().split(b'\n',3)[3]
        def chunk(kind,body):
            return struct.pack('>I',len(body))+kind+body+struct.pack('>I',zlib.crc32(kind+body)&0xffffffff)
        raw=b''.join(b'\0'+data[y*720:(y+1)*720] for y in range(240))
        return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',240,240,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(raw))+chunk(b'IEND',b'')

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
        def do_POST(self):
            if self.path!='/api':return self.respond(b'{}',status=404)
            if int(self.headers.get('Content-Length','0'))>4096:return self.respond(b'{}',status=413)
            origin=self.headers.get('Origin')
            if origin and origin not in {f'http://127.0.0.1:{args.port}',f'http://localhost:{args.port}'}:return self.respond(b'{}',status=403)
            try:
                d=json.loads(self.rfile.read(int(self.headers.get('Content-Length','0'))));action=d['action']
                if action=='step':
                    ms=int(d.get('ms',100));rms=float(d.get('rms',0));peak=float(d.get('peak',0));hz=float(d.get('hz',440));fresh=int(bool(d.get('fresh',True)))
                    if not (0<=ms<=3600000 and 0<=rms<=1 and 0<=peak<=1 and 0<=hz<=20000):raise ValueError('Input out of range')
                    command=f'step {ms} {rms} {peak} {hz} 1 {fresh}'
                elif action=='foot':command='foot '+str(2 if d.get('hold') else 1)
                elif action=='stage':
                    stage=int(d.get('stage',4))
                    if stage not in [2,3,4]:raise ValueError('Invalid stage')
                    command=f'stage {stage}'
                elif action in {'state','mode','quest','calibrate','save','reboot','corrupt','reset'}:command=action
                else:raise ValueError('Unknown action')
                self.respond(json.dumps(device.command(command)).encode())
            except (ValueError,KeyError,RuntimeError) as e:self.respond(json.dumps({'error':str(e)}).encode(),status=400)
        def log_message(self,*_):pass
    server=ThreadingHTTPServer(('127.0.0.1',args.port),Handler)
    print(f'VEX simulator: http://127.0.0.1:{args.port} (Ctrl-C to stop)',flush=True)
    try:server.serve_forever()
    except KeyboardInterrupt:pass
    finally:server.server_close();device.proc.terminate();device.proc.wait(timeout=5)
if __name__=='__main__':main()
