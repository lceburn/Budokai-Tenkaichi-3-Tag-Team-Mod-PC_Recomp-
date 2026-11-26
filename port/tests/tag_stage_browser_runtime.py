"""Exercise real stage previews and controller confirmation in an isolated room."""
from pathlib import Path
import os,subprocess,ctypes,struct,time,json,re
from ctypes import wintypes
import argparse
p=argparse.ArgumentParser(description="Windows live stage browsing, preview lifetime and explicit confirmation regression. Supply matching engine/map and nm dumps of the executable and menu object.")
p.add_argument('--engine',type=Path,required=True);p.add_argument('--data',type=Path,required=True)
p.add_argument('--symbols',type=Path,required=True);p.add_argument('--menu-symbols',type=Path,required=True)
p.add_argument('--port',type=int,default=7099);p.add_argument('--output',type=Path)
a=p.parse_args();r=Path(__file__).resolve().parents[2]
if os.name!='nt':raise SystemExit('This live process-memory driver requires Windows.')
out=a.output or r/'port/build/stage-browser-runtime';out.mkdir(parents=True,exist_ok=False)
exe=a.engine.resolve();maptext=Path(str(exe)+'.map').read_text(errors='replace')
base=int(re.search(r' \.bss\s+0x([0-9a-f]+)\s+0x[0-9a-f]+ \S*/menu_tag_online_menu\.o',maptext)[1],16)
nm=a.symbols.read_text();symbols={name:int(re.search(r'^([0-9a-fA-F]+)\s+\w\s+'+name+r'$',nm,re.M)[1],16) for name in ('sLowNext','sLowSizes','gGsFrame')}
env={k:v for k,v in os.environ.items()if not k.startswith('BT3_')};env.update(BT3_DATA=str(a.data.resolve()),BT3_GS='gpu',BT3_TAG_ONLINE='1',BT3_TAG_HOST=str(a.port),BT3_TAG_MENU_TEST='main',BT3_TAG_TEAM='1',BT3_UPDATE_CHECK='0',BT3_WINDOW='800x600',BT3_SETTINGS=str(out/'settings.txt'),BT3_SAVES=str(out/'saves'),SDL_AUDIO_DRIVER='dummy')
env['BT3_PAD']='200:cross,250:left,3600:cross';(out/'shots').mkdir();env['BT3_UI_SHOTS']='300,1400,3590,3650:'+str(out/'shots')+os.sep
(out/'settings.txt').write_text('frame_skip=0\n')
# TagNetStatus's fixed-width fields before config, plus TagNetConfig.stage.
config_stage_offset=20*4+192+11*4+10*192+3*4
k=ctypes.WinDLL('kernel32',use_last_error=True);k.OpenProcess.argtypes=[wintypes.DWORD,wintypes.BOOL,wintypes.DWORD];k.OpenProcess.restype=wintypes.HANDLE
k.ReadProcessMemory.argtypes=[wintypes.HANDLE,ctypes.c_void_p,ctypes.c_void_p,ctypes.c_size_t,ctypes.POINTER(ctypes.c_size_t)];k.WriteProcessMemory.argtypes=k.ReadProcessMemory.argtypes;k.CloseHandle.argtypes=[wintypes.HANDLE]
with (out/'game.log').open('w')as f:
 proc=subprocess.Popen([str(exe)],cwd=out,env=env,stdout=f,stderr=subprocess.STDOUT);h=k.OpenProcess(0x1038,False,proc.pid);samples=[]
 def read(addr,n=4):
  x=ctypes.create_string_buffer(n);got=ctypes.c_size_t();assert k.ReadProcessMemory(h,addr,x,n,ctypes.byref(got));return int.from_bytes(x.raw,'little')
 def put(addr,v):
  x=ctypes.create_string_buffer(struct.pack('<I',v));got=ctypes.c_size_t();assert k.WriteProcessMemory(h,addr,x,4,ctypes.byref(got))
 try:
  until=time.monotonic()+90
  while 'Room open.'not in (out/'game.log').read_text(errors='replace'):
   if proc.poll()is not None:raise RuntimeError('Exited before room open')
   if time.monotonic()>until:raise RuntimeError('Room timeout')
   time.sleep(.2)
  objnm=a.menu_symbols.read_text();local={name:base+int(re.search(r'^([0-9a-fA-F]+)\s+b\s+'+name+r'$',objnm,re.M)[1],16)for name in ('page','focus','stageCount','stageIds','stageCursor','choice','room')}
  count=read(local['stageCount']);ids=[read(local['stageIds']+i*4)for i in range(count)];assert count>35 and len(set(ids))==count and 35 not in ids
  assert read(symbols['gGsFrame'])<180,'Compatibility check exceeded the scheduled input window; rerun after it caches the data fingerprint.'
  put(local['page'],4);put(local['focus'],102);put(local['stageCursor'],2)
  while read(symbols['gGsFrame'])<215:time.sleep(.02)
  chosen=ids[2];assert read(local['choice'])==chosen
  while read(symbols['gGsFrame'])<265:time.sleep(.02)
  assert read(local['stageCursor'])!=2 and read(local['choice'])==chosen,'Controller browsing replaced the selected stage'
  for n in range(300):
   assert proc.poll()is None,'Game exited while cycling'
   cursor=n%count;put(local['stageCursor'],cursor);put(local['focus'],100+cursor);time.sleep(.12)
   assert read(local['choice'])==chosen,'Browsing replaced selected stage'
   samples.append(dict(step=n,stage=ids[cursor],frame=read(symbols['gGsFrame']),arena_used=read(symbols['sLowNext'],8)-0x13000000,size_classes=read(symbols['sLowSizes'])))
  assert samples[-1]['arena_used']<=samples[2*count]['arena_used']+512*1024,'Stage browsing continued consuming allocator space after warm-up'
  put(local['focus'],4)
  until=time.monotonic()+90
  while read(symbols['gGsFrame'])<3655:
   assert proc.poll()is None and time.monotonic()<until;time.sleep(.05)
  assert read(local['page'])==2 and read(local['room']+config_stage_offset)==chosen,'Confirm did not commit selected stage'
  result=dict(completed=len(samples),playable_stages=count,selected_stage=chosen,confirmed_stage=read(local['room']+config_stage_offset),returncode=proc.poll(),samples=samples);(out/'result.json').write_text(json.dumps(result,indent=2));print(json.dumps({k:v for k,v in result.items()if k!='samples'}),flush=True)
 finally:
  if proc.poll()is None:proc.terminate()
  proc.wait(timeout=10);k.CloseHandle(h)
