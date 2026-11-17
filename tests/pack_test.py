"""Archive integrity and failure behavior; no CAD, image or firmware dependencies."""
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest
from zipfile import ZipFile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from pack_mentor import FIXED, build_pack


class PackTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name) / 'source'
        for name in (*FIXED, 'hardware/data/measurements.csv', 'hardware/generated/drawings/window.dxf'):
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(('fixture:' + name).encode())
        self.output = Path(self.temp.name) / 'pack.zip'
        self.hero = 'assets/marketing/vex-hero.jpg'
        sha = lambda p: hashlib.sha256((self.root / p).read_bytes()).hexdigest()
        record = {'assembly_sha256': sha('hardware/generated/VEX_Pocket_Rival_Assembly.step'),
                  'outputs': [{'path': self.hero, 'sha256': sha(self.hero)}]}
        (self.root / 'assets/marketing/provenance.json').write_text(json.dumps(record))

    def test_archive_integrity_and_reproducibility(self):
        (self.root / '.env').write_text('private material must not enter the ZIP')
        build_pack(self.root, self.output)
        first = self.output.read_bytes()
        with ZipFile(self.output) as archive:
            prefix = 'VEX_R1.1_Mentor_Review/'
            names = set(archive.namelist())
            self.assertNotIn(prefix + '.env', names)
            manifest = json.loads(archive.read(prefix + 'MANIFEST.json'))
            self.assertEqual(names, {prefix + r['path'] for r in manifest['files']} | {prefix + 'MANIFEST.json'})
            for row in manifest['files']:
                data = archive.read(prefix + row['path'])
                self.assertEqual(hashlib.sha256(data).hexdigest(), row['sha256'])
                self.assertEqual(len(data), row['bytes'])
        build_pack(self.root, self.output)
        self.assertEqual(first, self.output.read_bytes())

    def test_missing_input_preserves_previous_archive(self):
        self.output.write_bytes(b'previous good archive')
        (self.root / 'docs/SHOPPING.md').unlink()
        with self.assertRaises(FileNotFoundError):
            build_pack(self.root, self.output)
        self.assertEqual(self.output.read_bytes(), b'previous good archive')

    def test_stale_image_is_rejected(self):
        (self.root / self.hero).write_bytes(b'changed without provenance')
        with self.assertRaisesRegex(ValueError, 'Stale marketing image'):
            build_pack(self.root, self.output)
        self.assertFalse(self.output.exists())

    def test_external_symlink_is_rejected(self):
        secret = Path(self.temp.name) / 'private'
        secret.write_text('private')
        target = self.root / 'LICENSE'
        target.unlink()
        target.symlink_to(secret)
        with self.assertRaisesRegex(ValueError, 'symlink or external'):
            build_pack(self.root, self.output)


if __name__ == '__main__':
    unittest.main()
