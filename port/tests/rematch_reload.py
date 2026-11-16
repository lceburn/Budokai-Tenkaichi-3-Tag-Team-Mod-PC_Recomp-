"""Asset-free regressions using the production reload, scheduler and teardown code."""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def function(text, name):
    # Find definitions, rather than a prototype or call.
    import re
    match = re.search(r'^(?:s32|void|BtlJobPool \*|BtlJob \*)\s*'+name+r'\([^;]*?\) \{', text, re.M)
    if not match:
        raise ValueError(name)
    start = match.start()
    end = text.index('\n}', match.end()) + 2
    return text[start:end]+'\n'


def main():
    out = ROOT/'port/build/rematch-regressions'
    out.mkdir(exist_ok=True)
    loader = (ROOT/'src/battle/battle_load.c').read_text()
    fixture = (ROOT/'port/tests/rematch_reload_fixture.c').read_text()
    functions = '\n'.join(function(loader,n) for n in
                          ('BtlJob_GetPool','BtlJob_InitPool','BtlJob_Alloc','BtlJob_Free','BtlJob_GetFreeCount','BtlLoad_Reload'))
    (out/'reload.c').write_text(fixture+'\n'+functions)
    base = ['cc','-DPORT=1','-w','-std=gnu99','-ffunction-sections','-fdata-sections',
            '-Iinclude','-Iport/include','-Iport/src','-Wl,--gc-sections']
    subprocess.run(base+[str(out/'reload.c'),'src/sys/queue.c','src/sys/job.c','src/sys/list.c',
                         '-o',str(out/'reload')],cwd=ROOT,check=True)
    subprocess.run([out/'reload'],check=True)
    overflow = subprocess.run([out/'reload','overflow'],capture_output=True,text=True)
    assert overflow.returncode == 86 and 'queue is full' in overflow.stderr
    teardown = (ROOT/'port/tests/rematch_teardown_fixture.c').read_text()
    teardown += '\n'+function((ROOT/'src/battle/eft_h.c').read_text(),'EftShot_DestroyChar')
    (out/'teardown.c').write_text(teardown)
    subprocess.run(base+[str(out/'teardown.c'),'-o',str(out/'teardown')],cwd=ROOT,check=True)
    subprocess.run([out/'teardown'],check=True)
    print('PASS: production queue overflow stops explicitly rather than dropping work.')


if __name__ == '__main__':
    main()
