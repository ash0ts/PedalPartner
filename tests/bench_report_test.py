"""tools/bench_report.py reduces a bench log to the numbers the acceptance log asks for: run time, a reboot,
flush timing against the cadence, clipping seconds, calibration, activity and fragments, and the DMA verdict."""
from pathlib import Path
import json,sys,tempfile
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
import bench_report,link

def row(up_ms,**kw):
    base={'stage':4,'up_ms':up_ms,'codec':1,'rms':.05,'peak':.3,'gate':.004,'active':1,'running':1,'session_s':up_ms//1000,'total_s':600+up_ms//1000,
          'fragments':2,'level':2,'cpu':11.3,'memory':12,'tx_bytes':28800,'tx_max_bytes':115200,'flush_us':4100,'flush_max_us':17890,'loop_max_us':18240,
          'cadence_ms':100,'diff':1,'full':0,'link':0,'test':0,'spi_hz':16000000,'in_level':0,'anim':2,'frame':3,'pitch':'yin','hz':110.0,'prob':.9,'yin_us':1480,'yin_max_us':2210}
    base.update(kw);return base

events=[(0,'session',{'started':'2026-11-18T09:15:00','port':'/dev/cu.usbmodem1','note':'test'})]
events.append((0.5,'text','VEX USB OK'))
for i in range(10):events.append((1+i,'telemetry',row(1000*(i+1))))
events.append((11,'telemetry',row(11000,peak=.99,in_level=3)))          # one clipped second after a level step
events.append((12,'telemetry',row(500,fragments=3,flush_max_us=42000,spi_hz=24000000)))   # reboot: uptime restarts; a slow flush
events.append((13,'state',{'now_ms':600,'flush_us':5000}))
events.append((14,'state',{'now_ms':300,'flush_us':4000}))              # second reboot seen through state packets
s=bench_report.summarise(events)
assert s['reboots']==2,s['reboots']
assert s['uptime_max_s']==11.0 and s['stages']==[4] and s['usb_ok_lines']==1
assert s['knobs']['spi_hz']==[16000000,24000000] and s['knobs']['in_level']==[0,3]
assert s['flush_us']['max']==5000 and s['flush_max_us']==42000 and s['flush_us']['samples']==14
assert s['clip_seconds']==1 and s['peak']['max']==.99
assert s['fragments']=={'first':2,'last':3} and s['gate']==.004 and s['active_seconds']==12
assert s['dma_gate']['keep_blocking_driver'] is False and s['dma_gate']['limit_us']==33333
text=bench_report.render(s)
assert 'reboots 2' in text and '09 Quest award' in text and 'open the DMA-driver task' in text and '06 Input attenuation' in text,text
# A quiet stage-2 log keeps the blocking driver and says so.
s2=bench_report.summarise([(i,'telemetry',{'stage':2,'up_ms':1000*i,'flush_us':9000,'flush_max_us':12000,'cadence_ms':100,'tx_bytes':2400}) for i in range(1,5)])
assert s2['dma_gate']['keep_blocking_driver'] and s2['reboots']==0 and s2['stages']==[2]
assert 'keep the pinned Adafruit path' in bench_report.render(s2)
# BenchLog writes what load() reads, routes JSON text to telemetry and keeps latest.jsonl pointing at the file.
with tempfile.TemporaryDirectory() as d:
    log=link.BenchLog(d,'/dev/null','unit test')
    log.record('text','{"stage":2,"up_ms":1000,"flush_us":5000,"cadence_ms":100}');log.record('text','plain line');log.record('state',{'now_ms':1000,'flush_us':5000});log.close()
    kinds=[k for _,k,_ in bench_report.load(log.path)];assert kinds==['session','telemetry','text','state'],kinds
    latest=Path(d)/'build/bench/latest.jsonl';assert latest.exists() and latest.resolve()==log.path.resolve()
    s3=bench_report.summarise(bench_report.load(latest));assert s3['stages']==[2] and s3['flush_us']['samples']==2
print('PASS: bench report (reboots, knobs, flush timing, clipping, practice counters, DMA gate, log round trip)')
