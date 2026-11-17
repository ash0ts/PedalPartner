"""Check cross-file delivery consistency without CAD or Arduino dependencies."""
import ast
import csv
import hashlib
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'hardware/data'))
from project_data import BOM, FASTENERS, WIRING

def main():
    errors = []
    def require(ok, message):
        if not ok:
            errors.append(message)
    for folder in ['tools', 'simulator', 'tests', 'hardware/cad', 'hardware/data']:
        for path in (ROOT/folder).glob('*.py'):
            ast.parse(path.read_text(), filename=str(path))
    for name, expected in [('BOM.csv', BOM), ('fasteners.csv', FASTENERS), ('wiring.csv', WIRING)]:
        rows = list(csv.reader((ROOT/'hardware/data'/name).open()))[1:]
        require(len(rows) == len(expected), f'{name}: stale row count; run make docs')
        for actual, source in zip(rows, expected):
            require(actual[:len(source)] == list(map(str,source)), f'{name}: stale row {source[0]}; run make docs')
    tree = json.loads((ROOT/'hardware/generated/assembly_tree.json').read_text())
    names = [name for groups in tree.values() for parts in groups.values() for name in parts]
    parts = json.loads((ROOT/'hardware/generated/parts.json').read_text())
    actual_names = list(parts)
    # parts.json also records non-physical wiring reserves; assembly tree intentionally excludes them.
    require(len(names) == len(set(names)), 'Assembly has duplicate physical component ownership')
    require(set(names).issubset(set(actual_names)), 'Assembly references absent components')
    report = json.loads((ROOT/'hardware/generated/checks/step_roundtrip.json').read_text())
    require(report['passed'] and report['components'] == len(names), 'STEP report and assembly disagree')
    provenance = json.loads((ROOT/'assets/marketing/provenance.json').read_text())
    require(provenance['components'] == len(names), 'Hero component count disagrees with assembly')
    hashes = {provenance['assembly']: provenance['assembly_sha256'],
              provenance['screen']: provenance['screen_sha256'],
              **provenance['source_files'],
              **{row['path']: row['sha256'] for row in provenance['outputs']}}
    for name, expected_hash in hashes.items():
        require(hashlib.sha256((ROOT/name).read_bytes()).hexdigest() == expected_hash,
                f'{name}: hero provenance is stale; see docs/MARKETING.md')
    for path in [ROOT/'README.md', ROOT/'AGENTS.md', *(ROOT/'docs').glob('*.md')]:
        for target in re.findall(r'\]\(([^\s)]+)\)',path.read_text()):
            if ':' in target or target.startswith('#'):
                continue
            require((path.parent/target.split('#')[0]).exists(), f'{path.relative_to(ROOT)}: missing link {target}')
    for path in (ROOT/'.agents/skills').glob('*/SKILL.md'):
        text = path.read_text()
        require(text.startswith('---\n'), f'{path}: missing frontmatter')
        require(f'name: {path.parent.name}\n' in text, f'{path}: skill name mismatch')
        require(bool(re.search(r'^description: .+',text,re.M)), f'{path}: missing description')
        require('TODO' not in text and len(text.splitlines()) < 500, f'{path}: unfinished or excessive skill')
    for path in [ROOT/'firmware/VEX_Pocket_Rival/VEX_Pocket_Rival.ino',ROOT/'simulator/main.cpp']:
        text = path.read_text()
        require(('ui.h' in text or 'app.h' in text) and 'vex::drawScreen' in text, f'{path}: shared renderer disconnected')
        require('app.h' in text and 'vex::App' in text, f'{path}: shared device state machine disconnected')
    if errors:
        raise SystemExit('\n'.join(errors))
    print('PASS: source syntax, BOM/wiring exports, assembly ownership, STEP report, hero provenance, document links and skills')

if __name__ == '__main__':
    main()
