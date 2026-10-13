"""Workshop release assembly and the actual four-platform workflow contract."""
import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest
import zipfile
import tarfile
import yaml

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('assembly', ROOT / 'scripts/assemble-release.py')
assembly = importlib.util.module_from_spec(spec)
spec.loader.exec_module(assembly)


class WorkshopReleaseTest(unittest.TestCase):
    def test_complete_release_and_rejected_payloads(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            artifacts = root / 'artifacts'
            for prefix, product in [('Impera', 'Impera'), ('Impera-Workshop', 'ImperaWorkshop')]:
                for platform, suffix in assembly.PLATFORMS.items():
                    folder = artifacts / f'{prefix}-{platform}'
                    folder.mkdir(parents=True)
                    name = f'{product}-vtest-{platform}{suffix}'
                    payload = folder / name
                    if platform.startswith('macos'):
                        stage = root / name.removesuffix('.tar.gz')
                        bundle = stage / (product + '.app') / 'Contents'
                        (bundle / 'MacOS').mkdir(parents=True)
                        (bundle / 'MacOS' / product).write_bytes(b'executable')
                        (stage / 'Licenses').mkdir()
                        (stage / 'Licenses/license.txt').write_text('license')
                        (stage / 'README.md').write_text(product)
                        with tarfile.open(payload, 'w:gz') as archive:
                            archive.add(stage, arcname=stage.name)
                    else:
                        payload.write_bytes(b'package')
                    digest = hashlib.sha256(payload.read_bytes()).hexdigest()
                    (folder / (name + '.sha256')).write_text(f'{digest}  {name}\n')
            engine = artifacts / 'Impera-windows-x86_64'
            for name in assembly.WINDOWS_FILES:
                path = engine / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b'engine')
            editor = artifacts / 'Impera-Workshop-windows-x86_64'
            files = ['bin/ImperaWorkshop.exe', 'bin/Qt6Core.dll', 'bin/Qt6Gui.dll',
                     'bin/Qt6Widgets.dll', 'bin/qt.conf', 'plugins/platforms/qwindows.dll',
                     'README.md', 'Run Workshop.cmd', 'Licenses/Qt-runtime.txt',
                     'Licenses/Qt/LGPL-3.0-only.txt', 'bin/vcruntime140.dll']
            for name in files:
                path = editor / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b'editor')
            output = root / 'dist'
            for _ in range(2):
                assembly.assemble(artifacts, 'vtest', output, with_workshop=True)
                self.assertEqual(len(list(output.iterdir())), 8)
                with tarfile.open(output / 'Impera-vtest-linux-x86_64.tar.gz') as archive:
                    names = archive.getnames()
                    self.assertTrue(any(n.endswith('/Mods') for n in names))
                    self.assertTrue(any(n.endswith('/Impera.AppImage') for n in names))
                    self.assertTrue(any(n.endswith('/ImperaWorkshop.AppImage') for n in names))
                    self.assertEqual(archive.getmember('Impera-vtest-linux-x86_64/Impera.AppImage').mode, 0o755)
                with zipfile.ZipFile(output / 'Impera-vtest-windows-x86_64.zip') as archive:
                    self.assertTrue(any(n.endswith('/ImperaWorkshop.exe') for n in archive.namelist()))
                    self.assertTrue(any(n.endswith('/Impera.exe') for n in archive.namelist()))
                    self.assertTrue(any(n.endswith('/Mods/') for n in archive.namelist()))
                    self.assertIn(b'Prefix=.', archive.read('Impera-vtest-windows-x86_64/qt.conf'))
                    self.assertFalse(any(n.endswith('.zip') for n in archive.namelist()))
                for platform in ('macos-x86_64', 'macos-arm64'):
                    with tarfile.open(output / f'Impera-vtest-{platform}.tar.gz') as archive:
                        names = archive.getnames()
                        self.assertTrue(any(n.endswith('/Impera.app') for n in names))
                        self.assertTrue(any(n.endswith('/ImperaWorkshop.app') for n in names))
                        self.assertTrue(any(n.endswith('/Mods') for n in names))
            (editor / 'SAVED.GAM').write_bytes(b'private')
            with self.assertRaisesRegex(ValueError, 'Windows Workshop'):
                assembly.assemble(artifacts, 'vtest', output, with_workshop=True)
            (editor / 'SAVED.GAM').unlink()
            (editor / 'plugins/platforms/qwindows.dll').unlink()
            with self.assertRaisesRegex(ValueError, 'Windows Workshop'):
                assembly.assemble(artifacts, 'vtest', output, with_workshop=True)
            folder = artifacts / 'Impera-Workshop-linux-x86_64'
            next(folder.glob('*.AppImage')).write_bytes(b'corrupt')
            with self.assertRaisesRegex(ValueError, 'Checksum mismatch'):
                assembly.assemble(artifacts, 'vtest', output, with_workshop=True)

    def test_workflow_preserves_platforms_and_requires_workshop_success(self):
        workflow = yaml.safe_load((ROOT / '.github/workflows/release.yml').read_text())
        builds = workflow['jobs']['build']
        self.assertEqual({p['platform'] for p in builds['strategy']['matrix']['include']},
                         {'linux-x86_64', 'windows-x86_64', 'macos-x86_64', 'macos-arm64'})
        steps = {s.get('name'): s for s in builds['steps']}
        # One root build includes both products; native validation runs on every event.
        self.assertIn('IMPERA_BUILD_WORKSHOP=ON', steps['Configure']['run'])
        self.assertIn('IMPERA_WORKSHOP_DEPLOY_QT=ON', steps['Configure']['run'])
        commands = [s.get('run', '') for s in builds['steps']]
        self.assertEqual(sum(command.startswith('cmake -S ') for command in commands), 1)
        self.assertEqual(sum(command.startswith('cmake --build ') for command in commands), 1)
        self.assertEqual(sum(command.startswith('ctest ') for command in commands), 1)
        for name in ('Configure', 'Build', 'Tests', 'Native executable startup'):
            self.assertNotIn('continue-on-error', steps[name])
            self.assertNotIn('if', steps[name])
        self.assertEqual(steps['Tests']['env']['QT_QPA_PLATFORM'], 'offscreen')
        release_steps = builds['steps'][builds['steps'].index(steps['Package']):]
        for step in release_steps:
            self.assertNotIn('continue-on-error', step)
            self.assertTrue(step['if'].startswith("startsWith(github.ref, 'refs/tags/v')"))
        self.assertIn('--build release-build/workshop', steps['Package Workshop with Qt runtime']['run'])
        release = workflow['jobs']['release']
        self.assertEqual(release['needs'], 'build')
        self.assertEqual(release['if'], "startsWith(github.ref, 'refs/tags/v')")
        self.assertIn('--with-workshop', next(s['run'] for s in release['steps']
                                            if s.get('name') == 'Assemble release downloads'))
        events = workflow.get('on', workflow.get(True))
        self.assertEqual(events['push']['branches'], ['main'])
        self.assertEqual(events['push']['tags'], ['v*'])
        self.assertIn('workflow_dispatch', events)
        for event in ('push', 'pull_request'):
            self.assertIn('src/**', events[event]['paths'])
            self.assertIn('workshop/**', events[event]['paths'])
        self.assertEqual(workflow['permissions'], {'contents': 'read'})
        self.assertEqual(release['permissions'], {'contents': 'write'})
        self.assertIn('github.ref', workflow['concurrency']['group'])
        self.assertIn("!startsWith(github.ref, 'refs/tags/')", workflow['concurrency']['cancel-in-progress'])
        smoke = steps['Workshop package startup smoke check']['run']
        self.assertEqual(smoke.count("'--playtest-smoke-test'"), 3)
        self.assertIn('timeout=60', smoke)
        self.assertIn('PASS native dialogue packaged smoke test', smoke)



if __name__ == '__main__':
    unittest.main()
