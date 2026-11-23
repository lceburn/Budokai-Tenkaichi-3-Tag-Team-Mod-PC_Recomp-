#include "../src/gs/tag_practice_clock.h"
#include <assert.h>
#include <stdio.h>
static unsigned advance(uint64_t *clock,uint64_t now,unsigned frame,unsigned limit) {
    unsigned target=TagPracticeClock_Target(clock,now,frame);
    if(target>limit)target=limit;
    if(target>frame){TagPracticeClock_Advance(clock,target-frame);frame=target;}
    return frame;
}
int main(void) {
    uint64_t clock=0;unsigned frame=0,tick;
    for(tick=1;tick<=36000;tick++) {
        uint64_t now=(uint64_t)tick*TAG_PRACTICE_BLANK_NS;
        frame=advance(&clock,now,frame,~0u);assert(frame==tick && clock==now);
    }
    clock=0;frame=0;
    for(tick=0;tick<=600000;tick+=7) {
        uint64_t now=(uint64_t)tick*1000000;
        frame=advance(&clock,now,frame,~0u);assert(frame==now/TAG_PRACTICE_BLANK_NS);
    }
    // Production's confirmation clamp must not discard elapsed clock ticks.
    clock=0;frame=advance(&clock,1000000000,0,3);assert(frame==3);
    assert(clock==3*TAG_PRACTICE_BLANK_NS);
    frame=advance(&clock,1000000000,frame,3);assert(frame==3);
    while(frame<59)frame=advance(&clock,1000000000,frame,~0u);
    assert(frame==59 && clock==59*TAG_PRACTICE_BLANK_NS);
    // Intermittent missing guest inputs, on the native menu's 33ms polling.
    clock=0;frame=0;
    for(tick=1;tick<=18000;tick++) {
        unsigned due=tick*2,limit=due;
        if(tick%100==0)limit=frame; // temporary input stall
        frame=advance(&clock,(uint64_t)due*TAG_PRACTICE_BLANK_NS,frame,limit);
        if(tick%100)assert(frame==due);
    }
    frame=advance(&clock,36000*TAG_PRACTICE_BLANK_NS,frame,~0u);assert(frame==36000);
    assert(TagPracticeClock_Target(&clock,0,frame)==frame && clock==0);
    puts("Practice clock: native blank rate, irregular polling, confirmation stalls and catch-up passed.");
}
