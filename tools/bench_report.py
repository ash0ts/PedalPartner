#!/usr/bin/env python3
"""Summarise one bench session log (build/bench/*.jsonl written by `make monitor` or the workbench bridge).

  python3 tools/bench_report.py                 # build/bench/latest.jsonl
  python3 tools/bench_report.py build/bench/20261118-091500.jsonl

Prints what the Teensy reported: run time and reboots, the knob and switch states seen, display flush
timing against the cadence, panel bytes, loop and audio timing, level range and clipping, calibration,
activity and fragments. Ends with the acceptance-log rows these numbers inform and the DMA-driver gate
from docs/HANDOFF.md. It reads and prints only; the measured fields in hardware/data/acceptance_log.csv
are entered by a person after looking at the panel and the meter, not by this script. Stdlib only.
"""
from pathlib import Path
import json,statistics,sys
ROOT=Path(__file__).resolve().parents[1]
CLIP=.98                 # ui.h and acceptance row 06 treat peak >= 0.98 as clipping
DMA_GATE_FRACTION=3      # keep the blocking driver while flush_max_us < cadence_ms*1000 / 3 (HANDOFF)

def load(path):
    """Yield (at, kind, item) from a bench log; tolerates a truncated last line."""
    for line in Path(path).read_text().splitlines():
        line=line.strip()
        if not line:continue
        try:row=json.loads(line)
        except ValueError:continue
        at=row.pop('at',0)
        for kind,item in row.items():yield at,kind,item

def pct(values,p):
    if not values:return None
    s=sorted(values);i=min(len(s)-1,max(0,round((len(s)-1)*p)));return s[i]

def summarise(events):
    """Reduce log events to a dict of plain numbers; `events` is an iterable of (at, kind, item)."""
    tel=[];states=[];texts=[];session=None;first_at=last_at=None
    for at,kind,item in events:
        first_at=at if first_at is None else min(first_at,at);last_at=at if last_at is None else max(last_at,at)
        if kind=='telemetry':tel.append(item)
        elif kind=='state':states.append(item)
        elif kind=='text':texts.append(item)
        elif kind=='session':session=item
    s={'session':session or {},'log_seconds':round((last_at or 0)-(first_at or 0),1),'telemetry_rows':len(tel),'state_packets':len(states),'text_lines':len(texts)}
    s['usb_ok_lines']=sum(1 for t in texts if 'VEX USB OK' in t)
    s['stages']=sorted({r.get('stage') for r in tel if 'stage' in r})
    # Reboots: the sketch's uptime goes backwards (telemetry up_ms) or a state packet's now_ms does.
    reboots=0;prev=None
    for r in tel:
        up=r.get('up_ms')
        if up is None:continue
        if prev is not None and up<prev:reboots+=1
        prev=up
    prev=None
    for st in states:
        now=st.get('now_ms')
        if now is None:continue
        if prev is not None and now<prev:reboots+=1
        prev=now
    s['reboots']=reboots
    ups=[r['up_ms'] for r in tel if 'up_ms' in r];s['uptime_max_s']=round(max(ups)/1000,1) if ups else None
    def seen(key):return sorted({r[key] for r in tel if key in r},key=lambda v:(str(type(v)),v))
    s['knobs']={k:seen(k) for k in ('spi_hz','in_level','cadence_ms','diff','full','link','test')}
    cadence=seen('cadence_ms');s['cadence_ms']=max(cadence) if cadence else None
    flush=[r['flush_us'] for r in tel if 'flush_us' in r]+[st['flush_us'] for st in states if 'flush_us' in st]
    flush=[f for f in flush if f]   # zero means no draw in that second
    s['flush_us']={'median':pct(flush,.5),'p95':pct(flush,.95),'max':max(flush) if flush else None,'samples':len(flush)}
    fmax=[r['flush_max_us'] for r in tel if 'flush_max_us' in r];s['flush_max_us']=max(fmax) if fmax else (s['flush_us']['max'])
    tx=[r['tx_bytes'] for r in tel if 'tx_bytes' in r];txm=[r['tx_max_bytes'] for r in tel if 'tx_max_bytes' in r]
    s['tx_bytes_max']=max(txm) if txm else (max(tx) if tx else None)
    loop=[r['loop_max_us'] for r in tel if 'loop_max_us' in r];s['loop_max_us']=max(loop) if loop else None
    cpu=[r['cpu'] for r in tel if 'cpu' in r];s['cpu_max']=max(cpu) if cpu else None
    mem=[r['memory'] for r in tel if 'memory' in r];s['memory_max']=max(mem) if mem else None
    yin=[r['yin_us'] for r in tel if r.get('yin_us')];ymax=[r['yin_max_us'] for r in tel if 'yin_max_us' in r]
    s['yin_us']={'median':pct(yin,.5),'max':max(ymax) if ymax else (max(yin) if yin else None)}
    rms=[r['rms'] for r in tel if 'rms' in r];peak=[r['peak'] for r in tel if 'peak' in r]
    s['rms']={'min':min(rms) if rms else None,'max':max(rms) if rms else None}
    s['peak']={'min':min(peak) if peak else None,'max':max(peak) if peak else None}
    s['clip_seconds']=sum(1 for p in peak if p>=CLIP)
    codec=[r['codec'] for r in tel if 'codec' in r];s['codec']={'seen':len(codec),'ok':sum(1 for c in codec if c)}
    gates=[r['gate'] for r in tel if r.get('gate')];s['gate']=gates[-1] if gates else None
    s['active_seconds']=sum(1 for r in tel if r.get('active'));s['running_seconds']=sum(1 for r in tel if r.get('running'))
    frags=[r['fragments'] for r in tel if 'fragments' in r];s['fragments']={'first':frags[0] if frags else None,'last':frags[-1] if frags else None}
    total=[r['total_s'] for r in tel if 'total_s' in r];s['total_s']={'first':total[0] if total else None,'last':total[-1] if total else None}
    level=[r['level'] for r in tel if 'level' in r];s['level_last']=level[-1] if level else None
    pitch=[r for r in tel if r.get('hz')];s['pitch_rows']=len(pitch)
    if s['cadence_ms'] and s['flush_max_us'] is not None:
        budget=s['cadence_ms']*1000;s['dma_gate']={'budget_us':budget,'limit_us':budget//DMA_GATE_FRACTION,'flush_max_us':s['flush_max_us'],'keep_blocking_driver':s['flush_max_us']<budget//DMA_GATE_FRACTION}
    else:s['dma_gate']=None
    return s

def fmt(v,unit=''):
    if v is None:return 'n/a'
    if isinstance(v,float):return f'{v:.4g}{unit}'
    return f'{v}{unit}'

def render(s):
    """Human-readable report text for a summary dict."""
    out=[];ses=s['session']
    out.append(f"Bench session {ses.get('started','?')} on {ses.get('port') or '?'} ({ses.get('note','')})")
    out.append(f"  log {s['log_seconds']} s · {s['telemetry_rows']} telemetry rows · {s['state_packets']} state packets · {s['text_lines']} other lines")
    out.append(f"  stages seen {s['stages'] or 'none'} · uptime max {fmt(s['uptime_max_s'],' s')} · reboots {s['reboots']} · 'VEX USB OK' lines {s['usb_ok_lines']}")
    k=s['knobs']
    out.append(f"  knobs: spi_hz {k['spi_hz'] or 'n/a'} · in_level {k['in_level'] or 'n/a'} · cadence_ms {k['cadence_ms'] or 'n/a'} · diff {k['diff']} full {k['full']} link {k['link']} test {k['test']}")
    f=s['flush_us']
    out.append(f"  display: flush_us median {fmt(f['median'])} p95 {fmt(f['p95'])} max {fmt(f['max'])} ({f['samples']} samples) · flush_max_us {fmt(s['flush_max_us'])} · tx_max_bytes {fmt(s['tx_bytes_max'])}")
    out.append(f"  loop_max_us {fmt(s['loop_max_us'])} · audio cpu max {fmt(s['cpu_max'],' %')} · audio memory max {fmt(s['memory_max'])} · yin_us median {fmt(s['yin_us']['median'])} max {fmt(s['yin_us']['max'])} · pitch rows {s['pitch_rows']}")
    out.append(f"  level: rms {fmt(s['rms']['min'])}..{fmt(s['rms']['max'])} · peak {fmt(s['peak']['min'])}..{fmt(s['peak']['max'])} · seconds at/above {CLIP} {s['clip_seconds']} · codec ok {s['codec']['ok']}/{s['codec']['seen']}")
    out.append(f"  practice: gate {fmt(s['gate'])} · running {s['running_seconds']} s · active {s['active_seconds']} s · total_s {fmt(s['total_s']['first'])} -> {fmt(s['total_s']['last'])} · fragments {fmt(s['fragments']['first'])} -> {fmt(s['fragments']['last'])} · level {fmt(s['level_last'])}")
    out.append('')
    out.append('Informs acceptance rows (hardware/data/acceptance_log.csv; enter the measured fields yourself):')
    rows=[]
    rows.append(('03 USB upload',f"{s['usb_ok_lines']} 'VEX USB OK' lines" if s['usb_ok_lines'] else 'stage 1 text not in this log'))
    if 2 in s['stages'] or f['samples']:rows.append(('04 Display and controls',f"flush max {fmt(s['flush_max_us'])} us of {fmt(s['cadence_ms']*1000 if s['cadence_ms'] else None)} budget; readability and button 0/1 behaviour are yours to judge"))
    if s['codec']['seen']:rows.append(('05 Codec',f"codec ok {s['codec']['ok']}/{s['codec']['seen']} s; rms range {fmt(s['rms']['min'])}..{fmt(s['rms']['max'])}"))
    if s['peak']['max'] is not None:rows.append(('06 Input attenuation',f"peak max {fmt(s['peak']['max'])}; {s['clip_seconds']} s at/above {CLIP}; in_level seen {k['in_level'] or 'n/a'} (target 0.10-0.80 strong playing, never 0.98)"))
    rows.append(('07 Silence calibration',f"gate {fmt(s['gate'])}" if s['gate'] else 'no calibration seen (gate 0)'))
    if s['running_seconds']:rows.append(('08 Activity',f"active {s['active_seconds']} of {s['running_seconds']} running seconds; total_s +{(s['total_s']['last'] or 0)-(s['total_s']['first'] or 0)}"))
    if s['fragments']['first'] is not None:rows.append(('09 Quest award',f"fragments {s['fragments']['first']} -> {s['fragments']['last']}"))
    rows.append(('18 Heat / run',f"uptime max {fmt(s['uptime_max_s'],' s')}, reboots {s['reboots']}"+(' (30 min reached)' if (s['uptime_max_s'] or 0)>=1800 else ' (30 min not reached)')))
    for name,text in rows:out.append(f'  {name:<26} {text}')
    out.append('')
    g=s['dma_gate']
    if g:out.append(f"DMA gate: flush_max_us {g['flush_max_us']} vs limit {g['limit_us']} (a third of {g['budget_us']}) -> "+('keep the pinned Adafruit path' if g['keep_blocking_driver'] else 'open the DMA-driver task in docs/PLAN.md with these numbers')+'. Seams and tearing are judged by eye.')
    else:out.append('DMA gate: no flush timing in this log (stage 2 or later with the display attached is needed).')
    return '\n'.join(out)

def main():
    arg=sys.argv[1] if len(sys.argv)>1 else None
    path=Path(arg) if arg else ROOT/'build/bench/latest.jsonl'
    if not path.exists():sys.exit(f'No bench log at {path}. `make monitor` or `make sim DEVICE=auto` writes build/bench/<start>.jsonl.')
    print(render(summarise(load(path))))

if __name__=='__main__':main()
