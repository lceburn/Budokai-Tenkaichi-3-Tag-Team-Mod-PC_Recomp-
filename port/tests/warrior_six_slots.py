"""Run the six-slot router/export test using synthetic resources only."""
from pathlib import Path
import os,struct,subprocess,sys,tempfile

def pack(entries):
    data=bytearray((len(entries)+2)*4);struct.pack_into('<I',data,0,len(entries))
    for i,entry in enumerate(entries):
        struct.pack_into('<I',data,4+i*4,len(data));data.extend(entry);data.extend(bytes((-len(data))%4))
    struct.pack_into('<I',data,4+len(entries)*4,len(data));return bytes(data)

with tempfile.TemporaryDirectory(prefix='warrior-six-') as directory:
    root=Path(directory);(root/'mods').mkdir();(root/'pzs4us1').mkdir()
    (root/'native-disc.json').write_text('{"game":"bt4","revision":"b14-rev2","language":"en"}')
    table=bytearray(8+250*60);struct.pack_into('<II',table,0,1,8)
    for i in range(250):struct.pack_into('<H',table,8+i*60+40,1)
    (root/'pzs4us1/00003.bin').write_bytes(table);(root/'native-roster.tsv').write_text('0\tGoku\t\n')
    texture=bytearray(192);struct.pack_into('<II',texture,0,1,8);struct.pack_into('<III',texture,32,24,0,96)
    mesh=bytearray(208);struct.pack_into('<I',mesh,0,0x6c646d70);struct.pack_into('<I',mesh,0x6c,144);struct.pack_into('<IHH',mesh,144,64,0,1)
    model=pack([bytes(256),b'',mesh]+[b'']*8+[texture]+[b'']*240)
    files={'Fighter_1p.pak':model,'Fighter_1p_dmg.pak':model,'Fighter_anm.pak':pack([bytes(148)]+[b'']*439),
           'Fighter_eff.pak':pack([b'']*6),'Fighter_img.cdbt':struct.pack('<II',0x60,8)+bytes(8),
           'pzs3us1/00001.bin':texture,'pzs3us1/00002.bin':texture}
    for i in range(6):
        folder=root/'mods'/f'Form{i}';folder.mkdir()
        (folder/'warrior.txt').write_text(f'name=Form{i}\nstyle=Goku\nvoice=Goku\n')
        (folder/'mod.txt').write_text(f'mode=new\ntrans0=1,0,2,-1,0,@Form{(i+1)%6}\n')
        (folder/'.warrior_built').write_text('#warrior 47\n'+'\n'.join(files)+'\n')
        for name,content in files.items():
            p=folder/name;p.parent.mkdir(exist_ok=True);p.write_bytes(content)
    (root/'mods/enabled.txt').write_text(''.join(f'Form{i}\n' for i in range(6)))
    subprocess.run([sys.argv[1]],env=os.environ|{'BT3_DATA':str(root),'BT3_WARRIOR_MODELS':'1'},check=True)
