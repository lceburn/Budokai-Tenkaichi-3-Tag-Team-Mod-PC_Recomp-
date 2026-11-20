"""Private offline smoke tests of imported warriors through the real Windows game.

Requires an experimental build and locally imported BT4 data. Results, logs and
screenshots stay under ignored port/build/. Optional round-trip test exercises
both form changes through the game's resource loader and action handlers.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
TRACE = re.compile(r"tagteam frame=(\d+) fighter=(\d+) chara=(\d+) team=(\d+) target=(\d+) hp=(-?\d+) action=(\d+)")


def warrior_inputs(data):
    """Bind results to the exact private folder order, recipes and resources."""
    mods = Path(data) / "mods"
    enabled = mods / "enabled.txt"
    if not enabled.is_file():
        return []
    result = []
    for name in enabled.read_text(encoding="utf-8").splitlines():
        if not name or name.startswith("#"):
            continue
        folder = mods / name
        if folder.parent.resolve() != mods.resolve():
            raise ValueError("Unsafe enabled warrior name")
        files = {}
        for path in sorted(folder.rglob("*")):
            if path.is_file():
                files[path.relative_to(folder).as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
        result.append({"folder": name, "files": files})
    return result


def run(args, character, costume=0, damaged=False):
    name = f"{character}-{costume}-{int(damaged)}-{args.renderer}"
    out = args.output.resolve() / name
    out.mkdir(parents=True, exist_ok=True)
    settings = out / "settings.txt"
    settings.write_text("tag_extra_intros=0\n", encoding="utf-8")
    env = {key: value for key, value in os.environ.items() if not key.startswith("BT3_")}
    env.update(BT3_DATA=str(args.data.resolve()), BT3_GS=args.renderer, BT3_DEMO="1",
               BT3_TAG_TEAM="1", BT3_TAG_ROSTER="1,1", BT3_TAG_TRACE="1",
               BT3_CHARA=str(character), BT3_COSTUME=str(costume), BT3_TEST_FRAMES=str(args.frames),
               BT3_SETTINGS=str(settings), BT3_SAVES=str(out / "saves"), BT3_UPDATE_CHECK="0",
               BT3_LOAD_TRACE="1", SDL_AUDIO_DRIVER="dummy")
    if character >= 250:
        env["BT3_WARRIOR_MODELS"] = "1"
    if args.human:
        env["BT3_TAG_PLAY"] = str(args.players)
    if args.trace_nodes:
        env["BT3_WARRIOR_TRACE"] = "1"
    if args.script_mods:
        mod_settings=out / "mods-settings.txt"
        mod_settings.write_text("modern_camera=1\nmodern_hud=1\nmodern_look=1\nskill_list=1\n",encoding="utf-8")
        env.update(BT3_MODS=str(ROOT / "port/examples/mods"),BT3_MODS_SETTINGS=str(mod_settings))
    if args.transform_roundtrip:
        env.update(BT3_TAG_TEST_CINEMA="28",BT3_TAG_SPECIAL_CHARACTER=str(character),BT3_TAG_PLAY="2")
    if damaged:
        env["BT3_VARIANT"] = "1"
    if args.renderer == "gpu":
        env.update(BT3_SHOT_VBLANK=str(args.shot_vblank), BT3_SHOT_DIR=str(out), BT3_UNCAPPED="1")
    log = out / "game.log"
    code = None
    with log.open("w", encoding="utf-8") as stream:
        try:
            result = subprocess.run([str(args.exe.resolve())], cwd=ROOT, env=env,
                                    stdout=stream, stderr=subprocess.STDOUT, timeout=args.timeout)
            code = result.returncode
        except subprocess.TimeoutExpired:
            code = "timeout"
    text = log.read_text(encoding="utf-8", errors="replace")
    trace = [tuple(map(int, match)) for match in TRACE.findall(text)]
    actors = {}
    for row in trace:
        entry = actors.setdefault(row[1], {"character": row[2], "actions": set(), "initial_hp": row[5], "minimum_hp": row[5]})
        entry["actions"].add(row[6])
        entry["minimum_hp"] = min(entry["minimum_hp"], row[5])
    completed = f"bt3: test frames complete: {args.frames}" in text or "bt3: battle finished" in text
    forms_seen=sorted({row[2] for row in trace if row[1]==0})
    if args.transform_roundtrip:
        completed="tagteam fixture: PASS check=99" in text and "tagteam fixture: FAIL" not in text
    requested = f"model load: actor=0 kind=-1 character={character} costume={costume} variant={int(damaged)}" in text
    passed = code == 0 and completed and requested and actors.get(0, {}).get("character") == character
    if args.transform_roundtrip:
        passed=passed and 250 in forms_seen and 251 in forms_seen
    if args.script_mods:
        passed = passed and all(f"bt3: mod {name}: loaded:" in text for name in
                               ("modern_camera","modern_hud","modern_look","skill_list"))
        passed = passed and not re.search(r"bt3: mod .*: (?:error|stopped)",text,re.I)
    result = dict(case=name, passed=passed, exit=code, reached_fight=bool(trace), completed=completed,
                  requested_model=requested, forms_seen=forms_seen, actors={str(key): value | {"actions": sorted(value["actions"])}
                                                     for key, value in actors.items()})
    (out / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    print(("PASS " if passed else "FAIL ") + name, flush=True)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, default=ROOT / "port/build/bt3.exe")
    parser.add_argument("--data", type=Path, default=ROOT / "gamedata-bt4")
    parser.add_argument("--output", type=Path, default=ROOT / "port/build/warrior-runtime")
    parser.add_argument("--frames", type=int, default=600)
    parser.add_argument("--timeout", type=int, default=120)
    parser.add_argument("--renderer", choices=("none", "gpu"), default="none")
    parser.add_argument("--shot-vblank", type=int, default=300)
    parser.add_argument("--character", type=int, action="append")
    parser.add_argument("--variants", action="store_true")
    parser.add_argument("--costumes", type=int, choices=(1,2,3,4), default=2,
                        help="Number of custom costumes to exercise with --variants")
    parser.add_argument("--human", action="store_true", help="Use the human-controlled player-one setup without injecting inputs")
    parser.add_argument("--players",type=int,choices=(1,2),default=1,help="Two idle humans isolate stance alignment from CPU attacks")
    parser.add_argument("--trace-nodes", action="store_true")
    parser.add_argument("--script-mods", action="store_true",help="Exercise bundled camera, HUD, look and skill-list mods together")
    parser.add_argument("--transform-roundtrip",action="store_true",help="Validate form change and return with normal battle handlers")
    args = parser.parse_args()
    if args.frames < 1 or args.timeout < 1:
        parser.error("Frames and timeout must be positive")
    args.output.mkdir(parents=True, exist_ok=True)
    record = dict(executable=hashlib.sha256(args.exe.read_bytes()).hexdigest(),
                  frames=args.frames, renderer=args.renderer,
                  transform_roundtrip=args.transform_roundtrip,
                  disc=json.loads((args.data / "native-disc.json").read_text()),
                  warrior_inputs=warrior_inputs(args.data))
    if args.script_mods:
        record["script_mods"]={p.relative_to(ROOT / "port/examples/mods").as_posix():
                               hashlib.sha256(p.read_bytes()).hexdigest()
                               for p in sorted((ROOT / "port/examples/mods").rglob("*")) if p.is_file()}
    (args.output / "build.json").write_text(json.dumps(record, indent=2) + "\n")
    results = []
    for character in args.character or [52, 250, 251]:
        for costume in range(args.costumes if args.variants and character >= 250 else 1):
            for damaged in range(2 if args.variants and character >= 250 else 1):
                results.append(run(args, character, costume, bool(damaged)))
                (args.output / "results.json").write_text(json.dumps(results, indent=2) + "\n")
                if not results[-1]["passed"]:
                    raise SystemExit(1)


if __name__ == "__main__":
    main()
