"""Release channel, signature, path and rollback protections for native auto updates."""
import hashlib
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
import zipfile

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import native_auto_update as auto
import update_key


class AutoUpdateTests(unittest.TestCase):
    def test_native_downloader_requires_https_without_embedded_credentials(self):
        for url in ('http://github.com/file','https://user:password@github.com/file','https://github.com:80/file'):
            with self.subTest(url=url),self.assertRaises(ValueError):auto.fetch_windows(url,100)

    def release(self, tag='v0.1.0-beta.8-experimental.1'):
        name='Tag-Team-Recomp-Beta.8-Experimental-Updater-Windows.zip'
        return {'tag_name':tag,'assets':[{'name':n,'size':100,'browser_download_url':auto.PREFIX+tag+'/'+n} for n in (name,name+'.sig')]}

    def test_only_newer_signed_same_channel_own_assets(self):
        current={'version':'beta.7-experimental','release_sequence':1}
        release=self.release()
        self.assertIsNotNone(auto.choose_release([release],current))
        self.assertIsNone(auto.choose_release([release],{'version':'beta.8-experimental','release_sequence':1}))
        self.assertIsNone(auto.choose_release([release],{'version':'beta.7'}))
        for change in ('unsigned','draft','foreign'):
            item=self.release()
            if change=='unsigned':item['assets'].pop()
            if change=='draft':item['draft']=True
            if change=='foreign':item['assets'][0]['browser_download_url']='https://example.com/evil.zip'
            self.assertIsNone(auto.choose_release([item],current))

    def archive(self,extra=None):
        files={'Play.exe':b'play','engine/TagTeam.exe':b'engine','engine/TagTeam.exe.mem':b'mem',
               'engine/TagTeam.dat':b'tables','engine/SDL3.dll':b'sdl',
               'release.json':json.dumps({'version':'beta.8-experimental'}).encode()}
        manifest={n:hashlib.sha256(data).hexdigest() for n,data in files.items()}
        files['manifest.json']=json.dumps(manifest).encode()
        out=io.BytesIO()
        with zipfile.ZipFile(out,'w') as z:
            for n,data in files.items():z.writestr('payload/'+n,data)
            if extra:
                entry=zipfile.ZipInfo('entry');entry.filename=extra
                z.writestr(entry,b'escape')
        return out.getvalue()

    def test_signed_archive_and_corruption(self):
        seed=bytes(range(32));public=update_key.public_key(seed);data=self.archive()
        signature=update_key.sign(seed,data).hex().encode()
        with tempfile.TemporaryDirectory() as folder,patch.object(auto,'PUBLIC_KEY',public),patch.object(auto.setup,'platform_name',return_value='windows'):
            payload=auto.verified_payload(data,signature,Path(folder),len(data),'sha256:'+hashlib.sha256(data).hexdigest())
            self.assertTrue((payload/'engine/TagTeam.exe').is_file())
            for broken in (data[:-1],data[:-1]+b'!'):
                with self.assertRaises(ValueError):auto.verified_payload(broken,signature,Path(folder),len(data))
            with self.assertRaisesRegex(ValueError,'checksum'):auto.verified_payload(data,signature,Path(folder),len(data),'sha256:wrong')

    def test_even_signed_path_traversal_is_rejected(self):
        seed=bytes(range(32))
        with tempfile.TemporaryDirectory() as folder,patch.object(auto,'PUBLIC_KEY',update_key.public_key(seed)):
            for path in ('payload/../../escape','payload\\escape','/payload/escape','payload/engine/C:escape'):
                data=self.archive(path)
                with self.subTest(path=path),self.assertRaisesRegex(ValueError,'path'):auto.verified_payload(data,update_key.sign(seed,data).hex().encode(),Path(folder),len(data))


if __name__=='__main__':unittest.main()
