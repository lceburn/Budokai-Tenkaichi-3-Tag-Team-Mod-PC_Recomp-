"""Atomic, local-disc-only creator cache setup and upgrade preparation."""
from pathlib import Path
import json,sys,tempfile,unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import native_creator_install as creator
class CreatorInstall(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.data=Path(self.temp.name)/'gamedata';self.data.mkdir()
        self.profile={'game':'bt4','revision':'b14-rev2','language':'en','executable_sha256':'disc'}
        (self.data/'native-disc.json').write_text(json.dumps(self.profile))
        (self.data/'player-data').write_text('preserve')
    def fake_prepare(self,source,dest,progress=None):
        self.assertEqual(source,self.data);dest.mkdir()
        for name in ('index.tsv','assets.tsv','provenance.json'):(dest/name).write_text('test')
    def ensure(self):
        with patch.object(creator,'prepare',side_effect=self.fake_prepare),patch.object(creator.shutil,'disk_usage',return_value=type('Space',(),{'free':9*1024**3})()):
            return creator.ensure(self.data,progress=lambda _:None)
    def test_atomic_success_and_reuse(self):
        self.assertTrue(self.ensure());cache=self.data/'warrior-sources'
        self.assertTrue((cache/creator.MARKER).is_file())
        with patch.object(creator,'prepare',side_effect=AssertionError('must reuse')):
            self.assertTrue(creator.ensure(self.data,progress=lambda _:None))
        self.assertEqual((self.data/'player-data').read_text(),'preserve')
        self.assertFalse(list(self.data.glob('.creator-sources-*')))
    def test_failure_never_publishes_partial_cache(self):
        def fail(source,dest,progress=None):
            dest.mkdir();(dest/'partial').write_text('partial');raise ValueError('injected')
        with patch.object(creator,'prepare',side_effect=fail),patch.object(creator.shutil,'disk_usage',return_value=type('Space',(),{'free':9*1024**3})()):
            with self.assertRaisesRegex(ValueError,'injected'):creator.ensure(self.data,progress=lambda _:None)
        self.assertFalse((self.data/'warrior-sources').exists());self.assertFalse(list(self.data.glob('.creator-sources-*')))
        self.assertEqual((self.data/'player-data').read_text(),'preserve')
    def test_existing_unowned_cache_preserved(self):
        cache=self.data/'warrior-sources';cache.mkdir();(cache/'mine').write_text('keep')
        with self.assertRaisesRegex(ValueError,'unrecognized'):self.ensure()
        self.assertEqual((cache/'mine').read_text(),'keep')
    def test_different_disc_not_silently_reused(self):
        self.ensure();self.profile['language']='es';(self.data/'native-disc.json').write_text(json.dumps(self.profile))
        with self.assertRaisesRegex(ValueError,'different disc'):self.ensure()
    def test_disabled_and_bt3_do_not_prepare(self):
        self.assertFalse(creator.ensure(self.data/'missing',enabled=False))
        (self.data/'native-disc.json').write_text(json.dumps({'game':'bt3'}))
        with patch.object(creator,'prepare',side_effect=AssertionError('not BT3')):
            self.assertFalse(creator.ensure(self.data))
if __name__=='__main__':unittest.main()
