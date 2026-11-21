"""Regression tests for deterministic offline bundles and Android associations."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

COMMON = Path(__file__).resolve().parents[1] / 'common'


class PWABuildTests(unittest.TestCase):
    def test_asset_only_changes_update_worker_but_identical_builds_do_not(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            web, build = root / 'web', root / 'build'
            (web / 'common').mkdir(parents=True)
            (web / 'icons').mkdir()
            (web / 'game.js').write_text('const gameConfig = ' + json.dumps(dict(title='Test', tagline='Test game')) + ';')
            (web / 'common/service-worker.js').write_bytes((COMMON / 'service-worker.js').read_bytes())
            for size in [192, 512]: (web / f'icons/icon-{size}.png').write_bytes(b'icon fixture')
            assets = ['index.html', 'index.js', 'index.wasm', 'index.data', 'web/game.js']
            assets += ['web/common/' + name for name in ['controls.js', 'touch.js', 'style.css', 'pwa.js', 'legacy_gl.js']]
            for name in assets:
                path = build / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b'asset fixture')
            command = [sys.executable, str(COMMON / 'pwa_build.py'), '--build', str(build), '--web', str(web)]
            subprocess.run(command, check=True, capture_output=True)
            worker = build / 'sw.js'
            original, modified = worker.read_bytes(), worker.stat().st_mtime_ns
            subprocess.run(command, check=True, capture_output=True)
            self.assertEqual(worker.stat().st_mtime_ns, modified)
            (build / 'web/common/style.css').write_text('body { color: red; }')
            subprocess.run(command, check=True, capture_output=True)
            self.assertNotEqual(worker.read_bytes(), original)
            self.assertNotIn(b'__BUILD_VERSION__', worker.read_bytes())
            self.assertNotIn(b'__PRECACHE_ASSETS__', worker.read_bytes())

    def test_associations_preserve_other_apps_and_normalize_fingerprints(self):
        with tempfile.TemporaryDirectory() as folder:
            output = Path(folder) / 'assetlinks.json'
            for package in ['org.example.first', 'org.example.second', 'org.example.first']:
                subprocess.run([sys.executable, str(COMMON / 'assetlinks.py'), '--package', package,
                                '--sha256', 'ab' * 32, '--output', str(output)], check=True, capture_output=True)
            statements = json.loads(output.read_text())
            self.assertEqual(len(statements), 2)
            self.assertEqual({s['target']['package_name'] for s in statements}, {'org.example.first', 'org.example.second'})
            self.assertEqual(statements[0]['target']['sha256_cert_fingerprints'], [':'.join(['AB'] * 32)])

    def test_invalid_certificate_does_not_create_association(self):
        with tempfile.TemporaryDirectory() as folder:
            output = Path(folder) / 'assetlinks.json'
            result = subprocess.run([sys.executable, str(COMMON / 'assetlinks.py'), '--package', 'org.example.game',
                                     '--sha256', 'placeholder', '--output', str(output)], capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertFalse(output.exists())


if __name__ == '__main__': unittest.main(verbosity=2)
