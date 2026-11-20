"""Synthetic checks for the static warrior preflight; no game assets required."""
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import inspect_warrior as warrior


def fixture(path, folder="Test Warrior", target="Other Warrior", missing=False):
    with zipfile.ZipFile(path, "w") as archive:
        archive.writestr(folder + "/warrior.txt", "name=Synthetic Fighter\n")
        archive.writestr(folder + "/mod.txt", f"mode=new\ntemplate=52\ntrans0=1,0,2,-1,0,@{target}\n")
        archive.writestr(folder + "/.warrior_built", "#warrior 47\nmodel.pak\n")
        if not missing:
            archive.writestr(folder + "/model.pak", struct.pack("<4I", 1, 12, 16, 123))


class WarriorInspectionTests(unittest.TestCase):
    def test_pair_resolves_folder_references_without_display_names(self):
        with tempfile.TemporaryDirectory() as directory:
            base, form = [Path(directory) / name for name in ("base.zip", "form.zip")]
            fixture(base)
            fixture(form, "Other Warrior", "Test Warrior")
            result = warrior.inspect_set([base, form])
            self.assertFalse(result["installed"])
            for report in result["warriors"]:
                self.assertEqual(report["unresolved_transformation_folders"], [])
                self.assertEqual(report["runtime_compatibility"], "unverified")
                self.assertEqual(report["resources"][0]["pack"]["entries"], 1)
            json.dumps(result)

    def test_missing_form_is_reported(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "base.zip"
            fixture(source)
            self.assertEqual(warrior.inspect_set([source])["warriors"][0]
                             ["unresolved_transformation_folders"], ["Other Warrior"])

    def test_missing_generated_resource_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "base.zip"
            fixture(source, missing=True)
            with self.assertRaisesRegex(ValueError, "Missing generated resource"):
                warrior.inspect(source)

    def test_archive_traversal_is_rejected_without_extraction(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "unsafe.zip"
            with zipfile.ZipFile(source, "w") as archive:
                archive.writestr("Warrior/../outside.txt", "unused")
            with self.assertRaisesRegex(ValueError, "Unsafe"):
                warrior.inspect(source)
            self.assertFalse((Path(directory) / "outside.txt").exists())

    def test_case_colliding_paths_are_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "duplicate.zip"
            with zipfile.ZipFile(source, "w") as archive:
                archive.writestr("Warrior/model.pak", b"a")
                archive.writestr("Warrior/MODEL.pak", b"b")
            with self.assertRaisesRegex(ValueError, "Duplicate resource"):
                warrior.inspect(source)

    def test_pack_rejects_offsets_outside_payload(self):
        for offset in (4, 1024):
            with self.subTest(offset=offset), self.assertRaisesRegex(ValueError, "outside"):
                warrior.pack_summary(struct.pack("<4I", 1, offset, 16, 0))

    def test_pack_masks_flags_and_accepts_shared_entries(self):
        data = struct.pack("<6I", 3, 20 | 1, 24, 20 | 2, 24, 0)
        result = warrior.pack_summary(data, model=True)
        self.assertEqual(result["embedded_sections"]["relocatable_model_metadata"], 4)
        self.assertEqual(result["embedded_sections"]["body"], 0)
        self.assertEqual(result["embedded_sections"]["render_model"], 4)


if __name__ == "__main__":
    unittest.main()
