"""Isolated Arduino setup, compile, and explicit bench upload/monitor.

`setup` and `build` never touch a device. `upload` is the only action that programs a Teensy and it
refuses to guess: it needs --port or exactly one Teensy on USB. `monitor` only reads the serial port.
"""
from pathlib import Path
import argparse,json,re,shutil,subprocess,sys,time
ROOT=Path(__file__).resolve().parents[1]
URL='https://www.pjrc.com/teensy/package_teensy_index.json'
CORE='teensy:avr@1.62.0'
FQBN='teensy:avr:teensy41:usb=serial,speed=600,opt=o2std,keys=en-us'
LIBRARIES=['Adafruit BusIO@1.17.4','Adafruit GFX Library@1.13.0','Adafruit GC9A01A@1.2.0']
def teensy_ports(cli,config):
    """Serial ports arduino-cli attributes to a Teensy (by board name or PJRC VID 16c0)."""
    out=subprocess.run([cli,'--config-file',str(config),'board','list','--format','json'],cwd=ROOT,capture_output=True,text=True)
    if out.returncode:return []
    data=json.loads(out.stdout or '{}');entries=data.get('detected_ports',data if isinstance(data,list) else [])
    ports=[]
    for entry in entries:
        port=entry.get('port',{});label=port.get('address','');props=port.get('properties',{}) or {}
        names=' '.join(b.get('name','') for b in entry.get('matching_boards',[]))
        if 'Teensy' in names or str(props.get('vid','')).lower().endswith('16c0'):ports.append(port.get('address',label))
    return [p for p in ports if p]
def main():
    parser=argparse.ArgumentParser(description=__doc__,formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('action',choices=['setup','build','upload','monitor','ports']);parser.add_argument('--stage',type=int,choices=[1,2,3,4],default=4);parser.add_argument('--all',action='store_true')
    parser.add_argument('--port',help='serial port for upload/monitor, e.g. /dev/cu.usbmodem12345');parser.add_argument('--baud',type=int,default=115200);parser.add_argument('--send',default='',help='monitor: characters to send once the port opens, e.g. "?l"')
    args=parser.parse_args()
    cli=shutil.which('arduino-cli')
    if not cli:sys.exit('arduino-cli is missing. On macOS: brew install arduino-cli; then make setup-firmware. No upload was attempted.')
    config=ROOT/'.arduino/arduino-cli.yaml';config.parent.mkdir(exist_ok=True)
    # JSON is valid YAML and quotes paths safely, including spaces in a checkout path.
    settings={'board_manager':{'additional_urls':[URL]},'directories':{'data':str(config.parent/'data'),'downloads':str(config.parent/'downloads'),'user':str(config.parent/'user')}}
    config.write_text(json.dumps(settings,indent=2))
    def run(*parts):
        command=[cli,'--config-file',str(config),*parts];print('+',' '.join(command),flush=True);subprocess.run(command,cwd=ROOT,check=True)
    if args.action=='setup':
        run('core','update-index');run('core','install',CORE)
        run('lib','install','--no-deps',*LIBRARIES);run('core','list');run('lib','list')
        return
    if args.action=='ports':
        ports=teensy_ports(cli,config);print('\n'.join(ports) if ports else 'No Teensy serial port detected.');return
    if args.action in('upload','monitor'):
        port=args.port
        if not port:
            ports=teensy_ports(cli,config)
            if len(ports)!=1:sys.exit(f'{args.action} needs --port; detected Teensy ports: {ports or "none"}. Nothing was sent to a device.')
            port=ports[0]
        if args.action=='monitor':return monitor(port,args.baud,args.send)
        stage_root=ROOT/'build'/f'stage-{args.stage}';hexes=sorted((stage_root/'output').glob('*.hex')) if stage_root.exists() else []
        if not hexes:sys.exit(f'No compiled image for stage {args.stage}; run make firmware STAGE={args.stage} first. Nothing was uploaded.')
        print(f'Uploading stage {args.stage} ({hexes[0].name}) to {port}. Teensy Loader may ask you to press the button once.',flush=True)
        run('upload','--fqbn',FQBN,'--port',port,'--input-dir',str(stage_root/'output'),str(stage_root/'VEX_Pocket_Rival'))
        (ROOT/'build/upload_report.json').write_text(json.dumps({'stage':args.stage,'port':port,'image':hexes[0].name,'at':time.strftime('%Y-%m-%dT%H:%M:%S')},indent=2))
        print('Upload complete. Next: make monitor (JSON telemetry) or make sim DEVICE=auto (panel mirror).');return
    results=[]
    for stage in ([1,2,3,4] if args.all else [args.stage]):
        stage_root=ROOT/'build'/f'stage-{stage}';sketch=stage_root/'VEX_Pocket_Rival';shutil.copytree(ROOT/'firmware/VEX_Pocket_Rival',sketch,dirs_exist_ok=True)
        p=sketch/'config.h';p.write_text(re.sub(r'#define VEX_STAGE \d+',f'#define VEX_STAGE {stage}',p.read_text()))
        try:
            run('compile','--fqbn',FQBN,'--warnings','all','--build-path',str(stage_root/'objects'),'--output-dir',str(stage_root/'output'),str(sketch))
            results.append({'stage':stage,'status':'PASS','output':f'build/stage-{stage}/output'})
        except subprocess.CalledProcessError:
            results.append({'stage':stage,'status':'FAIL'})
        (ROOT/'build/target_build_report.json').write_text(json.dumps(results,indent=2))
    (ROOT/'build/target_build_report.json').write_text(json.dumps(results,indent=2))
    if any(result['status']=='FAIL' for result in results):
        sys.exit('One or more stages failed; see build/target_build_report.json. No upload was attempted.')
    print('Compile complete. No device was accessed and no firmware was uploaded.')
def monitor(port,baud,send):
    """Print telemetry lines; bench link packets (binary) are summarised, not dumped. Ctrl-C stops."""
    sys.path.insert(0,str(ROOT/'tools'));import link
    try:import serial
    except ImportError:sys.exit('pyserial is missing: python3 -m pip install -r requirements-bench.txt')
    decoder=link.Decoder();print(f'Monitoring {port} at {baud}; Ctrl-C stops. Characters typed here are not forwarded; use --send or the workbench Device tab.',flush=True)
    with serial.Serial(port,baud,timeout=.2) as s:
        if send:s.write(send.encode())
        try:
            while True:
                chunk=s.read(4096)
                if not chunk:continue
                for kind,item in decoder.feed(chunk):
                    if kind=='text':print(item,flush=True)
                    elif kind=='band':print(f'[band y={item[0]} h={item[1]}]',flush=True)
                    elif kind=='state':print(f'[state anim={item["anim"]} frame={item["frame"]} flush_us={item["flush_us"]} tx_bytes={item["tx_bytes"]}]',flush=True)
        except KeyboardInterrupt:print('\nMonitor stopped.')
if __name__=='__main__':main()
