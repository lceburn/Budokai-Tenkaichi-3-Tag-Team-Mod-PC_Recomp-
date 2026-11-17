"""Retain matching DWARF, map and provenance for an effect diagnostic release."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--commit', required=True)
    parser.add_argument('--version', default='0.1.0-beta.12')
    parser.add_argument('--instructions', type=Path, default=ROOT/'docs/REMATCH_EFFECT_DIAGNOSTIC.md')
    parser.add_argument('--objcopy', default='x86_64-w64-mingw32-objcopy')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    build = ROOT/'port/build'
    exe = build/'bt3.exe'
    source = json.loads((build/'obj_win64/tag-net-build.json').read_text())
    record = dict(schema='effect-diagnostic-build-v1', source_commit=args.commit,
                  branch='integration/z3xox-main', version=args.version,
                  executable_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),
                  executable_size=exe.stat().st_size, symbols='DWARF (no PDB)',
                  build=source)
    record['compiler'] = subprocess.check_output(['clang','--version'],text=True).splitlines()[0]
    record['linker'] = subprocess.check_output(['x86_64-w64-mingw32-ld','--version'],text=True).splitlines()[0]
    with tempfile.TemporaryDirectory() as temp:
        debug = Path(temp)/'TagTeam.exe.debug'
        subprocess.run([args.objcopy,'--only-keep-debug',str(exe),str(debug)],check=True)
        with zipfile.ZipFile(args.output,'w',zipfile.ZIP_DEFLATED) as archive:
            archive.write(debug,debug.name)
            archive.write(build/'bt3.exe.map','TagTeam.exe.map')
            archive.writestr('build.json',json.dumps(record,indent=2)+'\n')
            archive.write(args.instructions,'Instructions.md')
    print(json.dumps({k:v for k,v in record.items() if k!='build'},indent=2))


if __name__ == '__main__':
    main()
