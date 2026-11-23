#include "../src/gs/tag_practice_clock.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    uint64_t clock;unsigned frame,tick,oldFrame=0;uint64_t oldClock=0;
    clock=0;frame=0;
    for(tick=1;tick<=36000;tick++) {
        uint64_t now=(uint64_t)tick*1000/60;
        unsigned oldSteps=(unsigned)((now-oldClock)*60/1000);
        oldClock+=(oldSteps*1000+59)/60;oldFrame+=oldSteps;
        frame=TagPracticeClock_Target(&clock,now,frame);
        assert(frame==now*60/1000);
    }
    assert(frame==36000 && oldFrame<frame-600);
    // Frequent and irregular native menu polling must not lose fractional time.
    clock=0;frame=0;
    for(tick=0;tick<=600000;tick+=7) {
        frame=TagPracticeClock_Target(&clock,tick,frame);
        assert(frame==(uint64_t)tick*60/1000);
    }
    // A stalled host catches up in bounded batches, preserving its time debt.
    clock=0;frame=TagPracticeClock_Target(&clock,1000,0);assert(frame==16);
    frame=TagPracticeClock_Target(&clock,1000,frame);assert(frame==32);
    frame=TagPracticeClock_Target(&clock,1000,frame);assert(frame==48);
    frame=TagPracticeClock_Target(&clock,1000,frame);assert(frame==60);
    assert(TagPracticeClock_Target(&clock,1000,frame)==60);
    // A reset clock never converts negative elapsed time into a huge advance.
    assert(TagPracticeClock_Target(&clock,0,frame)==frame && clock==0);
    puts("Practice pacing: sustained 60 Hz, irregular polling, bounded catch-up and clock reset passed.");
}
