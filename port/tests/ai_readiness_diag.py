"""Verify production readiness instrumentation against the previous condition."""
from pathlib import Path
import re,subprocess,argparse
ROOT=Path(__file__).resolve().parents[2]
def extract(text,name):
    m=re.search(r'^(?:static )?s32 '+name+r'\([^;]*?\)\s*\{',text,re.M)
    start=m.start();end=text.index('{',start)+1;depth=1
    while depth:
        if text[end]=='{':depth+=1
        elif text[end]=='}':depth-=1
        end+=1
    return text[start:end]+'\n'
def prepare():
    out=ROOT/'port/build/ai-readiness-diag';out.mkdir(exist_ok=True)
    production=(ROOT/'src/battle/btl_ai_act.c').read_text()
    # Frozen pre-observer condition, including its exact query order. Keep this
    # independent of the instrumented source so moving/eager queries fail.
    baseline='''s32 BtlAiSense_CheckBit35(AiActSide *s) {
    AiActStatus *st=&s->st;
    f32 progress=BtlCharApi_GetTechniqueProgress(TagTeam_Target(s->side));
    if(!BtlCharApi_GetOppSkillKind(s->side))return 0;
    if(st->react&0x400)return 0;
    if(progress==-1.0f)return 0;
    if(BtlCharApi_IsUnk1070Set(s->side))return 0;
    if(1.0f<progress)return 0;
    return 1;
}'''
    source='''#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
typedef int s32;typedef unsigned u32;typedef float f32;
typedef struct {u32 react;} AiActStatus;
typedef struct {int side;AiActStatus st;} AiActSide;
static int kind,cooldown,active,seen,trace[8],n;
static float progress;static unsigned captured[7];
int ND_AiActive(void){return active;}
void ND_AiCheck35(int actor,const unsigned *p,unsigned count){assert(actor==4&&count==7);seen++;memcpy(captured,p,28);}
int TagTeam_Target(int side){assert(side==4);trace[n++]=1;return 3;}
float BtlCharApi_GetTechniqueProgress(int target){assert(target==3);trace[n++]=2;return progress;}
int BtlCharApi_GetOppSkillKind(int side){assert(side==4);trace[n++]=3;return kind;}
int BtlCharApi_IsUnk1070Set(int side){assert(side==4);trace[n++]=4;return cooldown;}
'''
    source+=extract(baseline,'BtlAiSense_CheckBit35').replace('BtlAiSense_CheckBit35','baseline35')
    source+=extract(production,'ai_check35_result')
    source+=next(l for l in production.splitlines() if l.startswith('#define AI_CHECK35_RETURN(value,why) return ai_check35_result'))+'\n'
    source+=extract(production,'BtlAiSense_CheckBit35')
    source+='''int main(void){
float values[]={-2,-1,-0.0f,0,0.5f,1,1.01f,INFINITY,NAN};
int kinds[]={-1,0,1,2,257},cooldowns[]={0,1,-1};unsigned reacts[]={0,1,0x400,0x401};
int a,b,c,d,base,got,expected[8],count,cases=0;AiActSide s={4,{0}};
for(active=0;active<2;active++)for(a=0;a<9;a++)for(b=0;b<5;b++)for(c=0;c<3;c++)for(d=0;d<4;d++){
 progress=values[a];kind=kinds[b];cooldown=cooldowns[c];s.st.react=reacts[d];
 n=0;seen=0;base=baseline35(&s);count=n;memcpy(expected,trace,sizeof(trace));
 n=0;got=BtlAiSense_CheckBit35(&s);assert(base==got&&n==count&&!memcmp(trace,expected,n*sizeof(int)));
 assert(seen==active);if(active){unsigned bits;memcpy(&bits,&progress,4);assert(captured[0]==3&&captured[1]==bits&&captured[2]==(unsigned)kind&&captured[3]==s.st.react&&captured[5]==got);assert(captured[4]==(count==4?(unsigned)cooldown:0x80000000u));}
 cases++;
}
printf("PASS: %d readiness cases preserve result, original query order, short-circuiting, float bits and diagnostic opt-out.\\n",cases);return 0;}
'''
    (out/'check.c').write_text(source)
    return out
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--prepare',action='store_true');a=p.parse_args();out=prepare()
    if not a.prepare:
        subprocess.run(['cc','-std=gnu99','-O2','-fsanitize=address,undefined',str(out/'check.c'),'-o',str(out/'check')],check=True)
        subprocess.run([out/'check'],check=True)
