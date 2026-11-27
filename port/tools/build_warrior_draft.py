"""Build private warrior drafts with a prepared copy of the reference creator.

This is a development bridge, not a replacement game engine or a redistributable
creator. The reference executable and its BT3 disc data must remain private.
Jobs never run in the user's original reference installation. Successful outputs
are validated and staged before replacing any BT4 warrior folder; failed jobs
leave the current playable folders intact.
"""
import argparse
from contextlib import contextmanager
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import uuid

from inspect_warrior import fields, inspect_set, safe_name
from warrior_colours import catalog as colour_catalog

LIMIT = 128 * 1024
MAX_FORMS = 6  # Saved native library capacity; each form retains four direct links.


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def component(name):
    safe_name(name)
    if (not name or len(name.encode()) >= 192 or "/" in name or name in (".", "..")
            or name[-1] in ". " or any(ord(c) < 32 for c in name)):
        raise ValueError("Invalid warrior folder name")
    return name


def child(root, name, exists=True):
    path = root / component(name)
    resolved = path.resolve()
    if resolved.parent != root.resolve() or path.is_symlink():
        raise ValueError("Warrior folder leaves its library")
    if exists and not path.is_dir():
        raise ValueError("A required warrior folder is missing")
    return path


def recipe(path):
    raw = path.read_bytes()
    if len(raw) > LIMIT or b"\0" in raw:
        raise ValueError("Invalid warrior recipe size or text")
    parsed = fields(raw)
    for key in ("name", "body", "moves"):
        if len(parsed.get(key, [])) != 1 or not parsed[key][0]:
            raise ValueError("A draft needs exactly one name, body and moves field")
    return raw, parsed


def references(parsed):
    for key, values in parsed.items():
        if re.fullmatch(r"form[1-4]", key):
            if len(values) != 1:
                raise ValueError("Duplicate form link")
            parts = values[0].split(",", 2)
            if len(parts) != 3:
                raise ValueError("Malformed form link")
            yield component(parts[2])


def family(drafts, mods, requested):
    result = {}
    pending = list(requested)
    while pending:
        name = component(pending.pop(0))
        if name in result:
            continue
        source = child(drafts, name, False)
        if not source.is_dir():
            source = child(mods, name)
        raw, parsed = recipe(source / "warrior.txt")
        result[name] = (source, raw)
        if len(result) > MAX_FORMS:
            raise ValueError("The saved library supports six custom characters/forms")
        pending.extend(references(parsed))
    return result


def rewrite_links(raw, mapping, generated=False):
    text = raw.decode("utf-8-sig")
    lines = []
    for line in text.splitlines(keepends=True):
        ending = "\r\n" if line.endswith("\r\n") else "\n" if line.endswith("\n") else ""
        plain = line.rstrip("\r\n")
        if generated and re.match(r"trans[0-3]=", plain) and ",@" in plain:
            prefix, target = plain.rsplit(",@", 1)
            if target not in mapping:
                raise ValueError("Builder emitted an unexpected form dependency")
            plain = prefix + ",@" + mapping[target]
        elif not generated and re.match(r"form[1-4]=", plain):
            prefix, target = plain.rsplit(",", 1)
            if target not in mapping:
                raise ValueError("Draft form dependency was not staged")
            plain = prefix + "," + mapping[target]
        lines.append(plain + ending)
    return "".join(lines).encode("utf-8")


def copy_edit_assets(source, destination):
    paths=[]
    paint=source/"paint"
    if paint.exists():
        if paint.is_symlink():raise ValueError("Linked paint assets are unsupported")
        paths.extend(p for p in paint.rglob("*") if p.is_file() or p.is_symlink())
    for name in ("portrait_custom.bin","hud_photo.raw","chip_photo.raw"):
        if (source/name).exists():paths.append(source/name)
    total=0
    for path in paths:
        if path.is_symlink() or not path.resolve().is_relative_to(source.resolve()):
            raise ValueError("Linked editor assets are unsupported")
        size=path.stat().st_size;total+=size
        if size>64*1024*1024 or total>256*1024*1024:raise ValueError("Editor assets exceed the supported size")
        target=destination/path.relative_to(source);target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(path,target)


@contextmanager
def builder_lock(root):
    # A different game process must not change the helper's temporary enable list.
    with (root / ".bt4-builder.lock").open("a+b") as lock:
        lock.seek(0)
        if os.name == "nt":
            import msvcrt
            if not lock.read(1):
                lock.write(b"0"); lock.flush()
            lock.seek(0)
            msvcrt.locking(lock.fileno(), msvcrt.LK_NBLCK, 1)
        else:
            import fcntl
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        try:
            yield
        finally:
            lock.seek(0)
            if os.name == "nt":
                msvcrt.locking(lock.fileno(), msvcrt.LK_UNLCK, 1)
            else:
                fcntl.flock(lock, fcntl.LOCK_UN)


def remove_job_folder(path, mods, prefix):
    # Check the final absolute target before every recursive removal on Windows.
    if path.resolve().parent != mods.resolve() or not path.name.startswith(prefix):
        raise ValueError("Refusing to remove a path outside the builder job")
    if path.is_dir():
        shutil.rmtree(path)


def publish(stage, mods, backups, names, snapshots, enabled_before, enabled_after):
    enabled = mods / "enabled.txt"
    if (enabled.read_bytes() if enabled.exists() else None) != enabled_before:
        raise ValueError("The enabled warrior list changed during the build")
    for name, expected in snapshots.items():
        path = child(mods, name, False) / "warrior.txt"
        if (digest(path) if path.is_file() else None) != expected:
            raise ValueError("A warrior changed during the build; reload before saving")
    backups.mkdir()
    installed, moved = [], []
    temporary = mods / (".enabled-" + uuid.uuid4().hex + ".tmp")
    try:
        for name in names:
            destination = child(mods, name, False)
            if destination.exists():
                os.replace(destination, backups / name)
                moved.append(name)
            os.replace(stage / name, destination)
            installed.append(name)
        temporary.write_bytes(enabled_after)
        os.replace(temporary, enabled)
    except BaseException:
        # Move rather than delete failed outputs, so rollback remains inspectable.
        failed = stage.parent / "failed-publish"
        failed.mkdir(exist_ok=True)
        for name in reversed(installed):
            os.replace(mods / name, failed / name)
        for name in reversed(moved):
            os.replace(backups / name, mods / name)
        raise
    finally:
        if temporary.exists():
            temporary.unlink()


def draft_inputs(drafts, mods, folders, preview=False):
    if not preview:
        return family(drafts,mods,folders)
    if len(folders)!=1:
        raise ValueError("A preview requires exactly one assembled character")
    name=component(folders[0]);source=child(drafts,name)
    raw,_=recipe(source/'warrior.txt')
    # The assembled model does not need the other forms. Never rebuild or
    # publish playable dependencies just to preview a highlighted mesh.
    raw=re.sub(rb'(?m)^form[1-4]=[^\r\n]*(?:\r?\n|$)',b'',raw)
    return {name:(source,raw)}


def build(args):
    helper, data, drafts = (p.resolve() for p in (args.builder_copy, args.data, args.drafts))
    marker = json.loads((helper / ".bt4-builder-copy.json").read_text())
    exe = helper / "Tenkaichi3Decomp.exe"
    if marker.get("isolated_copy") is not True or marker.get("executable_sha256") != digest(exe):
        raise ValueError("A verified isolated builder copy is required")
    if helper == data or helper == drafts or data == drafts:
        raise ValueError("Builder, game data and drafts must be separate folders")
    mods = data / "mods"
    mods.mkdir(exist_ok=True)
    preview=getattr(args,'preview',False)
    inputs = draft_inputs(drafts, mods, args.folder,preview)
    enabled = mods / "enabled.txt"
    enabled_before = enabled.read_bytes() if enabled.exists() else None
    current = [line for line in (enabled_before or b"").decode("utf-8-sig").splitlines()
               if line and not line.startswith("#")]
    for name in current:
        component(name)
    wanted = list(current)
    for name in inputs:
        if preview:
            continue
        if name not in wanted:
            if not args.enable:
                raise ValueError("A new warrior requires explicit enable mode")
            wanted.append(name)
    if len(wanted) > MAX_FORMS or len({n.casefold() for n in wanted}) != len(wanted):
        raise ValueError("The saved library supports six unique custom characters/forms")
    enabled_after=enabled_before or b""
    extra=[name for name in wanted if name not in current]
    if extra:
        newline=b"\r\n" if b"\r\n" in enabled_after else b"\n"
        if enabled_after and not enabled_after.endswith(b"\n"):
            enabled_after+=newline
        enabled_after+=newline.join(name.encode("utf-8") for name in extra)+newline
    snapshots = {name: digest(mods/name/"warrior.txt") if (mods/name/"warrior.txt").is_file() else None
                 for name in inputs}
    job_id = uuid.uuid4().hex
    job = data / ".warrior-builds" / job_id
    stage = job / "publish"
    stage.mkdir(parents=True)
    helper_mods = helper / "gamedata/mods"
    helper_enabled = helper_mods / "enabled.txt"
    prefix = "BT4Job-" + job_id + "-"
    mapping = {name: prefix + str(i) for i, name in enumerate(inputs)}
    reverse = {v: k for k, v in mapping.items()}
    staged = []
    with builder_lock(helper):
        saved_enable = helper_enabled.read_bytes() if helper_enabled.exists() else None
        try:
            for name, (source, raw) in inputs.items():
                dest = child(helper_mods, mapping[name], False)
                dest.mkdir(); staged.append(dest)
                (dest / "warrior.txt").write_bytes(rewrite_links(raw, mapping))
                copy_edit_assets(source,dest)
            helper_enabled.write_text("\n".join(mapping.values())+"\n", encoding="utf-8")
            env = {k: v for k, v in os.environ.items() if not k.startswith("BT3_")}
            env.update(BT3_DATA=str(helper/"gamedata"), BT3_GS="none", BT3_DEMO="1", BT3_CHARA="3",
                       BT3_TEST_FRAMES="60", BT3_UPDATE_CHECK="0", BT3_SETTINGS=str(job/"settings.txt"),
                       BT3_SAVES=str(job/"saves"), SDL_AUDIO_DRIVER="dummy")
            with (job / "builder.log").open("w", encoding="utf-8") as log:
                result = subprocess.run([str(exe)], cwd=helper, env=env, stdout=log,
                                        stderr=subprocess.STDOUT, timeout=args.timeout,
                                        creationflags=subprocess.CREATE_NO_WINDOW if os.name=='nt' else 0)
            if result.returncode:
                raise ValueError("The reference builder exited with an error")
            for name, (source, raw) in inputs.items():
                built = child(helper_mods, mapping[name])
                if (built / "warrior_error.txt").is_file():
                    raise ValueError((built / "warrior_error.txt").read_text(errors="replace")[:1024])
                # Validate before collecting any output. Copy only listed data files.
                inspect_set([built])
                dest = stage / name; dest.mkdir()
                paths = (built / ".warrior_built").read_text().splitlines()[1:]
                for relative in paths + [".warrior_built", "mod.txt"]:
                    if not relative:
                        continue
                    safe_name(relative)
                    src = built / relative
                    if src.is_symlink() or src.resolve().is_relative_to(built.resolve()) is False:
                        raise ValueError("Builder output leaves its job folder")
                    if re.search(r"_[1-4]p(?:_dmg)?\.pak$",relative) and (src.stat().st_size+2047)//2048*2048>0xE6800:
                        raise ValueError("The assembled model exceeds this BT4 build's model buffer; use fewer source parts")
                    target = dest / relative; target.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(src, target)
                (dest / "warrior.txt").write_bytes(raw)
                (dest / "mod.txt").write_bytes(rewrite_links((dest/"mod.txt").read_bytes(), reverse, True))
                copy_edit_assets(source,dest)
                model=next((dest/p for p in paths if re.search(r"_1p\.pak$",p)),None)
                if model is not None:
                    colours=colour_catalog(model.read_bytes(),decode=True)
                    (dest/"native-colours.tsv").write_text("# BT4 material colours v1\n"+"".join(
                        f'{c["key"]}\t{c["texture"]}\t{c["rgb"][0]}\t{c["rgb"][1]}\t{c["rgb"][2]}\n' for c in colours),encoding="ascii")
            report = inspect_set([stage/name for name in inputs])
            if any(w["unresolved_transformation_folders"] for w in report["warriors"]):
                raise ValueError("Generated form links did not resolve after staging")
            roster={" ".join(line.split("\t")[1:]).strip()
                    for line in (data/"native-roster.tsv").read_text(encoding="utf-8").splitlines()}
            for name in inputs:
                _,parsed=recipe(stage/name/"warrior.txt")
                style=parsed.get("style",parsed.get("moves",parsed.get("body")))[0]
                voice=parsed.get("voice",parsed.get("body"))[0]
                if style not in roster or voice not in roster:
                    raise ValueError("Choose a style and voice present in this BT4 roster")
            (job/"validated.json").write_text(json.dumps(report,indent=2)+"\n")
            record = {"built": list(inputs), "builder_sha256": digest(exe), "job": job_id}
            if preview:
                record["preview"]=str(stage/args.folder[0])
                record["published"]=False
                (drafts/'preview-result.txt').write_text(str(stage/args.folder[0]),encoding='utf-8')
            else:
                publish(stage, mods, job/"previous", inputs, snapshots, enabled_before,enabled_after)
                record["published"]=True
            (job/"result.json").write_text(json.dumps(record,indent=2)+"\n")
            return record
        finally:
            if saved_enable is None:
                if helper_enabled.exists():
                    helper_enabled.unlink()
            else:
                helper_enabled.write_bytes(saved_enable)
            for path in staged:
                remove_job_folder(path, helper_mods, prefix)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--builder-copy", type=Path, required=True)
    parser.add_argument("--data", type=Path, required=True)
    parser.add_argument("--drafts", type=Path, required=True)
    parser.add_argument("--folder", action="append", required=True)
    parser.add_argument("--enable", action="store_true")
    parser.add_argument("--preview",action="store_true",help="Build one temporary assembled model without publishing or enabling it")
    parser.add_argument("--timeout", type=int, default=45, choices=range(1,61))
    args = parser.parse_args()
    try:
        print(json.dumps(build(args)), flush=True)
    except (OSError, ValueError, subprocess.TimeoutExpired) as error:
        parser.exit(1, f"Warrior build failed: {error}\n")


if __name__ == "__main__":
    main()
