#ifndef SYS_JOB_H
#define SYS_JOB_H

#include "types.h"
#include "sys/queue.h"

#ifdef PORT
/* Queue_AllocNode reserves one node. Allow every actor plus stage work;
   the fixed battle-job pool is drained in batches by BtlLoad_Reload. */
#include "battle/tag_capacity.h"
#define JOB_MAX (TAG_ACTOR_CAPACITY + 2)
#else
#define JOB_MAX 4
#endif

/* A queued job: any object whose second word is its step function. */
typedef struct Job {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 (*step)(struct Job *job); /* called once per frame; returns 1 when the job is finished */
} Job;

extern Queue *gJobQueue;

void Job_Clear(void);
void Job_Init(void);
void Job_Destroy(void);
s32 Job_Run(void);
s32 Job_Push(Job *job);
s32 Job_GetCount(void);

#endif
