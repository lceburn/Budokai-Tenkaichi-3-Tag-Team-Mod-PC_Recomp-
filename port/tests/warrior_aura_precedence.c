#include "../../src/battle/btl_tech_a.c"
#include <assert.h>
#include <stdio.h>
static BtlTechObj testObj;static BtlParam testParam;static int override=-1;
BtlTechObj *BtlChar_GetObj(BtlTechChr *chr){(void)chr;return &testObj;}
s32 BtlObj_GetWarriorAuraPreset(void *obj){assert(obj==&testObj);return override;}
s32 BtlMember_HasAbility(BtlTechChr *chr,s32 n){(void)chr;return n==0x54;}
int main(void) {
    testObj.param=&testParam;testParam.auraKind=5;
    assert(BtlParam_GetAuraKind(NULL)==8);
    override=10;assert(BtlParam_GetAuraKind(NULL)==10);
    override=0;assert(BtlParam_GetAuraKind(NULL)==0);
    override=-1;assert(BtlParam_GetAuraKind(NULL)==8);
    puts("PASS explicit aura wins over inherited ability; Default restores native aura behavior");return 0;
}
