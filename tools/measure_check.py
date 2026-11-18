#!/usr/bin/env python3
"""Read hardware/data/measurements.csv against MEASUREMENTS in hardware/data/project_data.py and say what the
numbers mean for fabrication.

  python3 tools/measure_check.py                 # the repository CSV
  python3 tools/measure_check.py path/to.csv     # another CSV with the same columns

For every row: CAD assumption, measured value, deviation and tolerance, where the number lives in
hardware/cad/build_cad.py and which printed or cut parts wait on it. Flags a `change_required` column that
disagrees with the numbers. Then classifies every file in hardware/generated/stl and drawings:
  print now   nothing measured gates it (the coupon is the instrument; cosmetic inserts carry no load)
  ready       every measurement it depends on is recorded, within tolerance and needs no CAD change
  wait        names the rows still missing, out of tolerance or marked for a change
Measured values are entered by a person with calipers; this script never writes the CSV. Stdlib only.
"""
from pathlib import Path
import csv,re,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'hardware/data'))
from project_data import MEASUREMENTS,CUT_PROFILES
NUMBER=re.compile(r'-?\d+(?:\.\d+)?')
COLUMNS=['item','CAD_assumption_mm','measured_mm','change_required','checked_by']

def first_number(text):
    m=NUMBER.search(text or '');return float(m.group()) if m else None

def read_csv(path):
    rows=list(csv.DictReader(Path(path).open()))
    missing=[c for c in COLUMNS if rows and c not in rows[0]]
    if missing:raise ValueError(f'{path}: missing columns {missing}')
    return rows

def evaluate(rows):
    """Join CSV rows with MEASUREMENTS. Returns (results, problems). Each result is a dict with status
    'missing' | 'ok' | 'out' | 'text' (text row, value present) | 'change' (in tolerance but a change is marked)."""
    spec={m[0]:m for m in MEASUREMENTS};results=[];problems=[]
    seen=set()
    for r in rows:
        item=r['item'].strip()
        if item not in spec:problems.append(f'CSV row {item!r} has no MEASUREMENTS entry in hardware/data/project_data.py');continue
        seen.add(item);_,nominal,tol,ref,parts=spec[item]
        measured=(r.get('measured_mm') or '').strip();change=(r.get('change_required') or '').strip()
        res={'item':item,'nominal':nominal,'tolerance':tol,'ref':ref,'parts':list(parts),'measured':measured,'change':change,'deviation':None,'status':'missing','note':''}
        if r['CAD_assumption_mm'].strip()!=nominal:problems.append(f'{item}: CSV assumption {r["CAD_assumption_mm"]!r} differs from project_data {nominal!r}')
        if not measured:res['status']='missing'
        elif tol is None:
            res['status']='change' if change else 'text'
        else:
            nominal_value=first_number(nominal);value=first_number(measured)
            if value is None:res['status']='missing';res['note']='measured field has no number'
            else:
                res['deviation']=round(value-nominal_value,3);within=abs(res['deviation'])<=tol+1e-9
                if within and not change:res['status']='ok'
                elif within and change:res['status']='change';res['note']='within tolerance but a change is marked; confirm which is right'
                elif not within and not change:res['status']='out';res['note']='outside tolerance and change_required is empty; decide and write the change'
                else:res['status']='change'
        results.append(res)
    for item in spec:
        if item not in seen:problems.append(f'MEASUREMENTS entry {item!r} is not in the CSV; add the row')
    return results,problems

def part_kinds():
    import json
    return {k:v.get('kind') for k,v in json.load((ROOT/'hardware/generated/parts.json').open()).items()}

def gate(results,files,kinds=None):
    """Classify fabrication files. `files` is an iterable of file names (stl/dxf). Returns list of (file, part, state, reasons).
    Cut-sheet parts (aluminium plate, window, perfboard) read 'cut now'; their STL is a reference, the DXF or stock is the output."""
    kinds=part_kinds() if kinds is None else kinds
    by_part={}
    for res in results:
        for p in res['parts']:by_part.setdefault(p,[]).append(res)
    out=[]
    for name in sorted(files):
        part=CUT_PROFILES.get(name,Path(name).stem)
        deps=by_part.get(part,[])
        cut=name.endswith('.dxf') or kinds.get(part)=='cut_sheet'
        if not deps:out.append((name,part,'cut now' if cut else 'print now',[]));continue
        reasons=[]
        for d in deps:
            if d['status']=='missing':reasons.append(f"measure '{d['item']}'")
            elif d['status']=='out':reasons.append(f"'{d['item']}' off by {d['deviation']:+g} mm (tol {d['tolerance']})")
            elif d['status']=='change':reasons.append(f"'{d['item']}' change pending: {d['change'] or 'unspecified'}")
        out.append((name,part,'ready' if not reasons else 'wait',reasons))
    return out

def fabrication_files():
    return [p.name for p in (ROOT/'hardware/generated/stl').glob('*.stl')]+[p.name for p in (ROOT/'hardware/generated/drawings').glob('*.dxf')]

def structural_check():
    """Used by tools/check_project.py: data and files agree; no CSV is interpreted here beyond parsing."""
    import json
    errors=[];rows=read_csv(ROOT/'hardware/data/measurements.csv');results,problems=evaluate(rows);errors+=problems
    parts=set(json.load((ROOT/'hardware/generated/parts.json').open()))
    for m in MEASUREMENTS:
        for p in m[4]:
            if p not in parts:errors.append(f'MEASUREMENTS {m[0]!r}: unknown part {p}')
        if m[2] is not None and first_number(m[1]) is None:errors.append(f'MEASUREMENTS {m[0]!r}: numeric tolerance but no number in {m[1]!r}')
    files=fabrication_files()
    for name,part,state,_ in gate(results,files):
        if part not in parts and part!='Fit_Coupon':errors.append(f'{name}: no part {part} in parts.json (add it to CUT_PROFILES or rename)')   # the coupon is outside the assembly by design
    for name in CUT_PROFILES:
        if name not in files:errors.append(f'CUT_PROFILES lists {name} but hardware/generated/drawings has no such file')
    return errors

def render(results,gates):
    out=['Measurements (hardware/data/measurements.csv against hardware/cad/build_cad.py)']
    for r in results:
        head=f"  {r['item']:<26} CAD {r['nominal']:<28} measured {r['measured'] or '-':<14}"
        if r['tolerance'] is not None:head+=f" dev {('%+g' % r['deviation']) if r['deviation'] is not None else '-':>7} / tol {r['tolerance']:<4}"
        else:head+=' '*22
        out.append(f"{head} [{r['status']}]"+(f"  {r['note']}" if r['note'] else ''))
        out.append(f"      in CAD: {r['ref']}");out.append(f"      gates: {', '.join(r['parts']) if r['parts'] else 'nothing (judgement / wiring)'}"+(f"  change: {r['change']}" if r['change'] else ''))
    out.append('');out.append('Fabrication gate (hardware/generated/stl and drawings)')
    for name,part,state,reasons in gates:
        out.append(f"  {state:<9} {name:<32} {part}"+(f"  <- {'; '.join(reasons)}" if reasons else ''))
    counts={}
    for _,_,state,_ in gates:counts[state]=counts.get(state,0)+1
    out.append('');out.append('  '+' · '.join(f'{k} {v}' for k,v in sorted(counts.items()))+'. Print the coupon first; the table above says what each other part waits for.')
    return '\n'.join(out)

def main():
    path=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/'hardware/data/measurements.csv'
    results,problems=evaluate(read_csv(path));gates=gate(results,fabrication_files())
    print(render(results,gates))
    if problems:print('\nProblems:\n  '+'\n  '.join(problems));sys.exit(1)

if __name__=='__main__':main()
