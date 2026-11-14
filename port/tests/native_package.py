"""Package real installer resources with tiny asset-free engine fixtures."""
from pathlib import Path
import hashlib
import json
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'port/tools'))
import package_native_setup as package


@unittest.skipUnless(sys.platform == 'win32', 'Windows installer packaging')
class NativePackageTests(unittest.TestCase):
    def test_current_libraries_and_script_resources_are_in_both_archives(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            portable = root / 'portable'
            portable.mkdir()
            for name in ('bt3.exe', 'bt3.exe.mem'):
                (portable / name).write_bytes(b'asset-free test fixture')
            (portable / 'bt3.dat').write_bytes(struct.pack('<4sIQ', b'BT3D', 0, 0xcbf29ce484222325))
            sdl = root / 'SDL3.dll'
            launcher = root / 'Play.exe'
            sdl.write_bytes(b'SDL fixture')
            launcher.write_bytes(b'launcher fixture')
            output = root / 'installer'

            def freeze(command, **kwargs):
                # Exercise the actual package layout without invoking the
                # compiler/freezer or mistaking a fixture for a usable game.
                self.assertEqual(command[1:3], ['-m', 'PyInstaller'])
                name = command[command.index('--name') + 1]
                destination = Path(command[command.index('--distpath') + 1])
                (destination / (name + '.exe')).write_bytes(b'freezer fixture')

            args = ['package_native_setup', '--platform', 'windows', '--version', '999',
                    '--portable', str(portable), '--sdl', str(sdl),
                    '--launcher', str(launcher), '--output', str(output)]
            with patch.object(sys, 'argv', args), patch.object(package.subprocess, 'run', side_effect=freeze):
                package.main()
            payload = output / 'payload'
            manifest = json.loads((payload / 'manifest.json').read_text())
            for name, expected in manifest.items():
                self.assertEqual(hashlib.sha256((payload / name).read_bytes()).hexdigest(), expected, name)
            for name in ('miniaudio', 'miniz', 'lua', 'monocypher'):
                self.assertIn('licenses/' + name + '.txt', manifest)
            self.assertIn('mods/modern_hud/mod.lua', manifest)
            self.assertEqual((payload / 'engine/TagTeam.dat').read_bytes()[:4], b'TTMD')
            for label in ('', 'Updater-'):
                archive = root / ('Tag-Team-Recomp-Beta.999-' + label + 'Windows.zip')
                with zipfile.ZipFile(archive) as packed:
                    names = packed.namelist()
                    self.assertIn('Update.exe', names)
                    self.assertEqual('Setup.exe' in names, not bool(label))
                    self.assertIn('payload/licenses/miniaudio.txt', names)
                    self.assertFalse(any(name.lower().endswith('.iso') for name in names))


if __name__ == '__main__':
    unittest.main()
