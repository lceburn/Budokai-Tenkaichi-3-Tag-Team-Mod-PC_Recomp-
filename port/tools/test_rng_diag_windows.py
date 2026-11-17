"""Test extracted production RNG functions with the release Windows compiler."""
from pathlib import Path
import subprocess
import sys
from test_effect_diag_windows import compile_one
from toolchain import ROOT,PREFIX
sys.path.insert(0,str(ROOT/'port/tests'))
from rng_diag import extract,prepare

def main():
    out=prepare()
    source='''#define PORT 1
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "sys/rand.h"
#include "port/vu0_a.h"
#include "battle/net_rng_diag.h"
unsigned long long gPortRandNext=1;
uint32_t gPortVu0R=0x3f800000u;
extern unsigned gRandState[624],gRandMag01[2];
extern int gRandIndex;
void Port_Srand(unsigned);
int Port_Rand(void);
float Rand_Float01(void);
'''
    for path,names in (('port/src/plat_sys.c',('Port_Srand','Port_Rand')),('port/src/mathf_pc.c',('bits','from','rnext','Rand_SeedFloat','Rand_Float01')),('src/sys/randf.c',('Rand_FloatRange','Rand_Libc','Rand_IntRange')),('src/sys/rand.c',('Rand_Seed','Rand_Next','Rand_Range'))):
        text=(ROOT/path).read_text()
        for name in names:source+=extract(text,name)
    (out/'current.c').write_text(source)
    for name,path in [('fixture','port/tests/rng_diag.c'),('baseline','port/build/rng-diag-tests/baseline.c'),('current','port/build/rng-diag-tests/current.c')]:
        compile_one(path,out/(name+'.o'),['-DPORT=1','-Dsrand=Port_Srand','-Drand=Port_Rand','-include','port/tests/effect_diag_noasm.h'])
    # Shared test arithmetic adapters, compiled with hardware floats so the
    # integer-bit soft-float ABI does not recursively call itself. These are
    # deliberately not an independent PS2 arithmetic implementation.
    (out/'adapters.c').write_text('''#include <stdint.h>
typedef union {uint32_t u;float f;} F;
uint32_t __addsf3(uint32_t a,uint32_t b){F x={a},y={b},z;z.f=x.f+y.f;return z.u;}
uint32_t __subsf3(uint32_t a,uint32_t b){F x={a},y={b},z;z.f=x.f-y.f;return z.u;}
uint32_t __mulsf3(uint32_t a,uint32_t b){F x={a},y={b},z;z.f=x.f*y.f;return z.u;}
int __gesf2(uint32_t a,uint32_t b){F x={a},y={b};return x.f==y.f?0:x.f>y.f?1:-1;}
int __lesf2(uint32_t a,uint32_t b){return __gesf2(a,b);}
int __ltsf2(uint32_t a,uint32_t b){return __gesf2(a,b);}
int __eqsf2(uint32_t a,uint32_t b){F x={a},y={b};return x.f!=y.f;}
unsigned __fixunssfsi(uint32_t a){F x={a};return (unsigned)x.f;}
''')
    subprocess.run([PREFIX+'gcc','-c','-O2',str(out/'adapters.c'),'-o',str(out/'adapters.o')],check=True)
    subprocess.run([PREFIX+'gcc','-o',str(out/'check.exe')]+[str(out/(n+'.o')) for n in ('fixture','baseline','current','adapters')]+['-Wl,--gc-sections','-Wl,--image-base,0x20000000','-Wl,--disable-dynamicbase','-static-libgcc'],check=True)
    print(out/'check.exe')

if __name__=='__main__':main()
