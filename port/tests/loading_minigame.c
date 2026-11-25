/* The production screen must use local input without advancing fight RNG. */
#include <assert.h>
#include <stdio.h>
#include "../../src/sys/loading.c"
LoadScreen gLoadScreen;
Progress *gProgress;
static int network=1,localPresses,randomCalls;
s32 Port_NetActive(void){return network;}
s32 Port_LoadMiniInput(s32 reset){int n=localPresses;localPresses=0;return reset?0:n;}
u32 Rand_Range(u32 count){randomCalls++;return count-1;}
s32 Rand_IntRange(s32 lo,s32 hi){randomCalls++;return hi;}
LoadPad gPad[2];
int main(void) {
    gLoadScreen.type=LOAD_TYPE_1;gLoadScreen.presses=2;gLoadScreen.cycles=1;
    localPresses=1;Load_ReadInput();assert(gLoadScreen.presses==1);
    localPresses=1;Load_ReadInput();assert(gLoadScreen.presses==0);
    Load_UpdateScreen();assert(gLoadScreen.frame==1 && gLoadScreen.presses==3);
    assert(!gPad[0].gamePressed && !gPad[1].gamePressed);
    gLoadScreen.type=LOAD_TYPE_2;gLoadScreen.frame=0;gLoadScreen.timer=20;gLoadScreen.period=12;
    gLoadScreen.flags=LOAD_FLAG_TAKEN;gLoadScreen.count=1;
    Load_UpdateScreen();assert(randomCalls==0);
    // Ordinary offline loading keeps the normal game-controller path.
    network=0;gLoadScreen.presses=2;gPad[0].gamePressed=LOAD_BUTTON;Load_ReadInput();
    assert(gLoadScreen.presses==1);
    puts("PASS: network minigame accepts local presses, preserves synchronized pad state and fight RNG; offline path retained");
}
