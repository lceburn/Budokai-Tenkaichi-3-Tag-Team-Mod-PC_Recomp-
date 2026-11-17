/* Host-only diagnostic history. Never restored into a rollback snapshot and
   never changes inputs, digests, network packets, or the simulation. */
#include "net_diag.h"
#include "port_host.h"
#include <stdlib.h>
#include <string.h>
typedef struct {
    unsigned stamp,valid,rewind,mask,confirmed,checked,rollbacks;
    unsigned present,words[ND_ACTORS],headerWords,rngWords,lobbyWords,mtWords,extraWords;
    uint32_t actor[ND_ACTORS][32],header[4],rng[2],lobby[2],mt[624],extra[4];
    uint64_t hash;
    unsigned hashValid,usedMask,actualMask;
    unsigned char used[ND_PADS][ND_PAD_BYTES],actual[ND_PADS][ND_PAD_BYTES];
} Snapshot;
PORT_HOST static Snapshot history[ND_HISTORY];
PORT_HOST static Snapshot *current;
PORT_HOST static unsigned roundNumber,lastFrame,haveFrame,captured,branches;
PORT_HOST static int enabled=-1;
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
void ND_End(uint64_t hash){if(ND_Enabled()&&!captured&&current){current->hash=hash;current->hashValid=1;}}
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
    fprintf(f,"net-desync-v1 build=%016llx data=%016llx round=%u host=%d role=%d peer_role=%d mismatch_frame=%u current_frame=%u branches=%u local=%016llx remote=%016llx\n",(unsigned long long)build,(unsigned long long)data,roundNumber,host,role,peer,at,lastFrame,branches,(unsigned long long)local,(unsigned long long)remote);
    fprintf(f,"schema actor words: chara costume hp maxHp ki blast action target player lockedOff clash fused controller next seconds swap revive helper assist power posX posY posZ posW rotX rotY rotZ rotW; floats are raw IEEE bits. header: actorCount sequenceState clock reserved. extra_rng: mtIndex sceneRand fighterRand vu0R. mt: 624 state words. rng: libc64 low/high. lobby: fingerprint low/high. States are at tick entry BEFORE that tick's inputs. Missing records and masks mean unavailable, not zero.\n");
    if(context)context(f);
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
        if(t==~0u)break;
    }
    {int failed=ferror(f);if(fclose(f))failed=1;if(failed){fprintf(stderr,"net diagnostic: report write failed\n");return 0;}}
    fprintf(stderr,"net diagnostic: saved net-desync.txt, round %u frame %u\n",roundNumber,at);return 1;
}
