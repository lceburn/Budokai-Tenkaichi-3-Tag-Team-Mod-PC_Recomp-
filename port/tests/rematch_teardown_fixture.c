#include "battle/eft_h.h"
#include "battle/effect_diag.h"
#include <assert.h>
#include <stdio.h>
static EftHChar chars[15];
static EftHMgr manager;
EftHMgr *gEftShot=&manager;
static EftHTask task;
static int invalidated,killed,pool,marked;
int EDiag_Enabled(void){return 1;}
void EDiag_Char(int a,int state,const char *site){assert(state==0);marked++;}
void EDiag_Unbind(int a,int s,const char *site){invalidated++;}
void BtlScene_ResetCharacter(int a){}
void EftShot_SetCharPool(int a){pool=a;}
void BtlTask_Kill(EftHTask *t){assert(t==&task);killed++;}
void EftShot_DestroyChar(s32 chr);
int main(void){manager.chars=chars;
for(int a=0;a<15;a++){
    chars[a].task=&task;chars[a].list=&task;chars[a].unk530=3;
    for(int s=0;s<5;s++)chars[a].slot[s].task=&task;
    EftShot_DestroyChar(a);
    assert(!chars[a].task&&!chars[a].list&&chars[a].unk530==-1&&pool==a);
    for(int s=0;s<5;s++)assert(!chars[a].slot[s].task);
}
assert(invalidated==75&&killed==15&&marked==15);
puts("PASS: production character teardown invalidates all five effect bindings for every supported fighter.");}
