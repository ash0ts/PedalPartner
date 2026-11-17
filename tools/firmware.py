"""Isolated Arduino setup and compile. This command never uploads firmware."""
from pathlib import Path
import argparse,json,re,shutil,subprocess,sys
ROOT=Path(__file__).resolve().parents[1]
URL='https://www.pjrc.com/teensy/package_teensy_index.json'
CORE='teensy:avr@1.62.0'
FQBN='teensy:avr:teensy41:usb=serial,speed=600,opt=o2std,keys=en-us'
LIBRARIES=['Adafruit BusIO@1.17.4','Adafruit GFX Library@1.13.0','Adafruit GC9A01A@1.2.0']
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('action',choices=['setup','build']);parser.add_argument('--stage',type=int,choices=[1,2,3,4],default=4);parser.add_argument('--all',action='store_true');args=parser.parse_args()
    cli=shutil.which('arduino-cli')
    if not cli:sys.exit('arduino-cli is missing. On macOS: brew install arduino-cli; then make setup-firmware. No upload was attempted.')
    config=ROOT/'.arduino/arduino-cli.yaml';config.parent.mkdir(exist_ok=True)
    # JSON is valid YAML and quotes paths safely, including spaces in a checkout path.
    settings={'boards_manager':{'additional_urls':[URL]},'directories':{'data':str(config.parent/'data'),'downloads':str(config.parent/'downloads'),'user':str(config.parent/'user')}}
    config.write_text(json.dumps(settings,indent=2))
    def run(*parts):
        command=[cli,'--config-file',str(config),*parts];print('+',' '.join(command),flush=True);subprocess.run(command,cwd=ROOT,check=True)
    if args.action=='setup':
        run('core','update-index');run('core','install',CORE)
        run('lib','install','--no-deps',*LIBRARIES);run('core','list');run('lib','list')
        return
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
if __name__=='__main__':main()
