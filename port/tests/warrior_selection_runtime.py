"""Select imported forms through the real offline team picker and match menus.

No demo-character override or prepared battle: pad input drives setup, fighter,
form, equipment, outfit, CPU and stage selection. All outputs stay private.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import time
from warrior_runtime import warrior_inputs

ROOT=Path(__file__).resolve().parents[2]


def inputs(form,flow,direct=False,stock_after_custom=False,single_form=False):
    events=([(450,"start")] if direct else [(150,"select"),(210,"cross"),(270,"cross"),(450,"start")])
    tick=540
    def press(key):
        nonlocal tick
        events.append((tick,key));tick+=60
    # Final row, second cell: appended ICEBURN group, then its form reel.
    for key in ("up","right"):press(key)
    if stock_after_custom:
        for key in ("down","up"):press(key)
    press("cross")
    if form:press("right")
    # An unlinked warrior skips the form reel and goes straight to equipment.
    for key in (("cross","cross","right","cross") if single_form else ("cross","cross","cross","right","cross")):press(key)
    # One stock CPU opponent, then End and the stage.
    for key in ("cross","cross","cross","cross","cross","right","cross"):press(key)
    tick+=80;press("cross")
    if flow!="match":
        if flow=="characters":events.append((4140,"down"))
        events.extend(((4200,"cross"),(4300,"up"),(4400,"cross")))
    return ",".join(f"{t}:{key}" for t,key in events)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--exe",type=Path,default=ROOT/"port/build/bt3.exe")
    p.add_argument("--data",type=Path,default=ROOT/"gamedata-bt4")
    p.add_argument("--output",type=Path,default=ROOT/"port/build/warrior-selection")
    p.add_argument("--form",type=int,choices=(0,1),default=0)
    p.add_argument("--flow",choices=("match","characters","retry"),default="match")
    p.add_argument("--shot",type=int,default=620)
    p.add_argument("--direct-setup",action="store_true",help="Use the new roster launcher's direct team-setup entry")
    p.add_argument("--stock-after-custom",action="store_true",help="Reuse custom icon reel clips for stock fighters and back")
    p.add_argument("--single-form",action="store_true",help="An unlinked warrior skips form selection")
    a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
    (out/"settings.txt").write_text("tag_extra_intros=0\ntag_controller_check=0\n",encoding="utf-8")
    env={k:v for k,v in os.environ.items() if not k.startswith("BT3_")}
    env.update(BT3_GS="gpu",BT3_DATA=str(a.data.resolve()),BT3_WARRIOR_MODELS="1",
               BT3_TAG_TEAM="1",BT3_TAG_MENU_TEST="roster" if a.direct_setup else "main",BT3_MENU_LANG="en",BT3_UNCAPPED="1",
               BT3_UPDATE_CHECK="0",BT3_LOAD_TRACE="1",BT3_TAG_TRACE="1",BT3_WINDOW="1024x768",
               BT3_SETTINGS=str(out/"settings.txt"),BT3_SAVES=str(out/"saves"),
               BT3_MODS_SETTINGS="",BT3_PAD=inputs(a.form,a.flow,a.direct_setup,a.stock_after_custom,a.single_form),SDL_AUDIO_DRIVER="dummy",
               BT3_UI_SHOT=f"{a.shot}:{(out/'picker.ppm').as_posix()}")
    if a.flow=="match":env["BT3_TEST_FRAMES"]="120"
    else:env["BT3_TAG_TEST_RESULT"]="1"
    log=out/"game.log";done=False
    with log.open("w",encoding="utf-8") as f:
        proc=subprocess.Popen([str(a.exe.resolve())],cwd=ROOT,env=env,stdout=f,stderr=subprocess.STDOUT)
        try:
            deadline=time.monotonic()+120
            while time.monotonic()<deadline:
                text=log.read_text(encoding="utf-8",errors="replace")
                if a.flow=="match":
                    done=any(int(frame)>=1200 for frame in re.findall(
                        rf"tagteam frame=(\d+) fighter=0 chara={250+a.form} ",text))
                elif a.flow=="characters":
                    done=bool(re.search(r"tagmenu frame=(?:4[4-9]\d\d|[5-9]\d{3}|\d{5}) page=40 state=0,",text))
                else:
                    # Fight Again can reuse resident models. Verify a second
                    # battle actually advances, rather than requiring a reload.
                    done=text.count(f"tagteam frame=1200 fighter=0 chara={250+a.form} ")>=2
                if done or proc.poll() is not None:break
                time.sleep(0.1)
        finally:
            if proc.poll() is None:proc.terminate()
            proc.wait(timeout=10)
    text=log.read_text(encoding="utf-8",errors="replace")
    passed=done and f"model load: actor=0 kind=-1 character={250+a.form} costume=0 variant=0" in text
    passed=passed and "bt3 crash" not in text and "tagteam fixture: FAIL" not in text
    record=dict(passed=passed,form=a.form,flow=a.flow,executable=hashlib.sha256(a.exe.read_bytes()).hexdigest(),
                warrior_inputs=warrior_inputs(a.data),completed=done,direct_setup=a.direct_setup)
    (out/"result.json").write_text(json.dumps(record,indent=2)+"\n",encoding="utf-8")
    print(("PASS" if passed else "FAIL"),a.form,a.flow,str(log),flush=True)
    if not passed:raise SystemExit(1)


if __name__=="__main__":main()
