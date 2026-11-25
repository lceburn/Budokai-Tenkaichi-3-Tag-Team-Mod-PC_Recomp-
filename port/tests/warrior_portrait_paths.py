from pathlib import Path
import tempfile,os,subprocess,sys
with tempfile.TemporaryDirectory()as td:
 root=Path(td);room='.warrior-sessions/'+'/'.join(('a'*100,'b'*100,'c'*100))
 resources=['Fighter_1p.pak','Fighter_2p.pak','Fighter_1p_dmg.pak','Fighter_2p_dmg.pak','Fighter_anm.pak','Fighter_eff.pak','pzs3us1/00001.bin','pzs3us1/00002.bin','Fighter_img.cdbt']
 for name in ('mods/Local','.warrior-builds/probe/Preview',room):
  folder=root/name;folder.mkdir(parents=True)
  (folder/'.warrior_built').write_text('#warrior 47\n'+'\n'.join(resources)+'\n')
  (folder/'warrior.txt').write_text('name=Portrait Probe\nstyle=Template Fighter\nvoice=Voice Fighter\n')
  (folder/'mod.txt').write_text('mode=new\n')
  for name in resources:
   p=folder/name;p.parent.mkdir(exist_ok=True,parents=True);p.write_bytes(b'abc')
 (root/'mods/enabled.txt').write_text('Local\n');(root/'.warrior-preview-1-path.txt').write_text('.warrior-builds/probe/Preview\n')
 subprocess.run([sys.argv[1],room],env=os.environ|{'BT3_DATA':str(root)},check=True)
