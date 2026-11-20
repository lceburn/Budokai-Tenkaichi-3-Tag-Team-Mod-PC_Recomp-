"""Inspect generated Z Warriors folders/ZIPs without installing or executing them.

This is an import preflight, not a game-data converter or a compatibility claim.
Reports contain local names and hashes: keep real-character reports private.
"""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import struct
import zipfile

MAX_FILE = 64 * 1024 * 1024
MAX_TOTAL = 512 * 1024 * 1024
MODEL_ENTRIES = {
    "relocatable_model_metadata": 1,
    "body": 2,
    "render_model": 3,
    "fighter_parameters": 0x12,
    "ai_first": 0x29,
    "ai_second": 0x2A,
    "ai_third": 0x2B,
    "english_lips_first": 0x35,
    "japanese_lips_first": 0x99,
}


def safe_name(name):
    if "\\" in name or ":" in name:
        raise ValueError("Unsupported archive path")
    path = PurePosixPath(name)
    if path.is_absolute() or ".." in path.parts or not path.parts:
        raise ValueError("Unsafe archive path")
    return path


def read_source(source):
    """Read one warrior directory or one single-root archive; never extract."""
    source = Path(source)
    files = {}
    total = 0

    def add(name, data):
        nonlocal total
        safe_name(name)
        key = name.casefold()
        if any(existing.casefold() == key for existing in files):
            raise ValueError("Duplicate resource path")
        total += len(data)
        if len(data) > MAX_FILE or total > MAX_TOTAL:
            raise ValueError("Warrior resources exceed inspection limits")
        files[name] = data

    if source.is_dir():
        for path in sorted(source.rglob("*")):
            if path.is_symlink():
                raise ValueError("Linked warrior resources are unsupported")
            if path.is_file():
                if path.stat().st_size > MAX_FILE:
                    raise ValueError("Resource exceeds inspection limit")
                add(path.relative_to(source).as_posix(), path.read_bytes())
        folder = source.name
    else:
        with zipfile.ZipFile(source) as archive:
            entries = archive.infolist()
            roots = {safe_name(entry.filename).parts[0] for entry in entries}
            if len(roots) != 1:
                raise ValueError("Expected exactly one warrior folder in ZIP")
            folder = roots.pop()
            for entry in entries:
                path = safe_name(entry.filename)
                if entry.is_dir():
                    continue
                if len(path.parts) < 2:
                    raise ValueError("Expected resources inside the warrior folder")
                if entry.file_size > MAX_FILE or total + entry.file_size > MAX_TOTAL:
                    raise ValueError("Resource exceeds inspection limit")
                add(PurePosixPath(*path.parts[1:]).as_posix(), archive.read(entry))
    return folder, files


def fields(data):
    result = {}
    if len(data) > 128 * 1024:
        raise ValueError("Text manifest exceeds inspection limit")
    for line in data.decode("utf-8-sig").splitlines():
        line = line.strip()
        if line and not line.startswith("#"):
            key, separator, value = line.partition("=")
            if not separator:
                raise ValueError("Malformed text manifest")
            result.setdefault(key.strip(), []).append(value.strip())
    return result


def pack_summary(data, model=False):
    """Describe the offset-table container read by BObjFile_GetEntry.

    Entries are one-based; low offset bits are flags. Report empty entries
    separately. Repeated/non-monotonic offsets are allowed, as in the loader.
    """
    if len(data) < 12:
        raise ValueError("Truncated pack")
    count = struct.unpack_from("<I", data)[0]
    if not 1 <= count <= 4096 or (count + 2) * 4 > len(data):
        raise ValueError("Invalid pack table")
    offsets = [value & ~3 for value in struct.unpack_from(f"<{count + 1}I", data, 4)]
    table_end = (count + 2) * 4
    if any(value and not table_end <= value <= len(data) for value in offsets):
        raise ValueError("Pack offset outside payload")
    boundaries = sorted(set(offsets + [len(data)]))

    def entry_size(index):
        if index > count:
            return None
        start, following = offsets[index - 1:index + 1]
        if not start or start == following or start == len(data):
            return 0
        end = next(boundary for boundary in boundaries if boundary > start)
        return end - start

    result = {"entries": count}
    if model:
        result["embedded_sections"] = {name: entry_size(index) for name, index in MODEL_ENTRIES.items()}
    return result


def inspect(source):
    folder, files = read_source(source)
    for required in ("warrior.txt", "mod.txt", ".warrior_built"):
        if required not in files:
            raise ValueError(f"Missing {required}")
    recipe, manifest = fields(files["warrior.txt"]), fields(files["mod.txt"])
    if manifest.get("mode") != ["new"]:
        raise ValueError("Only generated mode=new warriors are supported by this preflight")
    built = files[".warrior_built"].decode("utf-8-sig").splitlines()
    if not built or not built[0].startswith("#warrior "):
        raise ValueError("Unrecognized build marker")
    resources = []
    seen = set()
    for name in built[1:]:
        name = name.strip()
        if not name:
            continue
        safe_name(name)
        if name in seen:
            raise ValueError("Duplicate built resource")
        seen.add(name)
        if name not in files:
            raise ValueError(f"Missing generated resource: {name}")
        data = files[name]
        item = {"path": name, "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}
        if name.endswith(".pak"):
            item["pack"] = pack_summary(data, model=not name.endswith(("_anm.pak", "_eff.pak")))
        resources.append(item)
    if not resources:
        raise ValueError("Build marker lists no resources")
    references = []
    for key, values in manifest.items():
        if key.startswith("trans"):
            for value in values:
                target = value.rsplit(",", 1)[-1]
                if target.startswith("@"):
                    safe_name(target[1:])
                    if len(PurePosixPath(target[1:]).parts) != 1:
                        raise ValueError("Transformation target must name a folder")
                    references.append(target[1:])
    return {
        "folder": folder,
        "display_name": recipe.get("name", [folder])[-1],
        "build_marker": built[0],
        "template": manifest.get("template"),
        "voice": manifest.get("voice"),
        "transformation_folders": references,
        "resources": resources,
        "runtime_compatibility": "unverified",
    }


def inspect_set(sources):
    reports = [inspect(source) for source in sources]
    folders = [report["folder"] for report in reports]
    if len({folder.casefold() for folder in folders}) != len(folders):
        raise ValueError("Duplicate warrior folder identity")
    for report in reports:
        report["unresolved_transformation_folders"] = [
            target for target in report["transformation_folders"] if target not in folders
        ]
    return {"schema": 1, "installed": False, "warriors": reports}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sources", nargs="+", type=Path)
    parser.add_argument("--output", type=Path, help="Optional private JSON report; no game files are modified")
    args = parser.parse_args()
    try:
        result = inspect_set(args.sources)
    except (OSError, ValueError, zipfile.BadZipFile, UnicodeError) as error:
        parser.exit(1, f"Inspection failed: {error}\n")
    text = json.dumps(result, indent=2, ensure_ascii=False) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text, encoding="utf-8")
    else:
        print(text, end="")


if __name__ == "__main__":
    main()
