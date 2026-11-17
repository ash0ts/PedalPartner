"""Re-import STEP and compare every top-level component to the CAD report."""
from pathlib import Path
from build123d import import_step
import json
ROOT=Path(__file__).resolve().parents[2]
R=ROOT/'hardware/generated'
report=json.loads((R/'checks/cad_validation.json').read_text())
assembly=import_step(R/'VEX_Pocket_Rival_Assembly.step')
expected={p['name'].replace('.','_'):p for p in report['parts']}
rows=[]
def physical_parts(node):
    if node.label in expected:
        yield node
    else:
        for child in node.children:
            yield from physical_parts(child)
physical=list(physical_parts(assembly))
assert len(assembly.children)==5, 'Expected five service-oriented top-level assemblies'
assert len({p.label for p in physical})==len(expected), 'Missing or duplicate physical parts'
for part in physical:
    p=expected[part.label]
    bounds=part.bounding_box()
    dv=abs(part.volume-p['volume_mm3'])
    db=max(abs(a-b) for a,b in zip(list(bounds.min)+list(bounds.max),p['min']+p['max']))
    rows.append({'part':part.label,'valid':part.is_valid,'volume_difference_mm3':dv,'bounds_difference_mm':db,
                 'solid_count_matches':len(part.solids())==p['solids'],
                 'passed':bool(part.is_valid and dv<.01 and db<.001 and len(part.solids())==p['solids'])})
result={'passed':bool(assembly.is_valid and len(rows)==len(expected) and all(r['passed'] for r in rows)),
        'components':len(rows),'top_level_groups':[p.label for p in assembly.children],
        'component_sum_volume_mm3':sum(p.volume for p in physical),
        'parts':rows,'scope':'Per-component STEP re-import validity, volume, bounds and solid count. Compound aggregate mass is not used because nested vendor shapes can contain shared topology. Physical fit is not tested.'}
(R/'checks/step_roundtrip.json').write_text(json.dumps(result,indent=2))
assert result['passed'],[r for r in rows if not r['passed']]
assert not report['collisions'],report['collisions']
print('PASS: STEP round-trip of',len(rows),'components; all-pair geometry report has no volumetric overlaps.')
