#include "battle/battle_work.h"
#include "sys/job.h"
#include "sys/heap.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Queue *gJobQueue;
BtlJobPool gBtlJobPool;
static BattleRes resources;
static BattleWork work;
static BattleMember members[15];
static int actors,stageJobs,drains,reloaded,freed,rebound;
static unsigned changed;
static int ready[15];
#define BATTLE_RES (&resources)

void *Heap_Alloc(s32 n,u32 a,s32 tail,s32 heap){return calloc(1,n);}
void Heap_Free(void *p){free(p);}
void Port_TagTeamFatal(const char *s){fprintf(stderr,"%s\n",s);exit(86);}
BattleWork *Battle_GetWork(void){return &work;}
void BtlRes_CommitReloadEx(void){}
int Battle_GetStartStage(void){return 0;}
int TagTeam_Count(void){return actors;}
int BattleSide_IsFormChanged(int i){return !!(changed&(1u<<i));}
int BattleSide_GetStartChara(int i){return i+10;}
int BattleSide_GetStartCostume(int i){return 1;}
int BattleSide_GetStartVariant(int i){return 0;}
void BattleSide_SetForm(int i,int c,int costume,int variant){}
int BattleSide_GetObjId(int i){return i;}
int BattleSide_GetModelSlot(int i){return i;}
int BattleSide_GetMemberCount(int i){return 1;}
BattleMember *BattleSide_GetMember(int i,int m){return &members[i];}
void BtlObj_Rebind(int i,int model){assert(ready[i]);rebound++;}
void BtlScene_FreeChar(int i){assert(ready[i]);ready[i]=0;freed++;}
s32 BtlLoad_StepChara(BtlJob *job){
    assert(job->side>=0&&job->side<actors);
    if(++job->state<3)return 0;
    assert(!ready[job->side]);ready[job->side]=1;reloaded++;
    BtlJob_Free(job);return 1;
}
static s32 stage_step(BtlJob *job){if(++job->state<3)return 0;BtlJob_Free(job);return 1;}
s32 BtlLoad_RequestStageReload(int stage){
    for(int n=0;n<stageJobs;n++){BtlJob *j=BtlJob_Alloc();assert(j);j->step=stage_step;Job_Push((Job *)j);}
    return stageJobs!=0;
}
void Load_RunBlocking(void){int guard=0;drains++;while(Job_Run())assert(++guard<10000);}
static s32 never_done(Job *job){return 0;}

int main(int argc,char **argv){
    Job_Init();
    if(argc>1){Job pending[JOB_MAX]={0};for(int n=0;n<JOB_MAX;n++){pending[n].step=never_done;Job_Push(&pending[n]);}assert(0);}
    for(int size=2;size<=15;size++)for(int stage=0;stage<4;stage++)for(int partial=0;partial<2;partial++) {
        actors=size;stageJobs=stage;changed=partial?0x5555u:0x7fffu;
        for(int repeat=0;repeat<12;repeat++) {
            drains=reloaded=freed=rebound=0;
            for(int i=0;i<15;i++)ready[i]=1;
            BtlLoad_Reload();
            int expected=0;for(int i=0;i<actors;i++){assert(ready[i]);if(changed&(1u<<i))expected++;}
            assert(reloaded==expected&&freed==expected&&rebound==actors-expected);
            assert(BtlJob_GetFreeCount()==BTL_JOB_COUNT&&Job_GetCount()==0);
        }
    }
    Job_Destroy();
    puts("PASS: production reload, pool, queue and scheduler; 2..15 fighters, stage/queue pressure, mixed retained/rebuilt fighters and 12 repeated rematches per case.");
    return 0;
}
