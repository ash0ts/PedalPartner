"""Build, serve and check the in-browser VEX workbench.

The browser runs the firmware's shared headers (app.h, analyzer.h, ui.h) compiled to WebAssembly with
Emscripten; simulator/web/vex_web.cpp is the hardware adapter. Nothing here touches a Teensy.

  python3 tools/web.py build   compile build/web/vex.js + vex.wasm and copy the static page
  python3 tools/web.py serve   static loopback server with rebuild-on-change and live reload
  python3 tools/web.py check   run the module under Node and compare a frame with a native golden
"""
from pathlib import Path
import argparse,hashlib,json,re,shutil,subprocess,sys,threading,time
from http.server import SimpleHTTPRequestHandler,ThreadingHTTPServer

ROOT=Path(__file__).resolve().parents[1]
WEB=ROOT/'simulator/web';OUT=ROOT/'build/web';FIRMWARE=ROOT/'firmware/VEX_Pocket_Rival'
EMSCRIPTEN_MIN='7.0'   # Homebrew bottle used for development; record the actual version in the handoff
EXPORTS=['_vex_boot','_vex_controls','_vex_tick','_vex_observe','_vex_pitch','_vex_serial','_vex_audio_ptr','_vex_audio_capacity','_vex_audio_push',
         '_vex_render','_vex_frame_ptr','_vex_store_ptr','_vex_store_load','_vex_powercut','_vex_state_ptr','_vex_state','_vex_notice',
         '_vex_last_analysis_us','_vex_pose']
SOURCES=[WEB/'vex_web.cpp',*FIRMWARE.glob('*.h'),WEB/'index.html',WEB/'app.js',WEB/'worklet.js']

def emcc():
    cli=shutil.which('em++')
    if not cli:sys.exit('emcc is missing. On macOS: brew install emscripten. The native simulator (make test, build/vex-sim) does not need it.')
    version=subprocess.run([cli,'--version'],capture_output=True,text=True).stdout
    m=re.search(r'(\d+)\.(\d+)\.(\d+)',version)
    if not m or (int(m[1]),int(m[2]))<tuple(int(x) for x in EMSCRIPTEN_MIN.split('.')):
        sys.exit(f'emscripten {EMSCRIPTEN_MIN}+ required; found: {version.strip().splitlines()[0] if version else "unknown"}')
    return cli,m[0]

def build(quiet=False):
    cli,version=emcc();OUT.mkdir(parents=True,exist_ok=True)
    cmd=[cli,'-std=c++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti',
         '-sMODULARIZE=1','-sEXPORT_NAME=VexModule','-sENVIRONMENT=web,node','-sALLOW_MEMORY_GROWTH=0','-sINITIAL_MEMORY=4194304',
         '-sSTACK_SIZE=262144','-sFILESYSTEM=0','-sEXPORTED_FUNCTIONS='+','.join(EXPORTS),
         '-sEXPORTED_RUNTIME_METHODS=HEAPU8,HEAP16,HEAPF64,UTF8ToString',
         str(WEB/'vex_web.cpp'),'-o',str(OUT/'vex.js')]
    log=ROOT/'build/web-build.log'
    with log.open('w') as f:
        f.write(' '.join(cmd)+'\n');r=subprocess.run(cmd,cwd=ROOT,stdout=f,stderr=subprocess.STDOUT)
    if r.returncode:
        lines=[l for l in log.read_text().splitlines() if 'INFO' not in l and not l.startswith('/')][-12:]
        sys.exit('em++ failed; see build/web-build.log:\n'+'\n'.join(l[:300] for l in lines))
    for name in ['index.html','app.js','worklet.js']:shutil.copy(WEB/name,OUT/name)
    shutil.copy(ROOT/'test_audio/guitar_notes_left_only.wav',OUT/'guitar_notes_left_only.wav')
    (OUT/'build.json').write_text(json.dumps({'emscripten':version,'sources':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest()[:16] for p in SOURCES}},indent=1))
    if not quiet:print(f'Built build/web/vex.wasm ({(OUT/"vex.wasm").stat().st_size} bytes) with emscripten {version}')

def stamp():
    # Files can vanish for a moment during a git checkout; ignore them until they return.
    times=[]
    for p in SOURCES:
        try:times.append(p.stat().st_mtime)
        except OSError:pass
    return max(times) if times else 0

def serve(port):
    build();state={'generation':0,'stamp':stamp(),'error':''}
    def watch():
        while True:
            time.sleep(0.5)
            try:
                s=stamp()
                if s==state['stamp']:continue
                time.sleep(0.3);state['stamp']=stamp()   # let an editor or checkout finish writing
                try:build(quiet=True);state['error']='';print('rebuilt',flush=True)
                except SystemExit as e:state['error']=str(e);print(state['error'],flush=True)
                state['generation']+=1
            except Exception as e:   # never let the watcher die; the page would silently stop reloading
                print(f'watch error: {e}',flush=True)
    threading.Thread(target=watch,daemon=True).start()
    class Handler(SimpleHTTPRequestHandler):
        def __init__(self,*a,**k):super().__init__(*a,directory=str(OUT),**k)
        def do_GET(self):
            if self.path.startswith('/reload'):
                # Long poll: answer when the generation changes or after 25 s so the page can re-arm.
                want=int(self.path.split('=')[-1]) if '=' in self.path else -1;deadline=time.time()+25
                while state['generation']==want and time.time()<deadline:time.sleep(0.2)
                body=json.dumps({'generation':state['generation'],'error':state['error']}).encode()
                self.send_response(200);self.send_header('Content-Type','application/json');self.send_header('Cache-Control','no-store');self.send_header('Content-Length',str(len(body)));self.end_headers();self.wfile.write(body);return
            super().do_GET()
        def end_headers(self):
            self.send_header('Cache-Control','no-store');super().end_headers()
        def log_message(self,*_):pass
    server=ThreadingHTTPServer(('127.0.0.1',port),Handler)
    print(f'VEX workbench: http://127.0.0.1:{port} (Ctrl-C to stop; edits rebuild and reload)',flush=True)
    try:server.serve_forever()
    except KeyboardInterrupt:pass
    finally:server.server_close()

def check():
    """Boot the WASM module in Node, replay a scenario prefix and compare the frame with the native golden."""
    node=shutil.which('node')
    if not node or not shutil.which('emcc'):print('SKIP: web check needs node and emcc');return
    build(quiet=True)
    sys.path.insert(0,str(ROOT/'simulator'));from frames import png_rgb,diff_count
    # Two frames replayed with the same steps as the native scenarios: a pet frame and a tuner frame.
    script=r'''
const VexModule=require(process.argv[1]);
VexModule().then(M=>{
  let now=0;const raw={mode:0,quest:0,foot:0};let inject=null;
  const step=()=>{now++;if(inject){M._vex_observe(inject.rms,inject.peak,now);if(inject.hz>0)M._vex_pitch(inject.hz,inject.prob,now);}M._vex_controls(raw.mode,raw.quest,raw.foot,now);M._vex_tick(now);};
  const run=ms=>{for(let i=0;i<ms;i++)step();};
  const press=(name,ms)=>{raw[name]=1;run(ms);raw[name]=0;run(30);};
  const audio=(ms,rms,peak,hz=0,prob=1)=>{inject={rms,peak,hz,prob};run(ms);inject=null;};
  const frame=()=>{const ptr=M._vex_render(now);const rgba=M.HEAPU8.subarray(ptr,ptr+240*240*4);const rgb=Buffer.alloc(240*240*3);
    for(let i=0;i<240*240;i++){rgb[i*3]=rgba[i*4];rgb[i*3+1]=rgba[i*4+1];rgb[i*3+2]=rgba[i*4+2];}return rgb.toString('base64');};
  const frames=[];
  // tests/scenarios/practice_quest.txt up to `golden pet_listening`
  M._vex_boot(4,0);now=0;
  press('foot',100);audio(1000,.1,.3);M._vex_serial(99,now);audio(3100,.001,.002,0,0);press('foot',100);audio(3000,.1,.3);frames.push(frame());
  // tests/scenarios/stages_and_tuner.txt from `stage 4` up to `golden tuner_sharp`
  M._vex_boot(4,0);now=0;
  press('mode',100);audio(500,.1,.3,440,.99);audio(500,.1,.3,82.4069,.99);audio(500,.1,.3,446,.99);frames.push(frame());
  process.stdout.write(frames.join('\n'));
});'''
    r=subprocess.run([node,'-e',script,str(OUT/'vex.js')],capture_output=True,text=True,cwd=ROOT)
    if r.returncode:sys.exit('node failed:\n'+r.stderr[-2000:])
    import base64
    for name,frame in zip(['pet_listening','tuner_sharp'],r.stdout.strip().split('\n')):
        golden=png_rgb((ROOT/f'tests/golden/{name}.png').read_bytes());d=diff_count(golden,base64.b64decode(frame))
        if d:sys.exit(f'WASM frame differs from native golden {name} in {d} pixels')
    print('PASS: WASM build renders native goldens pet_listening and tuner_sharp pixel for pixel')

def main():
    p=argparse.ArgumentParser(description=__doc__,formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('action',choices=['build','serve','check']);p.add_argument('--port',type=int,default=8765);a=p.parse_args()
    {'build':build,'serve':lambda:serve(a.port),'check':check}[a.action]()
if __name__=='__main__':main()
