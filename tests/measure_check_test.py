"""tools/measure_check.py joins measurements.csv with MEASUREMENTS: in-tolerance rows free their parts,
out-of-tolerance rows without a written change are flagged, text rows count once a value is present, and the
fabrication gate names what each file waits for."""
from pathlib import Path
import csv,sys,tempfile
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
import measure_check as mc
from project_data import MEASUREMENTS

def write(rows,path):
    with open(path,'w',newline='') as f:
        w=csv.writer(f);w.writerow(mc.COLUMNS)
        for m in MEASUREMENTS:w.writerow([m[0],m[1],*rows.get(m[0],('','',''))])

with tempfile.TemporaryDirectory() as d:
    p=Path(d)/'m.csv'
    # Nothing measured: every row missing; only ungated files print now.
    write({},p);results,problems=mc.evaluate(mc.read_csv(p));assert not problems,problems
    assert all(r['status']=='missing' for r in results)
    gates=mc.gate(results,mc.fabrication_files());state={n:s for n,_,s,_ in gates}
    assert state['Fit_Coupon.stl']=='print now' and state['W_gold_insert.stl']=='print now' and state['W_crimson_cheek.stl']=='print now',state
    assert state['A_base.stl']=='wait' and state['B_faceplate.stl']=='wait' and state['Foot_Plate_2mm_Aluminum.dxf']=='wait',state
    # Measured: pitch in tolerance; jack hole 0.5 mm oversize with no change written (flag); shield revision text present;
    # spacer within tolerance but a change marked (flag the other way).
    rows={'Display mounting pitch':('22.9','',''),'Jack mounting hole':('10.3','',''),'Audio shield revision':('D2, 3 holes match','',''),
          'Display spacer thickness':('2.1','use 2.2 washers','')}
    write(rows,p);results,problems=mc.evaluate(mc.read_csv(p));assert not problems
    by={r['item']:r for r in results}
    assert by['Display mounting pitch']['status']=='ok' and abs(by['Display mounting pitch']['deviation']-.04)<1e-9
    assert by['Jack mounting hole']['status']=='out' and 'change_required is empty' in by['Jack mounting hole']['note']
    assert by['Audio shield revision']['status']=='text' and by['Display spacer thickness']['status']=='change'
    gates=mc.gate(results,['A_base.stl','E_electronics_tray.stl','K_display_spacer_1.stl','Fit_Coupon.stl']);g={n:(s,r) for n,_,s,r in gates}
    assert g['A_base.stl'][0]=='wait' and any('Jack mounting hole' in x and '+0.5' in x for x in g['A_base.stl'][1]),g['A_base.stl']
    assert g['K_display_spacer_1.stl'][0]=='wait' and any('change pending' in x for x in g['K_display_spacer_1.stl'][1])
    assert g['E_electronics_tray.stl'][0]=='wait' and any("Header seated gap" in x for x in g['E_electronics_tray.stl'][1])
    # Everything measured in tolerance, no changes: all gated parts become ready.
    full={m[0]:((m[1].split()[0] if m[2] is not None else 'checked'),'','') for m in MEASUREMENTS}
    write(full,p);results,problems=mc.evaluate(mc.read_csv(p));assert not problems
    assert all(s in('ready','print now','cut now') for _,_,s,_ in mc.gate(results,mc.fabrication_files()))
    text=mc.render(results,mc.gate(results,mc.fabrication_files()));assert 'ready' in text and 'Fit_Coupon.stl' in text
    # A CSV row the data does not know is a problem, not a crash.
    with open(p,'a',newline='') as f:csv.writer(f).writerow(['Mystery hole','5','','',''])
    _,problems=mc.evaluate(mc.read_csv(p));assert problems and 'Mystery hole' in problems[0]
assert not mc.structural_check(),mc.structural_check()
print('PASS: measurement checker (tolerances, change flags, text rows, fabrication gate, data/files agreement)')
