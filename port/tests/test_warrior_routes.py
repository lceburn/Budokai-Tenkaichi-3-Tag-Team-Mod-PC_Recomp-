"""Run the compiled routing harness with synthetic folders, never game assets.

Set WARRIOR_ROUTES_TEST_EXE to the compiled warrior_routes.c harness.
"""
import os
from pathlib import Path
import subprocess
import tempfile
import shutil
import unittest

RESOURCES = ["Fighter_1p.pak", "Fighter_2p.pak", "Fighter_1p_dmg.pak",
             "Fighter_2p_dmg.pak", "Fighter_anm.pak", "Fighter_eff.pak",
             "pzs3us1/00001.bin", "pzs3us1/00002.bin", "Fighter_img.cdbt"]


@unittest.skipUnless(os.environ.get("WARRIOR_ROUTES_TEST_EXE"), "Build the native routing harness first")
class WarriorRoutingTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        (self.root / "original.bin").write_bytes(b"abc")
        for name in ("Base Warrior", "Other Warrior"):
            folder = self.root / "mods" / name
            folder.mkdir(parents=True)
            (folder / ".warrior_built").write_text("#warrior 47\n" + "\n".join(RESOURCES) + "\n")
            (folder / "warrior.txt").write_text("name=Display Name\nstyle=Template Fighter\nvoice=Voice Fighter\n")
            other = "Other Warrior" if name == "Base Warrior" else "Base Warrior"
            (folder / "mod.txt").write_text(f"mode=new\ntrans0=1,0,2,-1,0,@{other}\n")
            for resource in RESOURCES:
                path = folder / resource
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b"abc")
        self.enabled = self.root / "mods/enabled.txt"
        self.enabled.write_text("\nBase Warrior\n\nOther Warrior\n")

    def check(self, count, error=False, enabled=True,preview=None,attack_names=False,aura_colour=None,bt4_rules=False):
        env = os.environ | {"BT3_DATA": str(self.root), "BT3_WARRIOR_MODELS": "1" if enabled else "0"}
        if preview:env['WARRIOR_ROUTES_PREVIEW']=preview
        if attack_names:env['WARRIOR_ATTACK_NAMES']='1'
        if aura_colour is not None:env['WARRIOR_AURA_COLOUR']=aura_colour
        if bt4_rules:env['WARRIOR_BT4_RULES']='1'
        result = subprocess.run([os.environ["WARRIOR_ROUTES_TEST_EXE"], str(count), str(int(error))],
                                env=env, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("PASS:", result.stdout)

    def test_both_folders_route_all_nine_resources(self):
        self.check(2)

    def test_aura_colour_is_scoped_and_preserves_black(self):
        path=self.root/'mods/Base Warrior/warrior.txt';before=path.read_text()
        for value,packed in [('12abef','efab12'),('000000','0')]:
            path.write_text(before+'aura='+value+'\n');self.check(2,aura_colour=packed)

    def test_malformed_aura_colours_are_rejected(self):
        path=self.root/'mods/Base Warrior/warrior.txt';before=path.read_text()
        for value in ('abc','1234567','12gg00','-12345','0x1234',''):
            with self.subTest(value=value):path.write_text(before+'aura='+value+'\n');self.check(0,error=True)

    def test_duplicate_aura_colour_is_rejected(self):
        path=self.root/'mods/Base Warrior/warrior.txt';path.write_text(path.read_text()+'aura=123456\naura=abcdef\n');self.check(0,error=True)

    def test_attack_names_are_optional_and_scoped_to_their_warrior(self):
        path=self.root/'mods/Base Warrior/warrior.txt'
        path.write_text(path.read_text()+'blast1_1name=First custom attack\nultimatename=Last custom attack\n')
        self.check(2,attack_names=True)

    def test_duplicate_attack_name_is_rejected(self):
        path=self.root/'mods/Base Warrior/warrior.txt'
        path.write_text(path.read_text()+'blast1_1name=One\nblast1_1name=Two\n')
        self.check(0,error=True)

    def test_overlong_attack_name_is_rejected(self):
        path=self.root/'mods/Base Warrior/warrior.txt'
        path.write_text(path.read_text()+'ultimatename='+32*'x'+'\n')
        self.check(0,error=True)

    def test_preview_binding_restores_playable_library(self):
        folder=self.root/'.warrior-builds/test/Preview'
        shutil.copytree(self.root/'mods/Base Warrior',folder)
        (folder/'warrior.txt').write_text('name=Preview Name\nstyle=Template Fighter\nvoice=Voice Fighter\n')
        (self.root/'.warrior-preview-1-path.txt').write_text('.warrior-builds/test/Preview\n')
        before=self.enabled.read_bytes();self.check(2,preview='valid')
        self.assertEqual(self.enabled.read_bytes(),before)

    def test_preview_rejects_path_outside_private_builds(self):
        (self.root/'.warrior-preview-1-path.txt').write_text('.warrior-builds/../../mods/Base Warrior\n')
        self.check(2,preview='invalid')

    def test_preview_missing_resources_preserves_registry(self):
        (self.root/'.warrior-preview-1-path.txt').write_text('.warrior-builds/absent/Preview\n')
        self.check(2,preview='invalid')

    def costumes(self, count):
        for name in ("Base Warrior", "Other Warrior"):
            folder = self.root / "mods" / name
            resources = [r for r in RESOURCES if count > 1 or not r.endswith(("_2p.pak", "_2p_dmg.pak"))]
            for costume in range(3, count + 1):
                for suffix in (f"_{costume}p.pak", f"_{costume}p_dmg.pak"):
                    resource = "Fighter" + suffix
                    resources.append(resource)
                    (folder / resource).write_bytes(b"abc")
            (folder / ".warrior_built").write_text("#warrior 47\n" + "\n".join(resources) + "\n")

    def test_single_costume(self):
        self.costumes(1)
        self.check(2)

    def test_default_creator_three_costumes_and_inherited_donors(self):
        self.costumes(3)
        for name in ("Base Warrior", "Other Warrior"):
            (self.root / "mods" / name / "warrior.txt").write_text(
                "name=New Warrior\nbody=Goku (End)\nmoves=Goku (End)\n")
        self.check(2)

    def test_four_costumes(self):
        self.costumes(4)
        self.check(2)

    def test_unpaired_extra_costume_rejected(self):
        self.costumes(3)
        folder = self.root / "mods/Base Warrior"
        manifest = folder / ".warrior_built"
        manifest.write_text(manifest.read_text().replace("Fighter_3p_dmg.pak\n", ""))
        self.check(0, error=True)

    def test_gap_in_costumes_rejected(self):
        self.costumes(4)
        folder = self.root / "mods/Base Warrior"
        manifest = folder / ".warrior_built"
        manifest.write_text(manifest.read_text().replace("Fighter_3p.pak\n", "").replace("Fighter_3p_dmg.pak\n", ""))
        self.check(0, error=True)

    def test_disabled_by_default(self):
        self.check(0, enabled=False)

    def test_absent_enable_list_registers_nothing(self):
        self.enabled.unlink()
        self.check(0)

    def test_missing_resource_rejects_entire_set(self):
        (self.root / "mods/Other Warrior/Fighter_anm.pak").unlink()
        self.check(0, error=True)

    def test_oversized_or_empty_model_rejects_before_loading(self):
        path = self.root / 'mods/Base Warrior/Fighter_1p.pak'
        for size in (0, 0x200001):
            with path.open('wb') as stream:
                stream.truncate(size)
            self.check(0, error=True)

    def test_duplicate_folder_identity_is_rejected(self):
        self.enabled.write_text("Base Warrior\nbase warrior\n")
        self.check(0, error=True)

    def test_traversal_in_enable_list_is_rejected(self):
        self.enabled.write_text("../outside\n")
        self.check(0, error=True)

    def test_traversal_in_build_list_is_rejected(self):
        (self.root / "mods/Base Warrior/.warrior_built").write_text("#warrior 47\n../outside.pak\n")
        self.check(0, error=True)

    def test_ambiguous_resource_role_is_rejected(self):
        folder = self.root / "mods/Base Warrior"
        with (folder / ".warrior_built").open("a") as output:
            output.write("Other_1p.pak\n")
        (folder / "Other_1p.pak").write_bytes(b"abc")
        self.check(0, error=True)

    def test_overlong_line_is_rejected(self):
        self.enabled.write_text("a" * 300 + "\n")
        self.check(0, error=True)

    def test_missing_form_is_rejected(self):
        self.enabled.write_text("Base Warrior\n")
        self.check(0, error=True)

    def test_bt4_rules_are_scoped_to_one_saved_warrior(self):
        with (self.root / 'mods/Base Warrior/mod.txt').open('a') as stream:
            stream.write('rules=bt4\n')
        self.check(2, bt4_rules=True)

    def test_invalid_or_duplicate_rules_are_rejected(self):
        path = self.root / 'mods/Base Warrior/mod.txt'
        original = path.read_text()
        for suffix in ('rules=other\n', 'rules=bt4\nrules=bt3\n'):
            path.write_text(original + suffix)
            self.check(0, error=True)

    def test_invalid_mode_is_rejected(self):
        (self.root / "mods/Base Warrior/mod.txt").write_text("mode=replace\n")
        self.check(0, error=True)

    def test_malformed_transformation_is_rejected(self):
        (self.root / "mods/Base Warrior/mod.txt").write_text("mode=new\ntrans0=1,2,@Other Warrior\n")
        self.check(0, error=True)


if __name__ == "__main__":
    unittest.main()
