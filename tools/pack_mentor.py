"""Create a small, reproducible review ZIP from current files; never build CAD/firmware."""
import argparse
import hashlib
import json
from pathlib import Path
from zipfile import ZipFile, ZipInfo, ZIP_DEFLATED

ROOT = Path(__file__).resolve().parents[1]
FIXED = (
    'LICENSE',
    'docs/VEX_Pocket_Rival_Mentor_Guide.pdf',
    'docs/VEX_Pocket_Rival_1to1_Templates.pdf',
    'docs/SHOPPING.md',
    'docs/PRINTING.md',
    'docs/THIRD_PARTY.md',
    'hardware/vendor/PROVENANCE.md',
    'hardware/vendor/ADAFRUIT_LICENSE.txt',
    'assets/fonts/LICENSE.txt',
    'hardware/generated/VEX_Pocket_Rival_Assembly.step',
    'hardware/generated/cad/Wiring_Reserves.step',
    'hardware/generated/parts.json',
    'hardware/generated/assembly_tree.json',
    'hardware/generated/checks/cad_validation.json',
    'hardware/generated/checks/step_roundtrip.json',
    'assets/marketing/vex-hero.jpg',
    'assets/marketing/vex-social.jpg',
    'assets/marketing/provenance.json',
)
PRINT_PARTS = ('A_base', 'B_faceplate', 'C_gold_bezel', 'W_gold_insert',
               'W_crimson_cheek', 'E_electronics_tray', 'K_display_spacer_1',
               'K_display_spacer_2', 'O_crimson_cap', 'Fit_Coupon')
FIXED += tuple('hardware/generated/stl/' + name + '.stl' for name in PRINT_PARTS)
GLOBS = ('hardware/data/*.csv', 'hardware/generated/drawings/*.dxf')
START = """# VEX / Pocket Rival — R1.1 mentor review pack

Unzip once and open this file. Keep the folders together. This is a review and
fit-planning packet; it is not a fabrication release or a complete source checkout.

1. Start with `docs/VEX_Pocket_Rival_Mentor_Guide.pdf` for assembly, wiring and bench order.
2. Open `hardware/generated/VEX_Pocket_Rival_Assembly.step` in CAD. It contains five
   named service groups and 72 components, in millimetres. Use `parts.json` for
   provenance and `assembly_tree.json` for ownership. The separate
   `cad/Wiring_Reserves.step` describes reserved space, not physical parts.
3. Review `docs/SHOPPING.md` and `hardware/data/BOM.csv`. Plan around Teensy 4.1,
   Audio Adapter Rev D/D2, Adafruit 6178, two SparkFun COM-11992 buttons,
   P-H605-BLK footswitch and Switchcraft 111X jack. Bring actual parts for fit review.
4. Record real dimensions in `hardware/data/measurements.csv`; keep unknowns blank.
   Try `hardware/generated/stl/Fit_Coupon.stl` before committing to the enclosure.
5. Read `docs/PRINTING.md`. Use `docs/VEX_Pocket_Rival_1to1_Templates.pdf`, `hardware/generated/stl/` and
   `hardware/generated/drawings/` only after fit decisions. The window and load plate
   use their specified polycarbonate/aluminium stock. Cut the input perfboard;
   do not print it. Only the ten selected print candidates are included as STLs.
6. `assets/marketing/` contains renders of the actual CAD, with proposed finishes.
   The images are not photographs or proof of fit, readability or foot strength.

The nominal enclosure is 86 x 146 x 45 mm; the active screen is 32.4 mm across.
USB 5 V only. Buffered dry audio input only. No audio output or bypass.
Teensy target compilation and physical acceptance remain unverified.

The pack deliberately omits duplicate flat/subassembly STEPs, vendor source CAD,
legacy/concept images, Blender intermediates and firmware toolchains. Source,
simulator, editable construction and latest status remain at:
https://github.com/ash0ts/PedalPartner

`MANIFEST.json` hashes every other included file, including this index. It proves
which bytes were packaged, not that new CAD or electrical checks were performed.
Read `docs/THIRD_PARTY.md` and `hardware/vendor/PROVENANCE.md` for attribution and
unresolved redistribution terms for community reference geometry.
"""


def safe_bytes(root, relative):
    path = root / relative
    if path.is_symlink() or not path.resolve().is_relative_to(root.resolve()):
        raise ValueError(f'Refusing symlink or external path: {relative}')
    return path.read_bytes()


def payloads(root):
    names = set(FIXED)
    for pattern in GLOBS:
        matches = list(root.glob(pattern))
        if not matches:
            raise FileNotFoundError(f'No files match required pack input {pattern}')
        names.update(p.relative_to(root).as_posix() for p in matches)
    data = {name: safe_bytes(root, name) for name in sorted(names)}
    provenance = json.loads(data['assets/marketing/provenance.json'])
    assembly = data['hardware/generated/VEX_Pocket_Rival_Assembly.step']
    if hashlib.sha256(assembly).hexdigest() != provenance['assembly_sha256']:
        raise ValueError('Hero references a different assembly. Regenerate with make hero.')
    for row in provenance['outputs']:
        if hashlib.sha256(data[row['path']]).hexdigest() != row['sha256']:
            raise ValueError(f'Stale marketing image: {row["path"]}')
    data['START_HERE.md'] = START.encode()
    rows = [{'path': n, 'bytes': len(b), 'sha256': hashlib.sha256(b).hexdigest()} for n, b in sorted(data.items())]
    data['MANIFEST.json'] = (json.dumps({'format_version': 1, 'revision': 'R1.1', 'files': rows}, indent=2) + '\n').encode()
    return data


def build_pack(root, output):
    data = payloads(root)  # Fail before touching an existing archive if an input is missing/stale.
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_suffix('.zip.tmp')
    try:
        with ZipFile(temporary, 'w', compression=ZIP_DEFLATED, compresslevel=9) as archive:
            for name, content in sorted(data.items()):
                info = ZipInfo('VEX_R1.1_Mentor_Review/' + name, date_time=(2026, 11, 17, 0, 0, 0))
                info.compress_type = ZIP_DEFLATED
                info.external_attr = 0o100644 << 16
                archive.writestr(info, content, compresslevel=9)
        with ZipFile(temporary) as archive:
            assert archive.testzip() is None
        temporary.replace(output)
    finally:
        temporary.unlink(missing_ok=True)
    return len(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'dist/VEX_R1.1_Mentor_Review.zip')
    args = parser.parse_args()
    count = build_pack(ROOT, args.output)
    print(f'PASS: {args.output} ({count} files, {args.output.stat().st_size:,} bytes)')


if __name__ == '__main__':
    main()
