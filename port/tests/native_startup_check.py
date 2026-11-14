"""The portable startup probe reports compatibility without installing or leaking paths."""
from pathlib import Path
import hashlib
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'port/tools'))
import check_native_startup as probe


class StartupCheckTests(unittest.TestCase):
    def test_match_and_mismatch_are_bound_to_build_and_disc_profile(self):
        with tempfile.TemporaryDirectory() as directory:
            manifest = Path(directory) / 'startup.dat'
            tables = [bytes(range(16)), bytes(16)]
            digest = 0xcbf29ce484222325
            for byte in tables[0][2:5]:
                digest = ((digest ^ byte) * 0x100000001b3) & 0xffffffffffffffff
            data = struct.pack('<4sIQ4I', b'BT3D', 1, digest, 0x20000000, 0, 2, 3)
            manifest.write_bytes(data)
            with patch.object(probe.native_tables, 'identify', return_value=({'id': 'bt4-en'}, [], [])), \
                 patch.object(probe.native_tables, 'reconstruct', return_value=tables):
                result = probe.check('private/path/disc.iso', manifest)
                self.assertEqual(result['status'], 'compatible')
                self.assertEqual(result['profile'], 'bt4-en')
                self.assertEqual(result['manifest_sha256'], hashlib.sha256(data).hexdigest())
                self.assertNotIn('private', str(result))
                tables[0] = bytes(16)
                self.assertEqual(probe.check('private/disc.iso', manifest)['status'], 'startup_mismatch')

    def test_bad_manifest_is_rejected_before_disc_is_opened(self):
        with tempfile.TemporaryDirectory() as directory:
            manifest = Path(directory) / 'startup.dat'
            for data in (b'', b'bad', struct.pack('<4sIQ', b'BT3D', 1, 0)):
                manifest.write_bytes(data)
                with patch.object(probe.native_tables, 'identify') as identify:
                    self.assertEqual(probe.check('disc.iso', manifest)['status'], 'invalid_manifest')
                    identify.assert_not_called()

    def test_unknown_disc_reports_no_path_or_contents(self):
        with tempfile.TemporaryDirectory() as directory:
            manifest = Path(directory) / 'startup.dat'
            manifest.write_bytes(struct.pack('<4sIQ', b'BT3D', 0, 0xcbf29ce484222325))
            with patch.object(probe.native_tables, 'identify', side_effect=ValueError('untrusted input')):
                result = probe.check('private/disc.iso', manifest)
                self.assertEqual(result['status'], 'unsupported_disc')
                self.assertIsNone(result['profile'])
                self.assertNotIn('untrusted', str(result))


if __name__ == '__main__':
    unittest.main()
