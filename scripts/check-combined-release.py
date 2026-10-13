#!/usr/bin/env python3
"""Launch both applications from the final combined layout on each native runner."""
import argparse
import hashlib
import importlib.util
import os
import re
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile
import zipfile

spec = importlib.util.spec_from_file_location('assembly', Path(__file__).with_name('assemble-release.py'))
assembly = importlib.util.module_from_spec(spec)
spec.loader.exec_module(assembly)


def check(platform, version):
    version = re.sub(r'[^A-Za-z0-9._-]', '-', version)
    with tempfile.TemporaryDirectory() as temporary:
        output = Path(temporary).resolve()
        suffix = '.zip' if platform.startswith('windows') else assembly.PLATFORMS[platform]
        for product, folder in (('Impera', 'dist'), ('ImperaWorkshop', 'workshop-dist')):
            name = f'{product}-{version}-{platform}'
            if platform.startswith('windows'):
                source = Path('windows-artifact' if product == 'Impera' else 'workshop-windows-artifact')
                with zipfile.ZipFile(output / (name + suffix), 'w') as stream:
                    for path in source.rglob('*'):
                        stream.write(path, Path(name) / path.relative_to(source))
            else:
                shutil.copy2(Path(folder) / (name + suffix), output / (name + suffix))
            # Assembly removes these intermediate sidecars after combining.
            digest = hashlib.sha256((output / (name + suffix)).read_bytes()).hexdigest()
            (output / (name + suffix + '.sha256')).write_text(f'{digest}  {name + suffix}\n')
        assembly.combine_downloads(version, output, [platform])
        archive = output / (f'Impera-{version}-{platform}' +
                            ('.zip' if platform.startswith('windows') else '.tar.gz'))
        unpacked = output / 'unpacked'
        if platform.startswith('windows'):
            with zipfile.ZipFile(archive) as stream:
                stream.extractall(unpacked)
        else:
            with tarfile.open(archive) as stream:
                stream.extractall(unpacked, filter='data')
        root = unpacked / f'Impera-{version}-{platform}'
        assert (root / 'Mods').is_dir() and not list((root / 'Mods').iterdir())
        if platform.startswith('windows'):
            commands = [[str(root / 'Impera.exe'), '--help'],
                        [str(root / 'ImperaWorkshop.exe'), '--playtest-smoke-test']]
        elif platform.startswith('macos'):
            commands = [[str(root / 'Impera.app/Contents/MacOS/Impera'), '--help'],
                        [str(root / 'ImperaWorkshop.app/Contents/MacOS/ImperaWorkshop'), '--playtest-smoke-test']]
        else:
            commands = [[str(root / 'Impera.AppImage'), '--appimage-extract-and-run', '--help'],
                        [str(root / 'ImperaWorkshop.AppImage'), '--appimage-extract-and-run', '--playtest-smoke-test']]
        env = {k: v for k, v in os.environ.items() if k not in
               {'LD_LIBRARY_PATH', 'DYLD_LIBRARY_PATH', 'DYLD_FRAMEWORK_PATH',
                'QT_PLUGIN_PATH', 'QT_QPA_PLATFORM_PLUGIN_PATH'}}
        env['QT_QPA_PLATFORM'] = 'offscreen'
        env['U5D_RUNTIME_DIR'] = str(output / 'runtime')
        if os.name == 'nt':
            env['PATH'] = os.pathsep.join(p for p in env['PATH'].split(os.pathsep) if 'qt' not in p.lower())
        for command, expected in zip(commands, ('Usage: ultima5', 'PASS native dialogue packaged smoke test')):
            result = subprocess.run(command, cwd=output, env=env, capture_output=True,
                                    text=True, check=True, timeout=60)
            assert expected in result.stdout, result.stdout + result.stderr
        print(f'PASS combined {platform} applications and empty Mods folder')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--platform', required=True)
    parser.add_argument('--version', default=os.environ.get('RELEASE_VERSION'))
    args = parser.parse_args()
    if not args.version:
        parser.error('--version or RELEASE_VERSION is required')
    check(args.platform, args.version)
