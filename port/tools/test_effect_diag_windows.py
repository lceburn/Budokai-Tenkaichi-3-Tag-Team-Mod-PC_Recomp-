"""Cross-compile production list regressions with the actual ptr32 ABI and a diagnostic stop harness."""
from pathlib import Path
import subprocess
import undefined
from toolchain import ROOT, PREFIX

def compile_one(source, output, more=()):
    command=['gcc','-m32','-std=gnu99','-c','-O2','-g','-fno-pic','-fno-strict-aliasing',
             '-ffunction-sections','-fdata-sections','-msoft-float','-mno-sse','-mno-mmx',
             '-w','-Iinclude','-Iport/include','-Iport/src']+list(more)
    result=undefined.compile_ee(command,ROOT/source,output)
    if result.returncode:raise SystemExit(result.stderr)

def main():
    out=ROOT/'port/build/effect-diag-win';out.mkdir(exist_ok=True)
    # The soft-float backend emits one text section, so GC cannot discard the
    # unrelated sprite renderer. Extract the exact production task section,
    # preserving original line locations; no copied implementation is tested.
    source=(ROOT/'src/battle/eft_ae.c').read_text()
    begin=source.index('void BtlTaskList_Reset(' ,source.index('/* Runs the reset callback'))
    end=source.index('\n}\n',begin)+3
    tasks=source.index('/* Creates the root list. No caller')
    prefix='''#define PORT 1
#include "battle/effect_diag.h"
#include "common.h"
#include "battle/eft_ae.h"
typedef Vec4 EftAeVec;
extern void Vec4_Copy(Vec4 *,Vec4 *);
extern int BtlPool_GetCurrent(void);
extern void *BtlPool_Alloc(int,int);
extern void BtlPool_Free(int,void *);
BtlTaskList *gBtlTaskRoot;
BtlTask *gBtlTaskCur;
BtlTaskList *BtlTaskList_Create(BtlTask *,unsigned,unsigned);
void BtlTaskList_Destroy(BtlTaskList *);
void BtlTaskList_KillAll(BtlTaskList *);
void BtlTask_Kill(BtlTask *);
void BtlTask_SetDead(BtlTask *);
void BtlTask_Init(BtlTask *,BtlTaskCls *,void *);
BtlTask *BtlTask_AddBefore(BtlTask *,BtlTaskCls *,void *);
BtlTask *BtlTask_AddAfter(BtlTask *,BtlTaskCls *,void *);
'''
    wrapper='#line 1 "src/battle/eft_ae.c"\n'+prefix
    wrapper+=f'#line {source[:begin].count(chr(10))+1} "src/battle/eft_ae.c"\n'+source[begin:end]
    task_end=source.index('\n}',source.index('BtlTask *BtlTask_GetParent('))+2
    wrapper+=f'#line {source[:tasks].count(chr(10))+1} "src/battle/eft_ae.c"\n'+source[tasks:task_end]
    (out/'task-section.c').write_text(wrapper)
    compile_one('port/build/effect-diag-win/task-section.c',out/'tasks.o',['-DPORT=1','-include','port/tests/effect_diag_noasm.h'])
    compile_one('port/tests/effect_diag.c',out/'test.o')
    compile_one('port/tests/effect_diag_stop.c',out/'stop.o')
    for name,objects in [('lifecycle',['tasks.o','test.o']),('stop',['stop.o'])]:
        subprocess.run([PREFIX+'gcc','-o',str(out/(name+'.exe'))]+[str(out/o) for o in objects]+
                       ['-Wl,--gc-sections','-Wl,--image-base,0x20000000','-Wl,--disable-dynamicbase','-static-libgcc'],check=True)
    print(out)
if __name__=='__main__':main()
