"""Material identities must survive palette edits and reject broken uploads."""
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import warrior_colours as colours


class MaterialColours(unittest.TestCase):
    def model(self):
        texture=bytearray(256)
        struct.pack_into('<II',texture,0,1,2)
        struct.pack_into('<IIII',texture,8,20,48,112,64)
        struct.pack_into('<Q',texture,56,19<<20)
        texture[176:192]=bytes(range(16))
        header=struct.pack('<14I',12,*([56]*12),312)
        return bytearray(header+texture)

    def test_palette_changes_keep_identity(self):
        model=self.model();before=colours.catalog(model)
        model[56+192:56+256]=bytes([123])*64
        self.assertEqual(colours.catalog(model),before)
        model[56+176]^=1
        self.assertNotEqual(colours.catalog(model)[0]['key'],before[0]['key'])

    def test_upload_outside_file_rejected(self):
        model=self.model();struct.pack_into('<I',model,56+16,10000)
        with self.assertRaises(ValueError):colours.catalog(model)

    def test_truncated_header_rejected(self):
        for size in (0,3,20,55):
            with self.subTest(size=size),self.assertRaises(ValueError):
                colours.catalog(bytes(self.model()[:size]))


if __name__=='__main__':unittest.main()
