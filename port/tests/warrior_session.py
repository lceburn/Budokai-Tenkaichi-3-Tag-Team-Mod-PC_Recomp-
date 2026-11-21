"""Build and exercise temporary room registration with generated fixture files."""
from pathlib import Path
import hashlib,os,subprocess,tempfile,struct
ROOT=Path(__file__).resolve().parents[2]
RESOURCES=['Fighter_1p.pak','Fighter_1p_dmg.pak','Fighter_anm.pak',
           'Fighter_eff.pak','pzs3us1/00001.bin','pzs3us1/00002.bin','Fighter_img.cdbt']
def resource_pack(entries):
    table=(len(entries)+2)*4;out=bytearray(table);struct.pack_into('<I',out,0,len(entries))
    for i,entry in enumerate(entries):
        struct.pack_into('<I',out,4+i*4,len(out));out.extend(entry)
        out.extend(bytes((-len(out))%4))
    struct.pack_into('<I',out,4+len(entries)*4,len(out));return bytes(out)
texture=bytearray(192);struct.pack_into('<II',texture,0,1,8)
struct.pack_into('<III',texture,32,24,0,96)
mesh=bytearray(208);struct.pack_into('<I',mesh,0,0x6c646d70);struct.pack_into('<I',mesh,0x6c,144)
struct.pack_into('<IHH',mesh,144,64,0,1)
model=resource_pack([bytes(256),b'',bytes(mesh)]+[b'']*8+[bytes(texture)]+[b'']*240)
animation=resource_pack([bytes(148)]+[b'']*439)
effects=resource_pack([b'']*6)
portrait=struct.pack('<II',0x60,8)+bytes(8)
def resource(name):
    if name.endswith('.bin'):return bytes(texture)
    if name.endswith('.cdbt'):return portrait
    if '_anm.' in name:return animation
    if '_eff.' in name:return effects
    return model
with tempfile.TemporaryDirectory() as work:
    root=Path(work);exe=root/'session-test'
    objects=[]
    for source in ('port/tests/warrior_session.c','port/src/plat_warriors.c','port/src/plat_game.c',
                   'port/src/gs/warrior_net_transfer.c','port/src/gs/warrior_net_catalog.c',
                   'port/third_party/monocypher/monocypher.c'):
        obj=root/(Path(source).stem+'.o');objects.append(str(obj))
        subprocess.run(['cc','-std=gnu99','-g','-fsanitize=address,undefined','-I'+str(ROOT/'port/include'),'-c',str(ROOT/source),'-o',str(obj)],check=True)
    subprocess.run(['c++','-std=c++17','-g','-fsanitize=address,undefined','-I'+str(ROOT/'port/include'),
                    str(ROOT/'port/src/gs/warrior_net_pack.cpp'),*objects,'-o',str(exe)],check=True)
    (root/'native-disc.json').write_text('{"game":"bt4","revision":"b14-rev2"}')
    table=bytearray(8+250*60);struct.pack_into('<I',table,4,8)
    for i in range(250):struct.pack_into('<H',table,8+i*60+40,1)
    (root/'pzs4us1').mkdir();(root/'pzs4us1/00003.bin').write_bytes(table)
    (root/'native-roster.tsv').write_text('0\tGoku\t\n')
    def fighter(folder,name,target=None):
        folder.mkdir(parents=True)
        (folder/'.warrior_built').write_text('#warrior 47\n'+'\n'.join(RESOURCES)+'\n')
        (folder/'warrior.txt').write_text('name='+name+'\nstyle=Goku\nvoice=Goku\n')
        (folder/'mod.txt').write_text('mode=new\n'+('trans0=1,0,2,-1,0,@'+target+'\n' if target else ''))
        for name in RESOURCES:
            p=folder/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(resource(name))
    fighter(root/'mods/Local','Local');(root/'mods/enabled.txt').write_text('Local\n')
    for i in range(19):fighter(root/f'.warrior-sessions/probe/form{i:02d}','Remote',f'form{(i+1)%19:02d}')
    (root/'.warrior-sessions/probe/roots.txt').write_text(''.join(f'.warrior-sessions/probe/form{i:02d}\n' for i in range(19)))
    before={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in (root/'mods').rglob('*') if p.is_file()}
    subprocess.run([str(exe)],env=os.environ|{'BT3_DATA':str(root),'BT3_WARRIOR_MODELS':'1'},check=True)
    after={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in (root/'mods').rglob('*') if p.is_file()}
    assert before==after
    print('PASS: saved local library remained byte-identical')
    subprocess.run([str(exe)],env=os.environ|{'BT3_DATA':str(root),'BT3_WARRIOR_MODELS':'1',
                    'BT3_WARRIOR_SESSION_TEST':'.warrior-sessions/probe/roots.txt','SESSION_BOOTSTRAP':'1'},check=True)
    fighter(root/'mods/A','A','B');fighter(root/'mods/B','B','A');fighter(root/'mods/Unused','Unused')
    (root/'mods/enabled.txt').write_text('A\nB\nUnused\n')
    before={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in (root/'mods').rglob('*') if p.is_file()}
    subprocess.run([str(exe)],env=os.environ|{'BT3_DATA':str(root),'BT3_WARRIOR_MODELS':'1','PACK_CLOSURE':'1'},check=True)
    assert before=={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in (root/'mods').rglob('*') if p.is_file()}
