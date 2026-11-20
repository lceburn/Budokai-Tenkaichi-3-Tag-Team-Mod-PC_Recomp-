"""Exercise staging, dependency rewriting and failed-publish rollback without game assets."""
import hashlib
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import build_warrior_draft as builder


class BuilderTransactions(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name);self.mods=self.root/"mods";self.mods.mkdir()
        self.drafts=self.root/"drafts";self.drafts.mkdir()
        self.job=self.root/"job";self.stage=self.job/"publish";self.stage.mkdir(parents=True)

    def folder(self,root,name,extra=""):
        folder=root/name;folder.mkdir()
        (folder/"warrior.txt").write_text("name="+name+"\nbody=Goku (End)\nmoves=Goku (End)\n"+extra)
        return folder

    def test_cyclic_form_family_collects_once(self):
        self.folder(self.drafts,"Base","form1=1,0,Form\n")
        self.folder(self.mods,"Form","form1=1,0,Base\n")
        result=builder.family(self.drafts,self.mods,["Base"])
        self.assertEqual(list(result),["Base","Form"])

    def test_preview_never_builds_form_dependencies_or_changes_draft(self):
        source=self.folder(self.drafts,'Preview','form1=1,0,Missing\nhead=Kid Goku\ncolour=123,4,5,6\n')
        before=(source/'warrior.txt').read_bytes()
        result=builder.draft_inputs(self.drafts,self.mods,['Preview'],True)
        self.assertEqual(list(result),['Preview'])
        self.assertNotIn(b'form1=',result['Preview'][1])
        self.assertEqual(result['Preview'][1],b''.join(line for line in before.splitlines(keepends=True) if not line.startswith(b'form1=')))
        self.assertEqual((source/'warrior.txt').read_bytes(),before)

    def test_preview_rejects_multiple_targets(self):
        with self.assertRaises(ValueError):
            builder.draft_inputs(self.drafts,self.mods,['Base','Form'],True)

    def test_form_namespace_round_trip(self):
        raw=b"# keep\r\nform1=1,0,Other\r\ncolour=1,2,3,4\r\n"
        changed=builder.rewrite_links(raw,{"Other":"Job-1"})
        self.assertEqual(changed,b"# keep\r\nform1=1,0,Job-1\r\ncolour=1,2,3,4\r\n")
        generated=b"mode=new\ntrans0=1,0,2,-1,0,@Job-1\ntrans1=1,0,2,-1,0,-1\n"
        self.assertEqual(builder.rewrite_links(generated,{"Job-1":"Other"},True),
                         generated.replace(b"@Job-1",b"@Other"))

    def test_unknown_dependency_cannot_escape_staging(self):
        with self.assertRaises(ValueError):builder.rewrite_links(b"form1=1,0,Missing\n",{})
        for name in ("../outside","C:/outside","nested/folder","bad\\folder","trailing."):
            with self.subTest(name=name),self.assertRaises(ValueError):builder.component(name)

    def test_success_retains_previous_folder(self):
        old=self.folder(self.mods,"Base");old_bytes=(old/"warrior.txt").read_bytes()
        new=self.folder(self.stage,"Base","voice=Nappa\n")
        (self.mods/"enabled.txt").write_bytes(b"# original\r\nBase\r\n")
        before=(self.mods/"enabled.txt").read_bytes()
        snapshots={"Base":builder.digest(old/"warrior.txt")}
        builder.publish(self.stage,self.mods,self.job/"previous",["Base"],snapshots,before,before)
        self.assertIn(b"voice=Nappa",(old/"warrior.txt").read_bytes())
        self.assertEqual((self.job/"previous/Base/warrior.txt").read_bytes(),old_bytes)
        self.assertEqual((self.mods/"enabled.txt").read_bytes(),before)

    def test_mid_publish_failure_restores_entire_family(self):
        snapshots={};originals={}
        for name in ("Base","Other"):
            old=self.folder(self.mods,name);self.folder(self.stage,name,"voice=Nappa\n")
            originals[name]=(old/"warrior.txt").read_bytes();snapshots[name]=builder.digest(old/"warrior.txt")
        enabled=b"Base\nOther\n";(self.mods/"enabled.txt").write_bytes(enabled)
        replace=builder.os.replace
        def fail_second(source,target):
            if Path(source)==self.stage/"Other":raise OSError("simulated publish failure")
            return replace(source,target)
        with patch.object(builder.os,"replace",side_effect=fail_second),self.assertRaises(OSError):
            builder.publish(self.stage,self.mods,self.job/"previous",["Base","Other"],snapshots,enabled,enabled)
        for name in originals:self.assertEqual((self.mods/name/"warrior.txt").read_bytes(),originals[name])
        self.assertEqual((self.mods/"enabled.txt").read_bytes(),enabled)

    def test_external_edit_refuses_publish(self):
        old=self.folder(self.mods,"Base");self.folder(self.stage,"Base")
        snapshot=builder.digest(old/"warrior.txt");(old/"warrior.txt").write_bytes(b"external edit")
        with self.assertRaises(ValueError):
            builder.publish(self.stage,self.mods,self.job/"previous",["Base"],{"Base":snapshot},None,b"Base\n")
        self.assertEqual((old/"warrior.txt").read_bytes(),b"external edit")


if __name__=="__main__":unittest.main()
