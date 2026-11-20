"""Drive native creator previews and saved-character playtests in private test data.

The save cases edit the selected test recipe. Use a disposable library, never
the working game installation. Tests require GPU rendering for the pad clock.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time
import re

from warrior_runtime import warrior_inputs

ROOT=Path(__file__).resolve().parents[2]


def child_interpreters(parent,all_images=False):
    """Read only this test game's child image names; never control other apps."""
    if os.name!='nt':return set()
    import ctypes
    from ctypes import wintypes
    class Entry(ctypes.Structure):
        _fields_=[('size',wintypes.DWORD),('usage',wintypes.DWORD),('pid',wintypes.DWORD),
                  ('heap',ctypes.c_size_t),('module',wintypes.DWORD),('threads',wintypes.DWORD),
                  ('parent',wintypes.DWORD),('priority',wintypes.LONG),('flags',wintypes.DWORD),
                  ('name',wintypes.WCHAR*260)]
    kernel=ctypes.WinDLL('kernel32',use_last_error=True)
    kernel.CreateToolhelp32Snapshot.argtypes=[wintypes.DWORD,wintypes.DWORD]
    kernel.CreateToolhelp32Snapshot.restype=wintypes.HANDLE
    kernel.Process32FirstW.argtypes=kernel.Process32NextW.argtypes=[wintypes.HANDLE,ctypes.POINTER(Entry)]
    kernel.CloseHandle.argtypes=[wintypes.HANDLE]
    handle=kernel.CreateToolhelp32Snapshot(2,0);result=set()
    if not handle or handle==ctypes.c_void_p(-1).value:return result
    try:
        entry=Entry();entry.size=ctypes.sizeof(entry);valid=kernel.Process32FirstW(handle,ctypes.byref(entry))
        while valid:
            if entry.parent==parent and (all_images or entry.name.lower().startswith('python')):result.add(entry.name.lower())
            valid=kernel.Process32NextW(handle,ctypes.byref(entry))
    finally:kernel.CloseHandle(handle)
    return result


def sequence(case,entry,recipe=None):
    events=[(150,'down'),(210,'down'),(270,'down'),(330,'down'),(390,'down'),(450,'cross')]
    if case=='body-cancel':
        events += [(900,'cross'),(960,'down'),(1020,'down'),(1080,'cross')]
        # Goku (End) is source 3; seven upward steps wrap to Chichi at 157.
        events += [(1140+i*60,'up') for i in range(7)]
        events += [(1680,'cross'),(3000,'triangle'),(3060,'triangle'),(3120,'cross')]
        return events,2800
    tick=900
    for _ in range(entry+1):events.append((tick,'down'));tick+=60
    events.append((tick,'cross'));tick+=60
    if case=='native-label-save-fight':
        events += [(tick,'r1'),(tick+60,'r1')];tick+=120
        fields=re.findall(r' X\((\w+),2,',(ROOT/'include/menu/warrior_fields.h').read_text())
        steps=len(fields)-fields.index('BLAST11NAME')
        events += [(tick+i*24,'up') for i in range(steps)];tick+=steps*24
        events += [(tick,'cross'),(tick+60,'cross'),(tick+120,'start'),(2600,'start'),
                   (4000,'cross'),(4060,'cross'),(4180,'select'),(5300,'start'),
                   (5400,'down'),(5460,'down'),(5520,'down'),(5580,'down'),
                   (5640,'cross'),(5760,'up'),(5820,'cross')]
        return events,4100
    if case in ('native-voice-save-fight','native-style-save-fight'):
        key='voice' if case=='native-voice-save-fight' else 'style'
        for i in range(3 if key=='voice' else 4):events.append((tick+i*24,'down'))
        tick+=(3 if key=='voice' else 4)*24
        events.append((tick,'cross'));tick+=60
        allowed=recipe['_native_allowed'];current=recipe.get(key) or recipe.get('body' if key=='voice' else 'moves')
        start=allowed.index(current) if current in allowed else 0;target=allowed.index('Nappa')
        down=(target-start)%len(allowed);up=(start-target)%len(allowed)
        direction,steps=('down',down) if down<=up else ('up',up)
        events += [(tick+i*6,direction) for i in range(steps)];tick+=steps*6
        events += [(tick,'cross'),(2600,'start'),(4000,'cross'),(4060,'cross'),(4180,'select'),
                   (5300,'start'),(5400,'down'),(5460,'down'),(5520,'down'),(5580,'down'),
                   (5640,'cross'),(5760,'up'),(5820,'cross')]
        return events,4100
    if case=='attack-controls-save':
        events += [(tick,'r1'),(tick+60,'r1')];tick+=120
        # Locate controls by schema so additions do not invalidate navigation.
        fields=re.findall(r' X\((\w+),2,',(ROOT/'include/menu/warrior_fields.h').read_text())
        steps=len(fields)-fields.index('BLAST11NAME')
        events += [(tick+i*60,'up') for i in range(steps)];tick+=steps*60
        events += [(tick,'cross'),(tick+60,'cross'),(tick+120,'start')];tick+=180
        events += [(tick+i*60,'down') for i in range(5)];tick+=300
        events += [(tick,'cross'),(tick+60,'left'),(tick+120,'cross'),(4800,'start')]
        return events,4500
    if case=='attack-effects-save-fight':
        events += [(tick,'r1'),(tick+60,'r1')];tick+=120
        fields=re.findall(r' X\((\w+),2,',(ROOT/'include/menu/warrior_fields.h').read_text())
        steps=len(fields)-fields.index('BLAST21FX')
        events += [(tick+i*60,'up') for i in range(steps)];tick+=steps*60
        events.append((tick,'cross'));tick+=60
        # Vegeta (Scouter), slot 1: a distinct beam visual source, unlike the
        # shared Kamehameha effect of the two Goku donors used in the first probe.
        events += [(tick+i*24,'down') for i in range(30)];tick+=720
        events += [(tick,'cross'),(3500,'start'),(5000,'cross'),(5060,'cross'),(5180,'select'),
                   (6300,'start'),(6400,'down'),(6460,'down'),(6520,'down'),
                   (6580,'down'),(6640,'cross'),(6760,'up'),(6820,'cross')]
        return events,3300
    if case=='playtest-return':
        events += [(tick+120,'select'),(2300,'start'),(2400,'down'),(2460,'down'),
                   (2520,'down'),(2580,'down'),(2640,'cross'),(2760,'up'),(2820,'cross')]
        return events,2100
    events.append((tick,'r1'));tick+=60
    if case=='browse-no-build':
        events.append((tick,'cross'))
        events += [(tick+60+i*60,'down') for i in range(8)]
        events += [(2500,'triangle'),(2560,'triangle'),(2620,'triangle')]
        return events,2200
    if case in ('whole-head-save','whole-head-save-fight'):
        # Start at the recipe's actual grouped source rather than assuming a
        # mixed/default head. User-created libraries can start anywhere.
        catalog=(ROOT/'src/menu/port_tbo.c').read_text().split('sTbNames[] = {',1)[1].split('};',1)[0]
        donors=re.findall(r'"([^"]+)"',catalog)
        current=0
        if recipe:
            values=[recipe.get(key,'') for key in ('hair','head','face')]
            if values.count(values[0])==3:
                source=values[0].split(', costume ')[0]
                if source=='none':current=1
                elif source in donors:current=2+donors.index(source)
        count=len(donors)+2;down=(2-current)%count;up=(current-2)%count
        direction,steps=('down',down) if down<=up else ('up',up)
        events.append((tick,'cross'));tick+=60
        events += [(tick+i*24,direction) for i in range(steps)];tick+=steps*24
        events += [(tick,'cross'),(2600,'start')]
        if case=='whole-head-save-fight':
            events += [(4000,'cross'),(4060,'cross'),(4180,'select'),(5300,'start'),
                       (5400,'down'),(5460,'down'),(5520,'down'),(5580,'down'),
                       (5640,'cross'),(5760,'up'),(5820,'cross')]
            return events,4100
        return events,2400
    if case=='layer-save':
        events += [(tick,'down'),(tick+60,'down'),(tick+120,'circle')]
        events += [(tick+180+i*60,'down') for i in range(3)]
        events += [(2600,'cross'),(3800,'start')]
        return events,3500
    # Seven size fields follow the four layers; wrap backwards from row zero.
    events += [(tick+i*60,'up') for i in range(8)]
    tick+=8*60;events.append((tick,'cross'));tick+=60
    events += [(tick+i*60,'right') for i in range(4)]
    events += [(2400,'cross'),(3600,'start')]
    if case=='sizing-save-fight':
        events += [(4500,'cross'),(4560,'cross'),(4680,'select'),(5900,'start'),
                   (6000,'down'),(6060,'down'),(6120,'down'),(6180,'down'),
                   (6240,'cross'),(6360,'up'),(6420,'cross')]
        return events,5500
    return events,3300


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',type=Path,default=ROOT/'port/build/bt3.exe')
    parser.add_argument('--data',type=Path,required=True)
    parser.add_argument('--builder-copy',type=Path)
    parser.add_argument('--reference-resources',type=Path,help='Previously generated golden resources; never executes the reference')
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--entry',type=int,default=2)
    parser.add_argument('--case',choices=('body-cancel','browse-no-build','whole-head-save','whole-head-save-fight','layer-save','sizing-save','sizing-save-fight','playtest-return','attack-controls-save','attack-effects-save-fight','native-label-save-fight','native-voice-save-fight','native-style-save-fight'),required=True)
    parser.add_argument('--timeout',type=int,default=180)
    args=parser.parse_args();data=args.data.resolve();out=args.output.resolve()
    native=args.case.startswith('native-')
    native_resources=args.case in ('native-voice-save-fight','native-style-save-fight')
    if native_resources and not args.reference_resources:parser.error('This resource comparison needs golden resources from the controlled probe')
    if not native and not args.builder_copy:parser.error('This resource-building case requires the isolated reference builder')
    before=warrior_inputs(data);count=len(before)
    if args.entry<0 or args.entry>=count:parser.error('Choose a saved test entry')
    if args.case=='body-cancel' and count>=4:parser.error('The fresh-body case needs one free form slot')
    if out.exists() and any(out.iterdir()):parser.error('Choose an empty output folder')
    out.mkdir(parents=True,exist_ok=True)
    target=data/'mods'/before[args.entry]['folder']/'warrior.txt'
    recipe_before=target.read_bytes();enabled_before=(data/'mods/enabled.txt').read_bytes()
    initial=dict(line.split('=',1) for line in recipe_before.decode('utf-8').splitlines() if '=' in line and not line.startswith('#'))
    if args.case=='attack-effects-save-fight' and initial.get('blast2_1fx'):
        parser.error('Use a disposable recipe without a first Blast 2 effect override for this source-selection case')
    if args.case=='attack-controls-save' and (initial.get('blast1_1name') or initial.get('blast1_1colour')):
        parser.error('Use a disposable recipe without first Blast 1 name/colour overrides for this case')
    if args.case=='native-label-save-fight' and initial.get('blast1_1name'):parser.error('Use a disposable recipe without a first attack-name override')
    if native_resources:
        roster={' '.join(line.split('\t')[1:]).strip() for line in (data/'native-roster.tsv').read_text().splitlines()}
        catalog=(ROOT/'src/menu/port_tbo.c').read_text().split('sTbNames[] = {',1)[1].split('};',1)[0]
        initial['_native_allowed']=[name for name in re.findall(r'"([^"]+)"',catalog) if name in roster]
    events,shot=sequence(args.case,args.entry,initial);capture=out/'capture.ppm'
    env={k:v for k,v in os.environ.items() if not k.startswith('BT3_')}
    env.update(BT3_DATA=str(data),BT3_GS='gpu',BT3_TAG_TEAM='1',BT3_WARRIOR_MODELS='1',
               BT3_TAG_MENU_TEST='main',BT3_TAG_TRACE='1',BT3_LOAD_TRACE='1',BT3_UPDATE_CHECK='0',
               BT3_WINDOW='1024x768',BT3_SETTINGS=str(out/'settings.txt'),BT3_SAVES=str(out/'saves'),
               SDL_AUDIO_DRIVER='dummy',
               BT3_UI_SHOT=f'{shot}:{capture.as_posix()}',BT3_PAD=','.join(f'{f}:{key}' for f,key in events))
    if not native:
        env.update(BT3_WARRIOR_PYTHON=sys.executable,BT3_WARRIOR_BUILD_TOOL=str(ROOT/'port/tools/build_warrior_draft.py'),
                   BT3_WARRIOR_BUILDER_COPY=str(args.builder_copy.resolve()))
    log=out/'game.log';completed=False;preview_recipe='';preview_seen=False;interpreters=set();preview_model_hash=''
    with log.open('w') as stream:
        process=subprocess.Popen([str(args.exe.resolve())],cwd=ROOT,env=env,stdout=stream,stderr=subprocess.STDOUT)
        try:
            deadline=time.monotonic()+args.timeout
            while time.monotonic()<deadline:
                text=log.read_text(errors='replace')
                interpreters.update(child_interpreters(process.pid,native))
                if capture.exists() and not preview_seen:
                    pointer=data/f'.warrior-preview-{process.pid}-path.txt'
                    if pointer.exists():
                        relative=pointer.read_text().strip()
                        folder=(data/relative).resolve()
                        if folder.is_relative_to(data/'.warrior-builds') and (folder/'warrior.txt').exists():
                            preview_recipe=(folder/'warrior.txt').read_text();preview_seen=True
                            model=next(folder.glob('*_1p.pak'),None)
                            if model:preview_model_hash=hashlib.sha256(model.read_bytes()).hexdigest()
                reopened=text.count(f'warrior-menu open count={count}')>=2
                if args.case=='body-cancel':completed=reopened and 'warriorui page=8' in text
                elif args.case=='browse-no-build':
                    completed=bool(re.search(r'tagmenu frame=\d+ page=4 ',text[text.rfind('warriorui page=2'):]))
                elif args.case=='playtest-return':
                    completed=reopened and f'model load: actor=0 kind=-1 character={250+args.entry} ' in text
                elif native:
                    completed=text.count(f'warrior-menu open count={count}')>=3 and 'warrior native ' in text and f'model load: actor=0 kind=-1 character={250+args.entry} ' in text
                elif args.case in ('sizing-save-fight','whole-head-save-fight','attack-effects-save-fight'):
                    completed=text.count(f'warrior-menu open count={count}')>=3 and 'warrior builder: {' in text and f'model load: actor=0 kind=-1 character={250+args.entry} ' in text
                else:completed=reopened and 'warrior builder: {' in text
                if completed or process.poll() is not None:break
                time.sleep(.1)
        finally:
            if process.poll() is None:process.terminate()
            process.wait(timeout=10)
    text=log.read_text(errors='replace');after=warrior_inputs(data)
    passed=completed and capture.is_file() and enabled_before==(data/'mods/enabled.txt').read_bytes()
    passed=passed and 'bt3 crash' not in text and 'tagteam fixture: FAIL' not in text
    if args.case in ('body-cancel','browse-no-build','playtest-return'):
        passed=passed and before==after and recipe_before==target.read_bytes()
    if args.case=='body-cancel':passed=passed and 'body=Chichi' in preview_recipe
    if args.case=='browse-no-build':passed=passed and 'warrior preview:' not in text and not interpreters
    if args.case in ('whole-head-save','whole-head-save-fight'):
        current=target.read_text()
        for key in ('hair','head','face'):
            passed=passed and f'{key}=Goku (Early)' in current and f'{key}=Goku (Early)' in preview_recipe
        passed=passed and 'headset=' not in current
        model=next(target.parent.glob('*_1p.pak'),None)
        passed=passed and model is not None and preview_model_hash==hashlib.sha256(model.read_bytes()).hexdigest()
    if args.case in ('layer-save','sizing-save','sizing-save-fight'):
        expected='extra1=hair:Goku (Med) Super Saiyan' if args.case=='layer-save' else 'headsize=120'
        passed=passed and expected in target.read_text() and expected in preview_recipe
        passed=passed and len(after)==len(before) and all(a==b for i,(a,b) in enumerate(zip(after,before)) if i!=args.entry)
    if args.case=='attack-controls-save':
        for expected in ('blast1_1name=A','blast1_1colour=f7ffff'):
            passed=passed and expected in target.read_text() and expected in preview_recipe
        passed=passed and len(after)==len(before) and all(a==b for i,(a,b) in enumerate(zip(after,before)) if i!=args.entry)
    if args.case=='attack-effects-save-fight':
        expected='blast2_1fx=Vegeta (Scouter):1'
        passed=passed and expected in target.read_text() and expected in preview_recipe
        passed=passed and len(after)==len(before) and all(a==b for i,(a,b) in enumerate(zip(after,before)) if i!=args.entry)
        effect_keys=[k for k in before[args.entry]['files'] if k.endswith('_eff.pak')]
        passed=passed and any(before[args.entry]['files'][k]!=after[args.entry]['files'].get(k) for k in effect_keys)
    if native:
        expected='blast1_1name=A' if not native_resources else ('voice=Nappa' if args.case=='native-voice-save-fight' else 'style=Nappa')
        passed=passed and expected in target.read_text() and not interpreters
        passed=passed and 'warrior builder:' not in text and 'warrior preview:' not in text
        passed=passed and len(after)==len(before) and all(a==b for i,(a,b) in enumerate(zip(after,before)) if i!=args.entry)
        allowed={'warrior.txt'}
        if native_resources:
            models=[key for key in before[args.entry]['files'] if re.search(r'_[1-4]p(?:_dmg)?\.pak$',key)]
            allowed.update(models);allowed.add('mod.txt')
            passed=passed and bool(models)
            for key in models:
                golden=args.reference_resources/key
                passed=passed and golden.is_file() and after[args.entry]['files'].get(key)==hashlib.sha256(golden.read_bytes()).hexdigest()
        passed=passed and all(value==after[args.entry]['files'].get(key) for key,value in before[args.entry]['files'].items() if key not in allowed)
    if os.name=='nt' and args.case not in ('browse-no-build','playtest-return') and not native:
        passed=passed and 'pythonw.exe' in interpreters and 'python.exe' not in interpreters
    record=dict(passed=passed,case=args.case,completed=completed,capture=capture.name,
                entered_creator='warrior-menu open count=' in text,built='warrior builder: {' in text,
                reloaded=text.count(f'warrior-menu open count={count}')>=2,
                fight_completed=passed and (native or args.case in ('playtest-return','sizing-save-fight','whole-head-save-fight','attack-effects-save-fight')),
                native_saved=native and 'warrior native ' in text,
                executable=hashlib.sha256(args.exe.read_bytes()).hexdigest(),
                preview_recipe=preview_recipe,preview_model_hash=preview_model_hash,worker_interpreters=sorted(interpreters),warrior_inputs_before=before,warrior_inputs_after=after)
    (out/'result.json').write_text(json.dumps(record,indent=2)+'\n')
    print(('PASS' if passed else 'FAIL'),args.case,str(log),flush=True)
    if not passed:raise SystemExit(1)


if __name__=='__main__':main()
