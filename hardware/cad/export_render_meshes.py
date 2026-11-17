"""Tessellate the checked-in STEP for presentation, without rebuilding geometry."""
import hashlib
import json
from pathlib import Path

from build123d import export_stl, import_step

ROOT = Path(__file__).resolve().parents[2]
GENERATED = ROOT / 'hardware/generated'


def main():
    source = GENERATED / 'VEX_Pocket_Rival_Assembly.step'
    expected = json.loads((GENERATED / 'checks/cad_validation.json').read_text())
    by_name = {p['name'].replace('.', '_'): p for p in expected['parts']}
    assembly = import_step(source)
    destination = GENERATED / 'meshes'
    destination.mkdir(exist_ok=True)
    rows = []

    def visit(node):
        if node.label in by_name:
            yield node
        else:
            for child in node.children:
                yield from visit(child)

    parts = list(visit(assembly))
    assert len(parts) == len(by_name) == len({p.label for p in parts})
    for part in parts:
        ref = by_name[part.label]
        bounds = part.bounding_box()
        delta = max(abs(a - b) for a, b in zip(
            list(bounds.min) + list(bounds.max), ref['min'] + ref['max']))
        assert delta < .001, (part.label, delta)
        # STEP labels normalize punctuation; restore the canonical parts.json name.
        target = destination / (ref['name'] + '.stl')
        assert export_stl(part, target, tolerance=.05, angular_tolerance=.1)
        rows.append({'name': ref['name'], 'sha256': hashlib.sha256(target.read_bytes()).hexdigest()})
    # Remove only obsolete STL intermediates, so a later renderer cannot include stale parts.
    for stale in destination.glob('*.stl'):
        if stale.stem not in {p['name'] for p in by_name.values()}:
            stale.unlink()
    manifest = {
        'source': str(source.relative_to(ROOT)),
        'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
        'scope': 'Tessellation of existing STEP; no construction or collision rerun.',
        'tolerance_mm': .05, 'angular_tolerance_rad': .1,
        'parts': rows,
    }
    (destination / 'source.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'PASS: {len(rows)} STEP components tessellated; bounds match recorded CAD.')


if __name__ == '__main__':
    main()
