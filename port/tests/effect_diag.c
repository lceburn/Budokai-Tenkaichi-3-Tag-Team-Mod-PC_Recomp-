/* Real production list operations + diagnostic sidecar; no disc assets. */
#define PORT 1
#define EFFECT_DIAG_TEST 1
#include "../src/effect_diag.c"
#include <assert.h>
BtlTaskList *BtlTaskList_Create(BtlTask *,unsigned,unsigned);
BtlTaskList *BtlTask_CreateChildList(BtlTask *,unsigned,unsigned);
BtlTask *BtlTaskList_AddTail(BtlTaskList *,BtlTaskCls *,void *);
void BtlTask_Kill(BtlTask *);
void BtlTask_SetDead(BtlTask *);
void BtlTask_DestroyChildList(BtlTask *);
void BtlTaskList_Reset(BtlTaskList *,int);
static int instances;
static unsigned long long arena[1024*1024];
static unsigned used;
unsigned long long TagNet_BuildIdentity(void){return 0x12345678;}
static int online;
int TagNet_Enabled(void){return online;}
void Vec4_Copy(Vec4 *out,Vec4 *in){*out=*in;}
int BtlPool_GetCurrent(void){return 4;}
void *BtlPool_Alloc(int pool,int bytes){void *p;(void)pool;bytes=(bytes+15)&~15;assert(used+bytes<sizeof(arena));p=(char *)arena+used;used+=bytes;memset(p,0,bytes);return p;}
void BtlPool_Free(int pool,void *p){(void)pool;(void)p;}
static void init_group(BtlTask *t,void *arg){(void)arg;BtlTask_CreateChildList(t,2,0);}
static void reset_group(BtlTask *t){(void)t;}
static void init_instance(BtlTask *t,void *arg){(void)t;(void)arg;instances++;}
static void reset_instance(BtlTask *t){BtlTask_SetDead(t);}
static BtlTaskCls group_class={NULL,init_group,NULL,NULL,reset_group,NULL};
static BtlTaskCls instance_class={NULL,init_instance,NULL,NULL,reset_instance,NULL};
static void clear(void){memset(tasks,0,sizeof(tasks));memset(lists,0,sizeof(lists));memset(bindings,0,sizeof(bindings));memset(ready,0,sizeof(ready));memset(events,0,sizeof(events));sequence=generation=match=frame=stopped=instances=used=0;lastReason=NULL;enabled=1;}
static void bind_slot_fixture(int actor,BtlTask *t){EDiag_Char(actor,1,"fixture-ready");EDiag_Bind(actor,2,(void *)(uintptr_t)(actor+1),t,t->list,&group_class,1,0x24);}
static int valid(int actor,BtlTask *t){return EDiag_Check(actor,2,(void *)(uintptr_t)(actor+1),t,t->list,&group_class);}
int main(void){
    BtlTaskList *root;BtlTask *group,*again,*all[NF];
    enabled=1;online=1;assert(!EDiag_Enabled());online=0;assert(EDiag_Enabled());
    clear();root=BtlTaskList_Create(NULL,16,0);group=BtlTaskList_AddTail(root,&group_class,NULL);bind_slot_fixture(2,group);
    assert(valid(2,group));BtlTask_Kill(group);assert(!valid(2,group)&&stopped);
    assert(!strcmp(lastReason,"bound-group-on-free-chain"));
    assert(task_find(group,0)->removed && task_find(group,0)->caller);
    clear();root=BtlTaskList_Create(NULL,16,0);group=BtlTaskList_AddTail(root,&group_class,NULL);bind_slot_fixture(2,group);
    BtlTask_Kill(group);again=BtlTaskList_AddTail(root,&group_class,NULL);assert(again==group);
    assert(!valid(2,again)&&stopped); /* matching address/class still stale */
    assert(!strcmp(lastReason,"slot-references-reused-task-address"));
    clear();root=BtlTaskList_Create(NULL,16,0);
    for(int a=0;a<NF;a++){all[a]=BtlTaskList_AddTail(root,&group_class,NULL);bind_slot_fixture(a,all[a]);EDiag_Roster(a,a+10,1,0,a%2,a<4?a:-1);}
    for(int round=0;round<12;round++) {
        EDiag_Match(4,1);
        for(int a=0;a<NF;a++)assert(BtlTaskList_AddTail(all[a]->child,&instance_class,NULL));
        BtlTaskList_Reset(root,0);EDiag_Boundary("fixture-rematch");assert(!stopped);
        for(int a=0;a<NF;a++)assert(valid(a,all[a]) && !all[a]->child->head);
    }
    assert(instances==12*NF);
    for(int a=0;a<NF;a++){EDiag_Char(a,0,"fixture-selection-return");EDiag_Unbind(a,2,"fixture-slot-clear");BtlTask_Kill(all[a]);}
    for(int a=0;a<NF;a++){all[a]=BtlTaskList_AddTail(root,&group_class,NULL);bind_slot_fixture(a,all[a]);assert(valid(a,all[a]));}
    EDiag_Boundary("fixture-selection-rebuilt");assert(!stopped);
    EDiag_Pool(4);assert(!valid(14,all[14])&&stopped);
    clear();assert(!EDiag_Request(2,2)&&stopped);
    clear();root=BtlTaskList_Create(NULL,16,0);group=BtlTaskList_AddTail(root,&group_class,NULL);bind_slot_fixture(2,group);
    assert(!EDiag_Check(2,2,(void *)3,(void *)1,root,&group_class)&&stopped); /* no invalid dereference */
    clear();root=BtlTaskList_Create(NULL,16,0);group=BtlTaskList_AddTail(root,&group_class,NULL);bind_slot_fixture(2,group);
    BtlTask_DestroyChildList(group);assert(!valid(2,group)&&stopped);
    clear();root=BtlTaskList_Create(NULL,16,0);group=BtlTaskList_AddTail(root,&group_class,NULL);bind_slot_fixture(2,group);
    BtlTask_DestroyChildList(group);BtlTask_CreateChildList(group,2,0);assert(!valid(2,group)&&stopped);
    puts("PASS: stale removal, address reuse, 12 rematches x15 fighters, selection rebuild, pool invalidation, pre-init request, unknown pointer and child destruction/replacement.");
}
