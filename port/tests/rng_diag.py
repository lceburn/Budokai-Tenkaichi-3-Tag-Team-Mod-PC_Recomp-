"""Compile production RNG code and retained Beta 14 baselines without disc data."""
from pathlib import Path
import re
import subprocess

ROOT=Path(__file__).resolve().parents[2]
BASE='0b6b9651ddc647ad5697e6cc0170706b5e1b3c0e'


def extract(text,name):
    m=re.search(r'^(?:static\s+)?(?:void|int|float|f32|uint32_t|u32|s32)\s+'+name+r'\([^;]*?\)\s*\{',text,re.M)
    if not m:raise ValueError(name)
    start=m.start();pos=text.index('{',m.start());depth=1;end=pos+1
    while depth:
        if text[end]=='{':depth+=1
        elif text[end]=='}':depth-=1
        end+=1
    return text[start:end]+'\n'


def prepare():
    out=ROOT/'port/build/rng-diag-tests';out.mkdir(exist_ok=True)
    old=lambda p:subprocess.check_output(['git','show',BASE+':'+p],cwd=ROOT,text=True)
    source='#include <stdint.h>\n#include "types.h"\n#include "sys/rand.h"\n#include "port/vu0_a.h"\nextern unsigned long long baselineLibc;\nuint32_t baselineVu0=0x3f800000u;\nextern unsigned baselineState[624],baselineMag[2];\nextern int baselineIndex;\nvoid baselineSrand(unsigned);\nvoid Rand_Seed(unsigned);\n'
    groups=(('port/src/plat_sys.c',('Port_Srand','Port_Rand')),('port/src/mathf_pc.c',('bits','from','rnext','Rand_SeedFloat','Rand_Float01')),('src/sys/randf.c',('Rand_FloatRange','Rand_Libc','Rand_IntRange')),('src/sys/rand.c',('Rand_Next','Rand_Range')))
    for path,names in groups:
        text=old(path)
        for name in names:source+=extract(text,name)
    replacements={'gPortRandNext':'baselineLibc','gPortVu0R':'baselineVu0','Port_Srand':'baselineSrand','Port_Rand':'baselineRand','Rand_SeedFloat':'baselineSeedFloat','Rand_Float01':'baselineFloat01','Rand_FloatRange':'baselineFloatRange','Rand_Libc':'baselineLibcWrap','Rand_IntRange':'baselineIntRange','Rand_Next':'baselineNext','Rand_Range':'baselineRange','gRandState':'baselineState','gRandIndex':'baselineIndex','gRandMag01':'baselineMag','srand':'baselineSrand','rand':'baselineRand'}
    for oldname,newname in replacements.items():source=re.sub(r'\b'+oldname+r'\b',newname,source)
    source=source.replace('sys/baselineRand.h','sys/rand.h')
    source='#include <string.h>\n'+source
    # The fallback seed path is not entered by this equivalence test.
    (out/'baseline.c').write_text(source)
    return out


def main():
    out=prepare()
    subprocess.run(['cc','-std=gnu99','-O2','-w','-DPORT=1','-Dsrand=Port_Srand','-Drand=Port_Rand','-include','port/tests/effect_diag_noasm.h','-Iinclude','-Iport/include','-Iport/src','-ffunction-sections','-fdata-sections','port/tests/rng_diag.c',str(out/'baseline.c'),'port/src/plat_sys.c','port/src/mathf_pc.c','src/sys/rand.c','src/sys/randf.c','-Wl,--gc-sections','-lm','-o',str(out/'check')],cwd=ROOT,check=True)
    subprocess.run([out/'check'],cwd=out,check=True)


if __name__=='__main__':main()
