/* Host-only practice coordinator uses the game's 59.94 Hz blank period.
   Only confirmed advancement consumes time; waiting for a guest retains debt. */
#ifndef TAG_PRACTICE_CLOCK_H
#define TAG_PRACTICE_CLOCK_H
#include <stdint.h>
#define TAG_PRACTICE_BLANK_NS UINT64_C(16683350)
static unsigned TagPracticeClock_Target(uint64_t *clock,uint64_t now,unsigned frame) {
    uint64_t steps;
    if(now<*clock){*clock=now;return frame;}
    steps=(now-*clock)/TAG_PRACTICE_BLANK_NS;
    if(steps>16)steps=16;
    return frame+(unsigned)steps;
}
static void TagPracticeClock_Advance(uint64_t *clock,unsigned steps) {
    *clock+=(uint64_t)steps*TAG_PRACTICE_BLANK_NS;
}
#endif
