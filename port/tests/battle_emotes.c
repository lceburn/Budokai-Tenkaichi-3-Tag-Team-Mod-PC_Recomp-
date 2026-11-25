#include <assert.h>
#include <stdio.h>
#include "../../src/battle/battle_emotes.c"
int Port_SpeedMilli(void){return 1000;}
int main(void) {
    const unsigned chord=BTLB_LOCKON|BTLB_GUARD;
    float frame=0;
    BtlEmote_Reset();
    assert(!BtlEmote_Input(0,chord,0,1) && !BtlEmote_Pose(0,&frame));
    BtlEmote_Input(0,0,1,1);
    assert(BtlEmote_Input(0,chord,1,1) && BtlEmote_Pose(0,&frame)==1 && frame==0);
    assert(BtlEmote_Input(0,chord,1,1));assert(BtlEmote_Pose(0,&frame)==1 && frame==2);
    assert(!BtlEmote_Input(0,0,1,1));assert(BtlEmote_Pose(0,&frame)==1 && frame==4);
    // Snapshot/replay gives exactly the same emote state for the same pad samples.
    BtlEmoteState snapshot=sEmotes[0];
    BtlEmote_Input(0,0,1,1);BtlEmoteState expected=sEmotes[0];sEmotes[0]=snapshot;
    BtlEmote_Input(0,0,1,1);assert(sEmotes[0].steps==expected.steps && sEmotes[0].selected==expected.selected);
    assert(!BtlEmote_Input(0,BTLB_UP,1,1) && !BtlEmote_Pose(0,&frame));
    BtlEmote_Input(1,0,1,27);assert(BtlEmote_Input(1,chord,1,27));
    assert(BtlEmote_Pose(1,&frame)==27 && !BtlEmote_Pose(0,&frame));
    assert(!BtlEmote_Input(1,chord,1,28) && !BtlEmote_Pose(1,&frame));
    assert(!BtlEmote_Input(-1,chord,1,1) && !BtlEmote_Input(BTL_OBJ_MAX,chord,1,1));
    BtlEmote_Reset();assert(!BtlEmote_Pose(1,&frame));
    puts("PASS: emote idle gating, edge activation, movement cancellation, independent fighters, rollback replay and reset");
}
