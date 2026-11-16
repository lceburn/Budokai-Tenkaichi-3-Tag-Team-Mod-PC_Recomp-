#include "common.h"
#include "sys/job.h"
#include "sys/heap.h"

/* libc */
extern void *memset(void *dst, s32 value, u32 size);

/* Drops every queued job. */
void Job_Clear(void) {
    Queue_Clear(gJobQueue);
}

/* Allocates the job queue and empties it (one node is reserved by Queue). */
void Job_Init(void) {
    gJobQueue = Heap_Alloc(sizeof(Queue), 0x20, 0, HEAP_ANY);
    memset(gJobQueue, 0, sizeof(Queue));
    Queue_Init(gJobQueue, JOB_MAX);
    Job_Clear();
}

/* Frees the job queue. No callers. */
void Job_Destroy(void) {
    Queue_Destroy(gJobQueue);
    Heap_Free(gJobQueue);
    gJobQueue = NULL;
}

/* Steps the oldest job and removes it when it reports 1; returns 0 when the queue was empty, 1 otherwise. */
s32 Job_Run(void) {
    Job *job = Queue_Peek(gJobQueue);

    if (job == NULL) {
        return 0;
    }
    if (job->step(job) == 1) {
        Queue_Pop(gJobQueue);
    }
    return 1;
}

/* Queues a job; if nothing was ahead of it, steps it once right away and returns 1. */
s32 Job_Push(Job *job) {
#ifdef PORT
    if (Queue_Push(gJobQueue, job) == NULL) {
        extern void Port_TagTeamFatal(const char *);
        Port_TagTeamFatal("The loader job queue is full. Loading stopped before an incomplete battle could start.");
        return 0;
    }
#else
    Queue_Push(gJobQueue, job);
#endif
    if (Queue_Count(gJobQueue) != 1) {
        return 0;
    }
    job->step(job);
    return 1;
}

/* Number of queued jobs. No callers. */
s32 Job_GetCount(void) {
    return Queue_Count(gJobQueue);
}
