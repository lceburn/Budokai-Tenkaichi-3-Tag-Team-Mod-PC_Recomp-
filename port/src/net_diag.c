/* Host-only diagnostic history. Never restored into a rollback snapshot and
   never changes inputs, digests, network packets, or the simulation. */
#include "net_diag.h"
#include "port_host.h"
#include <stdlib.h>
#include <string.h>
typedef struct {uint64_t caller,before,after;unsigned count;int kind,phase,view;} RngSite;
typedef struct {uint64_t seq,caller,before,after;unsigned frame,branch,ordinal,result;int kind,phase,view;} RngEvent;
typedef struct {
    unsigned stamp,valid,rewind,mask,confirmed,checked,rollbacks;
    unsigned present,words[ND_ACTORS],headerWords,rngWords,lobbyWords,mtWords,extraWords;
    uint32_t actor[ND_ACTORS][32],header[4],rng[2],lobby[2],mt[624],extra[4];
    uint64_t hash;
    unsigned hashValid,usedMask,actualMask;
    unsigned char used[ND_PADS][ND_PAD_BYTES],actual[ND_PADS][ND_PAD_BYTES];
    unsigned rngCount[NR_KIND_COUNT],siteCount,siteOverflow;
    uint64_t firstEvent,lastEvent;
    RngSite sites[ND_RNG_SITES];
} Snapshot;
PORT_HOST static Snapshot history[ND_HISTORY];
PORT_HOST static Snapshot *current;
PORT_HOST static unsigned roundNumber,lastFrame,haveFrame,captured,branches;
PORT_HOST static int enabled=-1;
PORT_HOST static RngEvent rngEvents[ND_RNG_EVENTS],seedEvents[32];
PORT_HOST static uint64_t rngSequence;
PORT_HOST static unsigned seedCount;
PORT_HOST static int rngPhase,rngView=-1,initialValid;
PORT_HOST static Snapshot initial;
int ND_Phase(int phase){int old=rngPhase;rngPhase=phase;return old;}
void ND_View(int view){rngView=view;}
void ND_Rng(int kind,unsigned long long before,unsigned long long after,unsigned long long caller,unsigned result) {
    RngEvent *e;unsigned i;RngSite *site=NULL;
    if(kind<=0||kind>=NR_KIND_COUNT)return;
    if(!current && kind!=NR_LIBC_SEED && kind!=NR_VU0_SEED)return;
    if(!ND_Enabled()||captured)return;
    if(!current||!current->valid) {
        if((kind==NR_LIBC_SEED||kind==NR_VU0_SEED)&&seedCount<32) {
            e=&seedEvents[seedCount++];*e=(RngEvent){0,caller,before,after,0,branches,0,result,kind,rngPhase,rngView};
        }
        return;
    }
    e=&rngEvents[rngSequence%ND_RNG_EVENTS];
    *e=(RngEvent){++rngSequence,caller,before,after,current->stamp,branches,++current->rngCount[kind],result,kind,rngPhase,rngView};
    if(!current->firstEvent)current->firstEvent=e->seq;current->lastEvent=e->seq;
    for(i=0;i<current->siteCount;i++) {
        RngSite *s=&current->sites[i];
        if(s->kind==kind&&s->caller==caller&&s->phase==rngPhase&&s->view==rngView){site=s;break;}
    }
    if(!site&&current->siteCount<ND_RNG_SITES){site=&current->sites[current->siteCount++];*site=(RngSite){caller,before,after,0,kind,rngPhase,rngView};}
    if(site){site->after=after;site->count++;}else current->siteOverflow++;
}
int ND_Enabled(void) {
    if(enabled<0) {
        const char *s=getenv("BT3_NET_DIAG");FILE *f=fopen("disable-net-diagnostics.txt","r");
        enabled=!(s&&!strcmp(s,"0"))&&!f;if(f)fclose(f);
    }
    return enabled;
}
void ND_Reset(unsigned round) {
    if(!ND_Enabled())return;
    memset(history,0,sizeof(history));current=NULL;roundNumber=round;
    lastFrame=haveFrame=captured=branches=0;
    rngSequence=seedCount=initialValid=0;rngPhase=NP_IDLE;rngView=-1;
}
void ND_Begin(unsigned frame,int rewind,unsigned mask,unsigned confirmed,unsigned checked,unsigned rollbacks) {
    if(!ND_Enabled()||captured)return;
    /* A restored frame invalidates its abandoned predicted future too. */
    if(haveFrame && frame<lastFrame) {
        unsigned i;branches++;
        for(i=0;i<ND_HISTORY;i++)if(history[i].valid&&history[i].stamp>=frame)history[i].valid=0;
    }
    current=&history[frame%ND_HISTORY];memset(current,0,sizeof(*current));
    current->stamp=frame;current->valid=1;current->rewind=rewind;current->mask=mask;
    current->confirmed=confirmed;current->checked=checked;current->rollbacks=rollbacks;
    lastFrame=frame;haveFrame=1;
}
static unsigned copy_words(uint32_t *out,unsigned capacity,const void *p,unsigned bytes) {
    unsigned n=bytes/4;if(n>capacity)n=capacity;if(p&&n)memcpy(out,p,n*4);return n;
}
void ND_Audit(int actor,const void *p,unsigned bytes) {
    if(!ND_Enabled()||captured||!current||!current->valid)return;
    if(actor>=0&&actor<ND_ACTORS){current->words[actor]=copy_words(current->actor[actor],32,p,bytes);current->present|=1u<<actor;}
    else if(actor==-1)current->headerWords=copy_words(current->header,4,p,bytes);
    else if(actor==-2)current->rngWords=copy_words(current->rng,2,p,bytes);
    else if(actor==-3)current->lobbyWords=copy_words(current->lobby,2,p,bytes);
    else if(actor==-4)current->mtWords=copy_words(current->mt,624,p,bytes);
    else if(actor==-5)current->extraWords=copy_words(current->extra,4,p,bytes);
}
void ND_End(uint64_t hash){if(ND_Enabled()&&!captured&&current){current->hash=hash;current->hashValid=1;if(!initialValid){initial=*current;initialValid=1;}}}
void ND_Inputs(unsigned frame,unsigned usedMask,unsigned actualMask,const unsigned char *used,const unsigned char *actual) {
    Snapshot *s=&history[frame%ND_HISTORY];unsigned p;
    if(!ND_Enabled()||captured||!s->valid||s->stamp!=frame)return;
    s->usedMask=usedMask&s->mask;s->actualMask=actualMask&s->mask;
    for(p=0;p<ND_PADS;p++) {
        if(s->usedMask&(1u<<p))memcpy(s->used[p],used+p*ND_PAD_BYTES,ND_PAD_BYTES);
        if(s->actualMask&(1u<<p))memcpy(s->actual[p],actual+p*ND_PAD_BYTES,ND_PAD_BYTES);
    }
}
static void words(FILE *f,const char *name,const uint32_t *p,unsigned count) {
    unsigned i;fprintf(f,"%s",name);for(i=0;i<count;i++)fprintf(f," %08x",p[i]);fputc('\n',f);
}
int ND_Dump(unsigned at,uint64_t local,uint64_t remote,uint64_t build,uint64_t data,int host,int role,int peer,void (*context)(FILE *)) {
    FILE *f;unsigned t,start,end,i,p,j;
    if(!ND_Enabled()||captured)return 0;
    captured=1;f=fopen("net-desync.txt","w");if(!f){fprintf(stderr,"net diagnostic: could not write net-desync.txt\n");return 0;}
    fprintf(f,"net-desync-v2 build=%016llx data=%016llx round=%u host=%d role=%d peer_role=%d mismatch_frame=%u current_frame=%u branches=%u local=%016llx remote=%016llx\n",(unsigned long long)build,(unsigned long long)data,roundNumber,host,role,peer,at,lastFrame,branches,(unsigned long long)local,(unsigned long long)remote);
    fprintf(f,"schema actor words: chara costume hp maxHp ki blast action target player lockedOff clash fused controller next seconds swap revive helper assist power posX posY posZ posW rotX rotY rotZ rotW; floats are raw IEEE bits. header: actorCount sequenceState clock reserved. extra_rng: mtIndex sceneRand fighterRand vu0R. mt: 624 state words. rng: libc64 low/high. lobby: fingerprint low/high. States are at tick entry BEFORE that tick's inputs. Missing records and masks mean unavailable, not zero.\n");
    if(context)context(f);
    fprintf(f,"rng_schema kinds=1:libc-next,2:libc-seed,3:vu0-float01,4:vu0-seed,5:mt-next,6:libc-wrapper,7:libc-range,8:vu0-range,9:mt-range phases=0:idle,1:net-tick,2:setup,3:audio,4:ai-input,5:fighters,6:effects-update,7:loaders,8:stage-update,9:camera,10:visibility,11:mechanics,12:view-prep,13:draw-world,14:draw-fighters,15:draw-effects,16:hud,17:overlay,18:frame-end. Events belong to the interval AFTER tick entry. Wrapper kinds describe callers and are not extra generator advances. Site overflow and overwritten events are explicit.\n");
    fprintf(f,"rng_storage event_capacity=%u site_capacity_per_tick=%u bytes=%llu\n",ND_RNG_EVENTS,ND_RNG_SITES,(unsigned long long)(sizeof(history)+sizeof(rngEvents)+sizeof(initial)+sizeof(seedEvents)));
    if(initialValid){fprintf(f,"initial_frame=%u\n",initial.stamp);words(f,"initial_rng",initial.rng,initial.rngWords);words(f,"initial_extra_rng",initial.extra,initial.extraWords);words(f,"initial_mt",initial.mt,initial.mtWords);}
    for(i=0;i<seedCount;i++){RngEvent *e=&seedEvents[i];fprintf(f,"seed_event kind=%d phase=%d caller=%016llx before=%016llx after=%016llx result=%08x\n",e->kind,e->phase,(unsigned long long)e->caller,(unsigned long long)e->before,(unsigned long long)e->after,e->result);}
    start=at>60?at-60:0;end=at>~0u-60?~0u:at+60;
    if(end>lastFrame)end=lastFrame;
    for(t=start;t<=end;t++) {
        Snapshot *s=&history[t%ND_HISTORY];
        if(!s->valid||s->stamp!=t){fprintf(f,"frame=%u unavailable\n",t);continue;}
        fprintf(f,"frame=%u resim=%u mask=%03x confirmed=%u checked=%u rollbacks=%u hash_valid=%u hash=%016llx\n",t,s->rewind,s->mask,s->confirmed,s->checked,s->rollbacks,s->hashValid,(unsigned long long)s->hash);
        words(f,"header",s->header,s->headerWords);words(f,"rng",s->rng,s->rngWords);words(f,"lobby",s->lobby,s->lobbyWords);words(f,"extra_rng",s->extra,s->extraWords);words(f,"mt",s->mt,s->mtWords);
        for(i=0;i<ND_ACTORS;i++)if(s->present&(1u<<i)){char key[32];snprintf(key,sizeof(key),"actor=%u words=%u",i,s->words[i]);words(f,key,s->actor[i],s->words[i]);}
        for(p=0;p<ND_PADS;p++)if((s->usedMask|s->actualMask)&(1u<<p)) {
            fprintf(f,"input=%u used_valid=%u actual_valid=%u used=",p,!!(s->usedMask&(1u<<p)),!!(s->actualMask&(1u<<p)));
            for(j=0;j<ND_PAD_BYTES;j++)fprintf(f,"%02x",s->used[p][j]);fprintf(f," actual=");
            for(j=0;j<ND_PAD_BYTES;j++)fprintf(f,"%02x",s->actual[p][j]);fputc('\n',f);
        }
        fprintf(f,"rng_counts frame=%u overflow=%u",t,s->siteOverflow);for(i=1;i<NR_KIND_COUNT;i++)fprintf(f," k%u=%u",i,s->rngCount[i]);fputc('\n',f);
        for(i=0;i<s->siteCount;i++){RngSite *r=&s->sites[i];fprintf(f,"rng_site frame=%u kind=%d phase=%d view=%d caller=%016llx count=%u before=%016llx after=%016llx\n",t,r->kind,r->phase,r->view,(unsigned long long)r->caller,r->count,(unsigned long long)r->before,(unsigned long long)r->after);}
        if(t>=(at>32?at-32:0)&&t<=at+8&&s->firstEvent) {
            uint64_t q,first=s->firstEvent,retained=rngSequence>=ND_RNG_EVENTS?rngSequence-ND_RNG_EVENTS+1:1;
            if(first<retained)first=retained;
            fprintf(f,"rng_coverage frame=%u first_seq=%llu last_seq=%llu overwritten=%llu\n",t,(unsigned long long)s->firstEvent,(unsigned long long)s->lastEvent,(unsigned long long)(first>s->firstEvent?((first>s->lastEvent+1?s->lastEvent+1:first)-s->firstEvent):0));
            for(q=first;q<=s->lastEvent;q++){RngEvent *r=&rngEvents[(q-1)%ND_RNG_EVENTS];if(r->seq!=q||r->frame!=t)continue;fprintf(f,"rng_event frame=%u seq=%llu branch=%u kind=%d ordinal=%u phase=%d view=%d caller=%016llx before=%016llx after=%016llx result=%08x\n",t,(unsigned long long)q,r->branch,r->kind,r->ordinal,r->phase,r->view,(unsigned long long)r->caller,(unsigned long long)r->before,(unsigned long long)r->after,r->result);}
        }
        if(t==~0u)break;
    }
    {int failed=ferror(f);if(fclose(f))failed=1;if(failed){fprintf(stderr,"net diagnostic: report write failed\n");return 0;}}
    fprintf(stderr,"net diagnostic: saved net-desync.txt, round %u frame %u\n",roundNumber,at);return 1;
}
