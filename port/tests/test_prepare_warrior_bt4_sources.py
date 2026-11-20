"""BT4 metadata reconstruction without a creator executable or BT3 data."""
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from prepare_warrior_bt4_sources import costume_families, embed, pack, unpack, playable_fighters, unpack_model


class Bt4DonorPreparation(unittest.TestCase):
    def test_stripped_model_only_accepts_absent_zero_tail_after_required_sections(self):
        entries = [bytes([i]) * 64 for i in range(52)]
        original = pack(entries)
        offset = struct.unpack_from('<I', original, 4 + 28 * 4)[0]
        stripped = original[:offset] + bytes(16)
        with self.assertRaises(ValueError):
            unpack(stripped)
        recovered, flags = unpack_model(stripped)
        self.assertEqual(recovered[:28], entries[:28])
        self.assertEqual(recovered[28:], [b''] * 24)
        self.assertEqual(len(flags), 52)
        with self.assertRaises(ValueError):
            unpack_model(stripped[:-1] + b'x')
        chain_offset = struct.unpack_from('<I', original, 4 + 22 * 4)[0]
        with self.assertRaises(ValueError):
            unpack_model(original[:chain_offset] + bytes(16))

    def test_full_catalogue_uses_real_bt4_identities_above_legacy_limit(self):
        table = bytearray(250 * 60)
        for i in (0, 160, 161, 200, 249):
            struct.pack_into('<H', table, i * 60 + 40, 1)
            table[i * 60 + 10] = 1
        table[161 * 60 + 10] = 2  # the Random/menu sentinel has no such costume
        roster = {0: 'Base', 160: 'Legacy boundary', 161: 'Random',
                  200: 'BT4 fighter', 231: '', 249: 'Last fighter'}
        self.assertEqual(playable_fighters(table, roster), [0, 160, 200, 249])

    def test_unrequested_aliases_preserve_physical_offsets(self):
        table = bytearray(250 * 60)
        for i, (physical, selectable) in enumerate(((2, 2), (1, 2), (3, 2))):
            struct.pack_into('<H', table, i * 60 + 40, physical)
            table[i * 60 + 10] = selectable
        families = costume_families(table, {0, 2})
        self.assertEqual(families[2], (5656, 3, 2))
        with self.assertRaises(ValueError):
            costume_families(table, {1})
        struct.pack_into('<H', table, 40, 17)
        with self.assertRaises(ValueError):
            costume_families(table, {2})

    def test_embeds_metadata_preserving_geometry_and_language_streams(self):
        entries = [b''] * 52
        for slot in (1, 2, 3, 11, 22, 25, 35, 51):
            entries[slot] = bytes([slot]) * 64
        model = pack(entries)
        metadata = [b'model-header' * 64, bytes(range(256)) * 48 + bytes(208),
                    pack([b'AI0' * 64, b'AI1' * 64, b'AI2' * 64])]
        self.assertEqual(len(metadata[1]), 0x30D0)
        text = b'\xff\xfe' + 'Technique\n'.encode('utf-16le')
        lips = [unpack(pack([bytes([i]) * 64 for i in range(100)]))
                for _ in range(2)]
        result, flags = unpack(embed(model, metadata, text, *lips))
        self.assertEqual(len(result), 253)
        self.assertEqual(len(flags), 253)
        for slot in (1, 2, 3, 11, 22, 25, 35):
            self.assertEqual(result[slot], entries[slot])
        self.assertEqual(result[17], metadata[1][:256])
        self.assertEqual(result[18], metadata[1][0x100:0x2240])
        # Non-aligned records have zero pack padding, never a truncated payload.
        self.assertEqual(result[27][:0x180], metadata[1][0x2F50:0x30D0])
        self.assertEqual(result[44][:len(text)], text)
        self.assertEqual(result[45:49], [b''] * 4)
        self.assertTrue(result[252].startswith(b'BT4WARRI'))
        self.assertEqual(result[52:152], lips[0][0])
        self.assertEqual(result[152:252], lips[1][0])
        self.assertEqual(result[49:52], unpack(metadata[2])[0])
        # A legacy embedded costume must use current BT4 metadata while keeping
        # the same geometry, just like the normal external-metadata binding.
        legacy = entries + [b'legacy lip data'] * 200
        converted, _ = unpack(embed(pack(legacy), metadata, text, *lips))
        self.assertEqual(converted, result)

    def test_lip_table_without_end_marker(self):
        data = bytearray(pack([b'a' * 64, b'b' * 64]))
        struct.pack_into('<I', data, 12, 8)
        with self.assertRaises(ValueError):
            unpack(data)
        entries, flags = unpack(data, terminal=False)
        self.assertEqual(entries, [b'a' * 64, b'b' * 64])
        self.assertEqual(flags, [0, 0])
        struct.pack_into('<I', data, 8, 4)
        with self.assertRaises(ValueError):
            unpack(data, terminal=False)

    def test_rejects_invalid_models_tables_and_metadata(self):
        for data in (b'', bytes(12), struct.pack('<I', 4097) + bytes(16)):
            with self.assertRaises(ValueError):
                unpack(data)
        with self.assertRaises(ValueError):
            pack([b'x'], [4])
        with self.assertRaises(ValueError):
            embed(pack([b'x']), [], b'', ([], []), ([], []))


if __name__ == '__main__':
    unittest.main()
