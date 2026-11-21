#include "../../include/warrior_net_catalog.h"
#include <string.h>
#include <limits.h>
static void put(unsigned char *p,unsigned n){p[0]=n>>24;p[1]=n>>16;p[2]=n>>8;p[3]=n;}
static unsigned get(const unsigned char *p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
void Wnc_Init(WncCatalog *c){memset(c,0,sizeof(*c));c->revision=1;}
int Wnc_Member(WncCatalog *c,int peer,int present,int owner,unsigned generation) {
    WncMember *m;
    if(!c || peer<0 || peer>=WNC_PEERS || (present!=0 && present!=1) || owner< -1 || owner>=10 || (present && !generation))return 0;
    m=&c->members[peer];
    if(m->present==present && m->owner==owner && m->generation==generation)return 1;
    m->present=present;m->owner=owner;m->generation=generation;m->ack=0;return 1;
}
int Wnc_Admit(WncCatalog *c,int peer,unsigned generation,unsigned forms,const unsigned char hash[WNT_HASH]) {
    WncMember *m;WncEntry *e;unsigned i;
    if(!c || !hash || peer<0 || peer>=WNC_PEERS || !forms || forms>PORT_WARRIOR_PLAYABLE_MAX)return -1;
    m=&c->members[peer];if(!m->present || m->owner<0 || m->generation!=generation)return -1;
    for(i=0;i<c->count;i++) {
        e=&c->entries[i];
        if(e->owner==(unsigned)m->owner && e->generation==generation && !memcmp(e->hash,hash,WNT_HASH))
            return e->forms==forms?(int)e->first:-1;
    }
    if(c->locked || c->count>=PORT_WARRIOR_SESSION_MAX || c->forms>PORT_WARRIOR_SESSION_MAX ||
       forms>PORT_WARRIOR_SESSION_MAX-c->forms || c->revision==UINT_MAX)return -1;
    e=&c->entries[c->count++];memset(e,0,sizeof(*e));e->owner=m->owner;e->generation=generation;
    e->forms=forms;e->first=PORT_WARRIOR_SESSION_FIRST+c->forms;memcpy(e->hash,hash,WNT_HASH);
    /* Activation is a distinct step after validated files have been installed. */
    c->forms+=forms;c->revision++;return (int)e->first;
}
unsigned Wnc_Encode(const WncCatalog *c,void *out,unsigned size) {
    unsigned char *p=out;unsigned i,needed;
    if(!c || !out || !c->revision || c->count>PORT_WARRIOR_SESSION_MAX)return 0;
    needed=16+c->count*48;if(size<needed)return 0;
    put(p,0x574e4331);put(p+4,c->revision);put(p+8,c->count);put(p+12,c->forms);
    for(i=0;i<c->count;i++) {
        const WncEntry *e=&c->entries[i];unsigned char *q=p+16+i*48;
        put(q,e->owner);put(q+4,e->generation);put(q+8,e->forms);put(q+12,e->first);memcpy(q+16,e->hash,WNT_HASH);
    }
    return needed;
}
void Wnc_Digest(const WncCatalog *c,unsigned char hash[WNT_HASH]) {
    unsigned char wire[WNC_WIRE_MAX];unsigned n=Wnc_Encode(c,wire,sizeof(wire));
    if(n)Wnt_Digest(wire,n,hash);else memset(hash,0,WNT_HASH);
}
int Wnc_Decode(WncCatalog *c,const void *data,unsigned size) {
    const unsigned char *p=data;WncCatalog next;unsigned i,j,total=0,revision,count;
    if(!c || !p || size<16 || size>WNC_WIRE_MAX || get(p)!=0x574e4331)return 0;
    revision=get(p+4);count=get(p+8);
    if(!revision || revision<c->revision || count>PORT_WARRIOR_SESSION_MAX || size!=16+count*48)return 0;
    next=*c;next.revision=revision;next.count=count;next.forms=0;
    memset(next.entries,0,sizeof(next.entries));
    for(i=0;i<count;i++) {
        const unsigned char *q=p+16+i*48;WncEntry *e=&next.entries[i];
        e->owner=get(q);e->generation=get(q+4);e->forms=get(q+8);e->first=get(q+12);memcpy(e->hash,q+16,WNT_HASH);
        if(e->owner>=10 || !e->generation || !e->forms || e->forms>PORT_WARRIOR_PLAYABLE_MAX ||
           e->forms>PORT_WARRIOR_SESSION_MAX-total || e->first!=PORT_WARRIOR_SESSION_FIRST+total)return 0;
        for(j=0;j<i;j++)if(e->owner==next.entries[j].owner && e->generation==next.entries[j].generation &&
            !memcmp(e->hash,next.entries[j].hash,WNT_HASH))return 0;
        /* Catalogs are append-only for the life of a room: no ID replacement. */
        if(i<c->count) {
            const WncEntry *old=&c->entries[i];
            if(e->owner!=old->owner || e->generation!=old->generation || e->forms!=old->forms ||
               e->first!=old->first || memcmp(e->hash,old->hash,WNT_HASH))return 0;
            e->verified=old->verified;
        }
        total+=e->forms;
    }
    if(count<c->count || total!=get(p+12) || (revision==c->revision && (count!=c->count || total!=c->forms)) ||
       (c->locked && count!=c->count))return 0;
    next.forms=total;
    if(revision!=c->revision)for(i=0;i<WNC_PEERS;i++)next.members[i].ack=0;
    *c=next;return 1;
}
int Wnc_Verified(WncCatalog *c,unsigned entry,const unsigned char hash[WNT_HASH]) {
    if(!c || !hash || entry>=c->count || memcmp(c->entries[entry].hash,hash,WNT_HASH))return 0;
    c->entries[entry].verified=1;return 1;
}
int Wnc_Ack(WncCatalog *c,int peer,unsigned generation,unsigned revision,const unsigned char hash[WNT_HASH]) {
    unsigned char expected[WNT_HASH];WncMember *m;
    if(!c || !hash || peer<0 || peer>=WNC_PEERS || revision!=c->revision)return 0;
    m=&c->members[peer];if(!m->present || m->generation!=generation)return 0;
    Wnc_Digest(c,expected);if(memcmp(hash,expected,WNT_HASH))return 0;m->ack=revision;return 1;
}
int Wnc_CanReady(const WncCatalog *c) {
    unsigned i;if(!c)return 0;for(i=0;i<c->count;i++)if(!c->entries[i].verified)return 0;return 1;
}
int Wnc_CanStart(const WncCatalog *c) {
    int i;if(!Wnc_CanReady(c))return 0;
    for(i=0;i<WNC_PEERS;i++)if(c->members[i].present && c->members[i].ack!=c->revision)return 0;
    return 1;
}
