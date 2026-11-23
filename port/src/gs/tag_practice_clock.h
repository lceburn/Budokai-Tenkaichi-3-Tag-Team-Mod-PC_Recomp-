/* Host-only practice coordinator clock, in 1/60 millisecond units. Keeping
   the remainder avoids rounding every 16.666ms frame up to 17ms. */
#ifndef TAG_PRACTICE_CLOCK_H
#define TAG_PRACTICE_CLOCK_H
#include <stdint.h>
static unsigned TagPracticeClock_Target(uint64_t *clock,uint64_t now,unsigned frame) {
    uint64_t scaled=now*60,steps;
    if(scaled<*clock){*clock=scaled;return frame;}
    steps=(scaled-*clock)/1000;
    if(steps>16)steps=16;
    *clock+=steps*1000;
    return frame+(unsigned)steps;
}
#endif
