#include "../../include/warrior_net_transfer.h"
#include "../../include/warrior_net_pack.h"
#include "../../include/plat_warriors.h"
#include "../../third_party/monocypher/monocypher.h"
#include <stdlib.h>
#include <string.h>

void Wnt_Digest(const void *data,unsigned size,unsigned char hash[WNT_HASH]) {
    crypto_blake2b(hash,WNT_HASH,data,size);
}
static void wire32(unsigned char *out,unsigned value) {
    out[0]=(unsigned char)(value>>24);out[1]=(unsigned char)(value>>16);
    out[2]=(unsigned char)(value>>8);out[3]=(unsigned char)value;
}
void Wnt_SendCancel(WntSend *s) {if(s)memset(s,0,sizeof(*s));}
int Wnt_SendBegin(WntSend *s,const void *data,unsigned size,unsigned id,
                  unsigned generation,unsigned forms,uint64_t now) {
    if(!s || s->state || s->data || !data || !id || !generation ||
       size<12 || size>WNP_MAX_BYTES || !forms || forms>PORT_WARRIOR_PLAYABLE_MAX ||
       now>UINT64_MAX-WNT_TIMEOUT_MS || WarriorNetPack_Check(data,size)!=(int)forms)return 0;
    memset(s,0,sizeof(*s));s->data=data;s->size=size;s->id=id;
    s->generation=generation;s->forms=forms;s->state=WNT_SEND_BEGIN;
    s->deadline=now+WNT_TIMEOUT_MS;Wnt_Digest(data,size,s->hash);return 1;
}
int Wnt_SendStep(WntSend *s,WntEmit emit,void *context,uint64_t now) {
    unsigned char wire[12+WNT_CHUNK];unsigned n,amount=0;int kind;
    if(!s || !emit)return -1;
    if(!s->state)return 0;
    if(now>=s->deadline || now>UINT64_MAX-WNT_TIMEOUT_MS) {Wnt_SendCancel(s);return -1;}
    kind=s->state;wire32(wire,s->id);wire32(wire+4,s->generation);
    if(kind==WNT_SEND_BEGIN) {
        wire32(wire+8,s->size);wire32(wire+12,s->forms);
        memcpy(wire+16,s->hash,WNT_HASH);n=16+WNT_HASH;
    }else if(kind==WNT_SEND_CHUNK) {
        amount=s->size-s->offset;if(amount>WNT_CHUNK)amount=WNT_CHUNK;
        wire32(wire+8,s->offset);memcpy(wire+12,s->data+s->offset,amount);n=12+amount;
    }else if(kind==WNT_SEND_END)n=8;
    else {Wnt_SendCancel(s);return -1;}
    if(!emit(context,kind,wire,n))return 0;
    s->deadline=now+WNT_TIMEOUT_MS;
    if(kind==WNT_SEND_BEGIN)s->state=WNT_SEND_CHUNK;
    else if(kind==WNT_SEND_CHUNK){s->offset+=amount;if(s->offset==s->size)s->state=WNT_SEND_END;}
    else Wnt_SendCancel(s);
    return 1;
}
void Wnt_Cancel(WntReceive *r) {
    if(!r)return;
    if(r->data) {
        if(r->budget){r->budget->bytes-=r->size;r->budget->forms-=r->forms;}
        free(r->data);
    }
    memset(r,0,sizeof(*r));
}
unsigned char *Wnt_Take(WntReceive *r,unsigned *size) {
    unsigned char *data;
    if(!r || !size || r->state!=WNT_VERIFIED)return NULL;
    data=r->data;*size=r->size;
    if(r->budget){r->budget->bytes-=r->size;r->budget->forms-=r->forms;}
    memset(r,0,sizeof(*r));return data;
}
int Wnt_Begin(WntReceive *r,WntBudget *budget,unsigned id,unsigned generation,
              unsigned size,unsigned forms,const unsigned char hash[WNT_HASH],uint64_t now) {
    unsigned char *data;
    if(!r || !budget || !hash || r->data || r->state || !id || !generation ||
       size<12 || size>WNP_MAX_BYTES || !forms || forms>PORT_WARRIOR_PLAYABLE_MAX ||
       budget->bytes>WNT_ROOM_BYTES || size>WNT_ROOM_BYTES-budget->bytes ||
       budget->forms>PORT_WARRIOR_SESSION_MAX || forms>PORT_WARRIOR_SESSION_MAX-budget->forms ||
       now>UINT64_MAX-WNT_TIMEOUT_MS)return 0;
    data=malloc(size);if(!data)return 0;
    memset(r,0,sizeof(*r));r->data=data;r->size=size;r->id=id;r->generation=generation;
    r->forms=forms;r->budget=budget;r->deadline=now+WNT_TIMEOUT_MS;r->state=WNT_RECEIVING;
    memcpy(r->hash,hash,WNT_HASH);budget->bytes+=size;budget->forms+=forms;return 1;
}
int Wnt_Expire(WntReceive *r,uint64_t now) {
    if(r && r->state==WNT_RECEIVING && now>=r->deadline){Wnt_Cancel(r);return 1;}return 0;
}
int Wnt_Chunk(WntReceive *r,unsigned id,unsigned generation,unsigned offset,
              const void *data,unsigned size,uint64_t now) {
    if(!r || Wnt_Expire(r,now) || r->state!=WNT_RECEIVING || r->id!=id || r->generation!=generation ||
       !data || !size || size>WNT_CHUNK || offset!=r->received || size>r->size-r->received ||
       now>UINT64_MAX-WNT_TIMEOUT_MS)return 0;
    memcpy(r->data+r->received,data,size);r->received+=size;
    r->deadline=now+WNT_TIMEOUT_MS;return 1;
}
int Wnt_Finish(WntReceive *r,unsigned id,unsigned generation,uint64_t now) {
    unsigned char hash[WNT_HASH];
    if(!r || Wnt_Expire(r,now) || r->state!=WNT_RECEIVING || r->id!=id || r->generation!=generation ||
       r->received!=r->size)return 0;
    Wnt_Digest(r->data,r->size,hash);
    if(crypto_verify32(hash,r->hash) || WarriorNetPack_Check(r->data,r->size)!=(int)r->forms) {
        Wnt_Cancel(r);return 0;
    }
    r->state=WNT_VERIFIED;return 1;
}
