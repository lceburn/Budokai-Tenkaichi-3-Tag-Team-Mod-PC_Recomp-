/* Opt-in host-only lifecycle sidecar. No game structures or pool layouts change. */
#include "battle/eft_ae.h"
#include "battle/effect_diag.h"
#include "port_host.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
#define NT 65536
#define NL 8192
#define NE 4096
#define NF 15
#define NS 6
typedef struct {void *ptr,*list,*cls;unsigned generation,listgen;int live;unsigned long long removed,caller;unsigned removedMatch,removedFrame;} Task;
typedef struct {void *ptr,*tasks,*work,*parent;unsigned generation,count,stride,bytes;int live,pool;} List;
typedef struct {void *slot,*task,*list,*cls,*child;unsigned generation,listgen,childgen;int active,type,move;} Binding;
typedef struct {unsigned long long seq,value;unsigned match,frame;int resim,a,b;const char *site;void *task,*list,*child,*cls;unsigned generation,listgen,tag;} Event;
PORT_HOST static Task tasks[NT];
PORT_HOST static List lists[NL];
PORT_HOST static Binding bindings[NF][NS];
PORT_HOST static Event events[NE];
PORT_HOST static int roster[NF][5],ready[NF],enabled=-1,stopped,mode,sessionMode,resim;
PORT_HOST static unsigned generation,match,frame;
PORT_HOST static unsigned long long sequence;
PORT_HOST static const char *lastReason;
extern unsigned long long TagNet_BuildIdentity(void);
extern int TagNet_Enabled(void);
int EDiag_Enabled(void) {
    /* Offline lifecycle sidecars cannot follow rollback. Network captures use
       their own history, so an old marker must not interrupt a LAN match. */
    if(TagNet_Enabled())return 0;
    if(enabled<0){const char *s=getenv("BT3_EFFECT_DIAG");FILE *f=fopen("effect-diagnostics.enabled","r");enabled=(s && !strcmp(s,"1")) || f!=NULL;if(f)fclose(f);}
    return enabled;
}
static unsigned bucket(void *p,unsigned mask){return (unsigned)(((uintptr_t)p>>4)*2654435761u)&mask;}
static Task *task_find(void *p,int create) {
    unsigned i,n;if(!p)return NULL;i=bucket(p,NT-1);
    for(n=0;n<NT;n++,i=(i+1)&(NT-1)) {
        if(tasks[i].ptr==p)return &tasks[i];
        if(!tasks[i].ptr){if(create)tasks[i].ptr=p;return create?&tasks[i]:NULL;}
    }return NULL;
}
static List *list_find(void *p,int create) {
    unsigned i,n;if(!p)return NULL;i=bucket(p,NL-1);
    for(n=0;n<NL;n++,i=(i+1)&(NL-1)) {
        if(lists[i].ptr==p)return &lists[i];
        if(!lists[i].ptr){if(create)lists[i].ptr=p;return create?&lists[i]:NULL;}
    }return NULL;
}
void EDiag_Event(const char *site,void *t,void *l,int a,int b,unsigned long long value) {
    Event *e=&events[sequence%NE];Task *m=task_find(t,0);List *v=list_find(l,0);
    memset(e,0,sizeof(*e));e->seq=++sequence;e->site=site;e->task=t;e->list=l;
    e->match=match;e->frame=frame;e->resim=resim;e->a=a;e->b=b;e->value=value;
    if(m){e->generation=m->generation;e->cls=m->cls;
        /* Never inspect an unknown, freed or pool-invalidated task. */
        List *owner=list_find(m->list,0);
        if(m->live && owner && owner->live && owner->generation==m->listgen) {
            BtlTask *actual=t;e->child=actual->child;e->tag=actual->tag;
        }
    }
    if(v)e->listgen=v->generation;
}
static void report(const char *reason,int actor,int slot) {
    FILE *f;unsigned long long n,first;
    f=fopen("effect-lifecycle.txt","w");
    if(f) {
        fprintf(f,"effect-lifecycle-v1 build_identity=%016llx stop=0xE0425443 reason=%s fighter=%d slot=%d match=%u frame=%u resim=%d mode=%d tag_mode=%d\n",TagNet_BuildIdentity(),reason,actor,slot,match,frame,resim,mode,sessionMode);
        for(int i=0;i<NF;i++)fprintf(f,"roster fighter=%d character=%d costume=%d variant=%d team=%d controller=%d ready=%d\n",i,roster[i][0],roster[i][1],roster[i][2],roster[i][3],roster[i][4],ready[i]);
        if(actor>=0&&actor<NF&&slot>=0&&slot<NS){Binding *b=&bindings[actor][slot];fprintf(f,"binding slot_ptr=%p task=%p list=%p class=%p child=%p task_generation=%u list_generation=%u child_generation=%u active=%d type=%d move_id=%d\n",b->slot,b->task,b->list,b->cls,b->child,b->generation,b->listgen,b->childgen,b->active,b->type,b->move);}
        if(actor>=0&&actor<NF&&slot>=0&&slot<NS){Task *t=task_find(bindings[actor][slot].task,0);if(t)fprintf(f,"last_removal seq=%llu caller=%llx match=%u frame=%u current_task_gen=%u live=%d\n",t->removed,t->caller,t->removedMatch,t->removedFrame,t->generation,t->live);}
        first=sequence>NE?sequence-NE:0;
        for(n=first;n<sequence;n++){Event *e=&events[n%NE];fprintf(f,"seq=%llu match=%u frame=%u resim=%d site=%s task=%p list=%p child=%p class=%p task_gen=%u list_gen=%u tag=%08x a=%d b=%d value=%llx\n",e->seq,e->match,e->frame,e->resim,e->site,e->task,e->list,e->child,e->cls,e->generation,e->listgen,e->tag,e->a,e->b,e->value);}
        fflush(f);fclose(f);
    }
    fprintf(stderr,"EFFECT DIAGNOSTIC STOP: %s fighter=%d slot=%d; effect-lifecycle.txt; exception E0425443\n",reason,actor,slot);fflush(stderr);
}
void EDiag_Crash(void){if(EDiag_Enabled()&&!stopped)report("external-crash-before-validation",-1,-1);}
static void capture(const char *reason,int actor,int slot) {
    if(stopped)return;stopped=1;lastReason=reason;
    EDiag_Event("FIRST_VIOLATION",actor>=0&&actor<NF&&slot>=0&&slot<NS?bindings[actor][slot].task:NULL,NULL,actor,slot,0);
    report(reason,actor,slot);
#ifdef EFFECT_DIAG_TEST
    return;
#elif defined(_WIN32)
    { ULONG_PTR info[2]={(ULONG_PTR)actor,(ULONG_PTR)slot};RaiseException(0xE0425443u,EXCEPTION_NONCONTINUABLE,2,info);TerminateProcess(GetCurrentProcess(),0xE0425443u); }
#else
    abort();
#endif
}
void EDiag_List(void *ptr,void *array,unsigned count,unsigned stride,void *work,unsigned bytes,int pool,void *parent) {
    List *l=list_find(ptr,1);if(!l){capture("list-sidecar-capacity",-1,-1);return;}
    *l=(List){ptr,array,work,parent,++generation,count,stride,bytes,1,pool};
    for(unsigned i=0;i<count;i++){Task *t=task_find((char *)array+i*stride,1);if(!t){capture("task-sidecar-capacity",-1,-1);return;}t->list=ptr;t->listgen=l->generation;t->live=0;t->generation=++generation;}
    EDiag_Event("list-create",parent,ptr,(int)count,pool,(uintptr_t)array);
}
void EDiag_ListGone(void *ptr) {List *l=list_find(ptr,0);EDiag_Event("list-destroyed",NULL,ptr,0,0,0);if(l)l->live=0;}
void EDiag_Task(void *ptr,void *list,void *cls) {
    Task *t=task_find(ptr,0);List *l=list_find(list,0);
    if(!t||!l||!l->live){capture("init-outside-owned-list",-1,-1);return;}
    t->generation=++generation;t->list=list;t->listgen=l->generation;t->cls=cls;t->live=1;
    EDiag_Event("task-init/reuse",ptr,list,0,0,0);
}
void EDiag_Kill(void *ptr,const char *site,unsigned long long caller) {Task *t=task_find(ptr,0);EDiag_Event(site,ptr,t?t->list:NULL,0,0,caller);if(t){t->live=0;t->removed=sequence;t->caller=caller;t->removedMatch=match;t->removedFrame=frame;}}
void EDiag_Pool(int pool) {EDiag_Event("pool-reset",NULL,NULL,pool,0,0);for(unsigned i=0;i<NL;i++)if(lists[i].ptr&&lists[i].pool==pool)lists[i].live=0;}
void EDiag_Char(int actor,int state,const char *site){EDiag_Event(site,NULL,NULL,actor,state,0);if(actor>=0&&actor<NF)ready[actor]=state;}
void EDiag_Unbind(int actor,int slot,const char *site){if(actor>=0&&actor<NF&&slot>=0&&slot<NS){Binding *b=&bindings[actor][slot];EDiag_Event(site,b->task,b->list,actor,slot,0);b->active=0;}}
void EDiag_Bind(int actor,int slot,void *address,void *ptr,void *list,void *cls,int type,int move) {
    Task *t=task_find(ptr,0);List *l=list_find(list,0);
    if(actor<0||actor>=NF||slot<0||slot>=NS){capture("slot-index-out-of-range",actor,slot);return;}
    void *child=t&&t->live&&l&&l->live?((BtlTask *)ptr)->child:NULL;List *cl=list_find(child,0);
    bindings[actor][slot]=(Binding){address,ptr,list,cls,child,t?t->generation:0,l?l->generation:0,cl?cl->generation:0,1,type,move};
    EDiag_Event("slot-bind",ptr,list,actor,slot,(uintptr_t)address);
}
int EDiag_Check(int actor,int slot,void *address,void *ptr,void *list,void *cls) {
    Binding *b;Task *t;List *l,*child;BtlTask *actual;const char *why=NULL;
    if(actor<0||actor>=NF||slot<0||slot>=NS){capture("request-index-out-of-range",actor,slot);return 0;}
    b=&bindings[actor][slot];t=task_find(ptr,0);l=list_find(list,0);
    if(!ready[actor])why="request-before-character-effect-init";
    else if(!b->active||b->slot!=address||b->task!=ptr)why="slot-binding-missing-or-changed";
    else if(!t||!l||!l->live)why="unknown-task-or-destroyed-owning-list";
    else if(t->generation!=b->generation)why="slot-references-reused-task-address";
    else if(t->list!=list||b->list!=list||t->listgen!=l->generation||b->listgen!=l->generation)why="owning-list-or-generation-mismatch";
    else if(t->cls!=cls||b->cls!=cls)why="group-class-mismatch";
    if(why){capture(why,actor,slot);return 0;}
    /* Pointer belongs to this still-live registered allocation. Only now
       inspect the task or traverse its list, and only through owned nodes. */
    BtlTask *node=((BtlTaskList *)list)->free;unsigned steps=0;
    while(node){Task *known=task_find(node,0);if(++steps>l->count||!known||known->list!=list||known->listgen!=l->generation){capture("invalid-free-chain",actor,slot);return 0;}if(node==ptr){capture("bound-group-on-free-chain",actor,slot);return 0;}node=node->nextFree;}
    if(!t->live){capture("slot-references-removed-task",actor,slot);return 0;}
    actual=ptr;
    if(actual->list!=list||actual->cls!=cls){capture("live-task-links-or-class-corrupted",actor,slot);return 0;}
    node=((BtlTaskList *)list)->head;steps=0;int found=0;
    while(node){Task *known=task_find(node,0);if(++steps>l->count||!known||known->list!=list||known->listgen!=l->generation){capture("invalid-active-chain",actor,slot);return 0;}if(node==ptr)found=1;node=node->next;}
    if(!found){capture("bound-group-not-in-active-chain",actor,slot);return 0;}
    child=list_find(actual->child,0);
    if(!child||!child->live||child->parent!=ptr){capture("group-child-list-missing-or-destroyed",actor,slot);return 0;}
    if(actual->child!=b->child||child->generation!=b->childgen){capture("group-child-list-replaced-or-reused",actor,slot);return 0;}
    EDiag_Event("slot-valid",ptr,list,actor,slot,(uintptr_t)address);return 1;
}
int EDiag_Request(int actor,int slot){EDiag_Event("EftShot_Start/request",NULL,NULL,actor,slot,0);if(actor<0||actor>=NF||slot<0||slot>=NS){capture("request-index-out-of-range",actor,slot);return 0;}if(!ready[actor]){capture("request-before-character-effect-init",actor,slot);return 0;}return 1;}
void EDiag_Boundary(const char *site){EDiag_Event(site,NULL,NULL,0,0,0);for(int a=0;a<NF;a++)if(ready[a])for(int s=0;s<NS;s++){Binding *b=&bindings[a][s];if(b->active&&!EDiag_Check(a,s,b->slot,b->task,b->list,b->cls))return;}}
void EDiag_Match(int battleMode,int tagMode){match++;frame=0;mode=battleMode;sessionMode=tagMode;EDiag_Event("Battle_Restart/begin",NULL,NULL,battleMode,tagMode,0);}
void EDiag_Frame(unsigned f,int rewind){frame=f;resim=rewind;if(rewind)capture("rollback-encountered-offline-diagnostic-only",-1,-1);}
void EDiag_Roster(int a,int c,int costume,int variant,int team,int control){if(a>=0&&a<NF){roster[a][0]=c;roster[a][1]=costume;roster[a][2]=variant;roster[a][3]=team;roster[a][4]=control;EDiag_Event("roster",NULL,NULL,a,c,(unsigned)costume);}}
void EDiag_Scene(int start){EDiag_Event(start?"BtlScene_Init":"BtlScene_Term",NULL,NULL,0,0,0);memset(ready,0,sizeof(ready));if(start)for(int a=0;a<NF;a++)for(int s=0;s<NS;s++)bindings[a][s].active=0;}
