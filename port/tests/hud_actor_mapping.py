"""Exercise the production local-view and target selectors without renderer state."""
from pathlib import Path
import argparse
import subprocess

ROOT=Path(__file__).resolve().parents[2]

def function(source,name):
    start=source.index('s32 '+name+'(')
    opening=source.index('{',start);depth=1;end=opening+1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]

def main():
    p=argparse.ArgumentParser();p.add_argument('--prepare',action='store_true');args=p.parse_args()
    source=(ROOT/'src/battle/btl_cam.c').read_text()
    output=ROOT/'port/build/hud-actor-mapping.c'
    output.write_text('''#include <assert.h>
typedef int s32;typedef unsigned u32;
enum {TN_PLAYERS=10};
typedef struct {int size[2];} TagNetConfig;
static int active=1,online=1,role,observer,watch,own[10],target[10];
static TagNetConfig config={{4,5}};
int TagNet_Role(void){return role;}
int TagNet_Active(void){return online;}
int TagTeam_Active(void){return active;}
int TagNet_Observer(void){return observer;}
int TagTeam_ViewActor(int v){assert(v>=0 && v<10);return own[v];}
int TagNet_View(void){return watch;}
int TagFusion_Owner(int actor){return actor;}
void TagNet_CopyConfig(u32 ignored){(void)ignored;}
int TagTeam_HudTarget(int actor){assert(actor>=0 && actor<10);return target[actor];}
'''+function(source,'TagOnline_LocalActor').replace('TagNet_CopyConfig((u32)&c);','c=config;')+'\n'+
        function(source,'TagTeam_ScriptHudActor')+'''
int main(void) {
    for(int i=0;i<10;i++){own[i]=i;target[i]=(i+1)%10;}
    own[0]=2;own[1]=5;target[2]=7;target[5]=8;
    role=0;assert(TagTeam_ScriptHudActor(0)==2 && TagTeam_ScriptHudActor(1)==7);
    role=1;assert(TagTeam_ScriptHudActor(0)==5 && TagTeam_ScriptHudActor(1)==8);
    target[5]=3;assert(TagTeam_ScriptHudActor(1)==3);
    target[5]=-1;assert(TagTeam_ScriptHudActor(1)==-1);
    observer=1;watch=4;target[1]=6;
    assert(TagTeam_ScriptHudActor(0)==1 && TagTeam_ScriptHudActor(1)==6);
    online=0;assert(TagTeam_ScriptHudActor(0)==2 && TagTeam_ScriptHudActor(1)==7);
    active=0;assert(TagTeam_ScriptHudActor(0)==0 && TagTeam_ScriptHudActor(1)==1);
}
''')
    if args.prepare:print(output);return
    exe=output.with_suffix('')
    subprocess.run(['cc','-std=c99','-O1','-g','-fsanitize=address,undefined',str(output),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
    print('Host/client HUD mapping, target changes, hidden targets, spectators and legacy fallback passed.')

if __name__=='__main__':main()
