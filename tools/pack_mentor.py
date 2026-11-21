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
STUDIO_EXTRA = (
    'docs/STUDIO_BRIEF.md',
    'docs/CAD_REVIEW.md',
    'docs/DECISIONS.md',
    'docs/EXTENSIBILITY.md',
    'docs/BRINGUP.md',
    'docs/HANDOFF.md',
    'docs/BUILD_GUIDE.md',
    'docs/LOCAL_DEVELOPMENT.md',
    'docs/PLAN.md',
    'docs/DRILLS.md',
    'docs/SOURCES.md',
    'hardware/generated/parameters.json',
    'firmware/VEX_Pocket_Rival/config.h',
    'assets/motion/vex-motion.mp4',
    'assets/motion/storyboard.png',
    'assets/screens/contact_sheet.png',
)
START = """# VEX / Pocket Rival — R1.1 review pack

Unzip once and open this file. Keep the folders together. This is a review and
fit-planning packet for a hobbyist building alone or with a mentor; it is not a
fabrication release or a complete source checkout. Where the guide says "mentor",
read: an experienced second person.

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
All four firmware stages compile locally for Teensy 4.1; physical acceptance remains unverified.

The pack deliberately omits duplicate flat/subassembly STEPs, vendor source CAD,
legacy/concept images, Blender intermediates and firmware toolchains. Source,
simulator, editable construction and latest status remain at:
https://github.com/ash0ts/PedalPartner

`MANIFEST.json` hashes every other included file, including this index. It proves
which bytes were packaged, not that new CAD or electrical checks were performed.
Read `docs/THIRD_PARTY.md` and `hardware/vendor/PROVENANCE.md` for attribution and
unresolved redistribution terms for community reference geometry.
"""
START_STUDIO = """# VEX / Pocket Rival — R1.1 studio & fabrication pack

Unzip once and open this file. Keep the folders together. This packet is for a
design studio or fabrication partner reviewing physical fit against sourced BOM
components and building the R1.1 3D-printed and laser-cut prototype enclosure.

1. Read `docs/STUDIO_BRIEF.md` first: what VEX is, the locked R1.1 off-the-shelf
   electronics BOM, where you have full geometry/layout latitude, Phase 1
   deliverables, and how Phase 1 separates from future V2 PCB/volume work.
2. Open `hardware/generated/VEX_Pocket_Rival_Assembly.step` in CAD: five named service
   groups, 72 components, millimetres. `parts.json` records which solids are original
   geometry, vendor models (Adafruit #6178 display) or community/simplified envelopes
   (Teensy 4.1, Audio Adapter, switches, jack); `assembly_tree.json` records
   ownership; `parameters.json` lists driving dimensions; `docs/CAD_REVIEW.md` maps
   every CAD file. `cad/Wiring_Reserves.step` shows reserved cable/plug zones.
3. Order or verify parts from `docs/SHOPPING.md` and `hardware/data/BOM.csv` (live
   SparkFun, Adafruit and Amplified Parts SKUs/links), `fasteners.csv` and `wiring.csv`.
   Record measured physical dimensions in `hardware/data/measurements.csv`.
4. Fabrication inputs: `hardware/generated/drawings/` has the DXFs for the 2 mm
   aluminum foot-load plate and 1 mm clear polycarbonate window; `hardware/generated/stl/`
   has the current print candidates; `docs/PRINTING.md` and
   `docs/VEX_Pocket_Rival_1to1_Templates.pdf` have print and stock notes.
5. `assets/marketing/` shows the intended charcoal, crimson and muted-gold finish
   (renders of the actual CAD). `assets/motion/vex-motion.mp4`, `storyboard.png` and
   `assets/screens/contact_sheet.png` show the 240 x 240 UI and character.
6. `docs/VEX_Pocket_Rival_Mentor_Guide.pdf` (`docs/BUILD_GUIDE.md`) covers wiring,
   staged bring-up and physical acceptance checks; `docs/DECISIONS.md` and
   `docs/EXTENSIBILITY.md` cover rationale and future V2 scope.

Nominal enclosure 86 x 146 x 45 mm; active screen 32.4 mm. USB 5 V only. Buffered dry
audio input only. No audio output or bypass. There is no PCB schematic file, force
rating or electrical/regulatory certification yet; see `docs/STUDIO_BRIEF.md`.

Source, simulator, firmware and latest status: https://github.com/ash0ts/PedalPartner
`MANIFEST.json` hashes every included file. `docs/THIRD_PARTY.md` and
`hardware/vendor/PROVENANCE.md` carry attribution and redistribution limits.
"""
PROFILES = {
    'mentor': dict(folder='VEX_R1.1_Mentor_Review', output='dist/VEX_R1.1_Mentor_Review.zip', extra=(), start=START),
    'studio': dict(folder='VEX_R1.1_Studio_Pack', output='dist/VEX_R1.1_Studio_Pack.zip', extra=STUDIO_EXTRA, start=START_STUDIO),
}


def safe_bytes(root, relative):
    path = root / relative
    if path.is_symlink() or not path.resolve().is_relative_to(root.resolve()):
        raise ValueError(f'Refusing symlink or external path: {relative}')
    return path.read_bytes()


def payloads(root, profile='mentor'):
    spec = PROFILES[profile]
    names = set(FIXED) | set(spec['extra'])
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
    data['START_HERE.md'] = spec['start'].encode()
    rows = [{'path': n, 'bytes': len(b), 'sha256': hashlib.sha256(b).hexdigest()} for n, b in sorted(data.items())]
    data['MANIFEST.json'] = (json.dumps({'format_version': 1, 'revision': 'R1.1', 'profile': profile, 'files': rows}, indent=2) + '\n').encode()
    return data


def build_pack(root, output, profile='mentor'):
    data = payloads(root, profile)  # Fail before touching an existing archive if an input is missing/stale.
    folder = PROFILES[profile]['folder']
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_suffix('.zip.tmp')
    try:
        with ZipFile(temporary, 'w', compression=ZIP_DEFLATED, compresslevel=9) as archive:
            for name, content in sorted(data.items()):
                info = ZipInfo(folder + '/' + name, date_time=(2026, 11, 17, 0, 0, 0))
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
    parser.add_argument('--profile', choices=sorted(PROFILES), default='mentor',
                        help='mentor: hobbyist/mentor review pack; studio: adds the product brief, decisions, motion and screen references')
    parser.add_argument('--output', type=Path, default=None)
    args = parser.parse_args()
    output = args.output or ROOT / PROFILES[args.profile]['output']
    count = build_pack(ROOT, output, args.profile)
    print(f'PASS: {output} ({args.profile} profile, {count} files, {output.stat().st_size:,} bytes)')


if __name__ == '__main__':
    main()
