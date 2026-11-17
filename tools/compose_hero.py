"""Lay out CAD renders and type deterministically with Pillow (no generated art)."""
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont, ImageFilter

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'assets/marketing'
FONTS = ROOT / 'assets/fonts'
CREAM, MUTED, GOLD, RED = '#f3e8d6', '#a9a6ae', '#cba66e', '#a93748'


def font(size, bold=False, mono=False):
    name = 'DejaVuSansMono.ttf' if mono else 'DejaVuSans-Bold.ttf' if bold else 'DejaVuSans.ttf'
    return ImageFont.truetype(str(FONTS / name), size)


def compose(width, height, social=False):
    # Work at one design size for consistent geometry across both crops.
    canvas = Image.new('RGBA', (1920, 1080), '#15151c')
    glow = Image.new('RGBA', canvas.size)
    g = ImageDraw.Draw(glow)
    g.ellipse((800, 120, 1990, 1200), fill=(104, 34, 46, 40))
    canvas = Image.alpha_composite(canvas, glow.filter(ImageFilter.GaussianBlur(130)))
    product = Image.open(ROOT / 'build/hero/product.png').convert('RGBA')
    product.thumbnail((1120, 1120), Image.Resampling.LANCZOS)
    canvas.alpha_composite(product, (815, -50))
    d = ImageDraw.Draw(canvas)
    d.rectangle((100, 106, 146, 112), fill=RED)
    d.text((165, 90), 'PEDALPARTNER   /   POCKET RIVAL', fill=GOLD, font=font(21, mono=True))
    d.text((90, 190), 'VEX', fill=CREAM, font=font(158, bold=True))
    d.text((100, 392), 'Make practice', fill=CREAM, font=font(51, bold=True))
    d.text((100, 458), 'a little more alive.', fill=CREAM, font=font(51, bold=True))
    d.text((104, 574), 'A pixel companion for your guitar.', fill=MUTED, font=font(27))
    d.text((104, 622), 'Play. Take a quest. Grow together.', fill=MUTED, font=font(27))
    d.line((105, 735, 652, 735), fill='#39313a', width=2)
    for y, title, body in [(774, 'LISTEN', 'Buffered dry input'), (824, 'PRACTICE', 'Participation XP + short quests')]:
        d.text((105, y), title, fill=GOLD, font=font(19, mono=True))
        d.text((246, y-1), body, fill=CREAM, font=font(21))
    d.line((100, 960, 1820, 960), fill='#39313a', width=2)
    d.text((104, 992), 'R1.1  /  CAD STUDY  /  MENTOR-REVIEW PROTOTYPE', fill=MUTED, font=font(18, mono=True))
    d.text((1240, 992), '86 x 146 x 45 mm  |  USB 5 V', fill=GOLD, font=font(18, mono=True))
    # The social crop uses a small top/bottom crop, keeping all copy in the safe area.
    if social:
        canvas = canvas.crop((0, 36, 1920, 1044))
    return canvas.convert('RGB').resize((width, height), Image.Resampling.LANCZOS)


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    outputs = []
    for name, width, height, social in [('vex-hero.jpg', 1920, 1080, False), ('vex-social.jpg', 1200, 630, True)]:
        path = OUT / name
        compose(width, height, social).save(path, quality=88, optimize=True, progressive=True, subsampling=0)
        outputs.append({'path': str(path.relative_to(ROOT)), 'width': width, 'height': height,
                        'bytes': path.stat().st_size, 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
    record = json.loads((ROOT / 'build/hero/render.json').read_text())
    record['outputs'] = outputs
    record['reproduce'] = 'make hero'
    record['source_files'] = {
        name: hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in (
            'hardware/cad/export_render_meshes.py', 'hardware/cad/render_cad.py',
            'hardware/cad/render_hero.py', 'tools/compose_hero.py')}
    (OUT / 'provenance.json').write_text(json.dumps(record, indent=2) + '\n')
    print('PASS: hero and social JPEGs composed; provenance records exact CAD and image hashes.')


if __name__ == '__main__':
    main()
