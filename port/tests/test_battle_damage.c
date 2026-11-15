#define PORT 1
#include "../../src/battle/btl_char_ctl_b.c"
#include <assert.h>
#include <stdio.h>
static BtlCtlRoster roster;
BtlCtlRoster *gBtlChars=&roster;
int main(void) {
    BtlChange_Reset();
    assert(!TagDamage_ShouldChange(501,1000,0,0));
    assert(TagDamage_ShouldChange(500,1000,0,0));
    assert(TagDamage_ShouldChange(1,1000,0,0));
    assert(!TagDamage_ShouldChange(0,1000,0,0));
    assert(!TagDamage_ShouldChange(400,1000,1,0));
    assert(!TagDamage_ShouldChange(400,1000,0,1));
    assert(!TagDamage_ShouldChange(1,0,0,0));
    assert(BtlChange_RequestBattleDamage(7,10,2));
    assert(!BtlChange_RequestBattleDamage(8,11,0));
    BtlChange_Update();assert(BtlChange_IsBattleDamage(7));
    assert(!BtlChange_IsBattleDamage(0));
    assert(roster.change.cur->variant==1 && roster.change.cur->animChara==-1);
    assert(roster.change.timeStop && roster.change.state==BTL_CHANGE_STARTED);
    BtlChange_SetTaken();BtlChange_SetLoaded();BtlChange_Update();
    assert(roster.change.state==BTL_CHANGE_READY);
    BtlChange_SetDone(7);BtlChange_Update();
    assert(!roster.change.timeStop && !BtlChange_IsBattleDamage(7));
    BtlChange_RequestChara(3,1,0,0,-1,-1,-1);BtlChange_Update();
    assert(!BtlChange_IsBattleDamage(3));
    BtlChange_SetLoaded();BtlChange_Update();assert(roster.change.state==BTL_CHANGE_LOADED);
    assert(!BtlChange_RequestBattleDamage(7,10,2));
    puts("PASS: health damage threshold, clothing exclusions, model-only queue, automatic readiness/completion and normal request isolation.");
}
