#ifndef WARRIOR_NET_CATALOG_H
#define WARRIOR_NET_CATALOG_H
#include "plat_warriors.h"
#include "warrior_net_transfer.h"
#ifdef __cplusplus
extern "C" {
#endif
enum { WNC_PEERS=20, WNC_WIRE_MAX=16+48*PORT_WARRIOR_SESSION_MAX };
typedef struct WncEntry {
    unsigned owner,generation,forms,first;
    unsigned char hash[WNT_HASH];
    int verified;
} WncEntry;
typedef struct WncMember {int present,owner;unsigned generation,ack;} WncMember;
typedef struct WncCatalog {
    unsigned revision,count,forms;
    int locked;
    WncEntry entries[PORT_WARRIOR_SESSION_MAX];
    WncMember members[WNC_PEERS];
} WncCatalog;
void Wnc_Init(WncCatalog *c);
/* Membership comes from the room-assigned connection, never packet claims.
   An observer uses owner=-1. Seat reuse must receive a new generation. */
int Wnc_Member(WncCatalog *c,int peer,int present,int owner,unsigned generation);
/* A verified received package is admitted whole. Existing IDs never move.
   Returns the assigned first character ID, or -1 without changing the catalog. */
int Wnc_Admit(WncCatalog *c,int peer,unsigned generation,unsigned forms,const unsigned char hash[WNT_HASH]);
unsigned Wnc_Encode(const WncCatalog *c,void *out,unsigned size);
int Wnc_Decode(WncCatalog *c,const void *data,unsigned size);
void Wnc_Digest(const WncCatalog *c,unsigned char hash[WNT_HASH]);
int Wnc_Verified(WncCatalog *c,unsigned entry,const unsigned char hash[WNT_HASH]);
int Wnc_Ack(WncCatalog *c,int peer,unsigned generation,unsigned revision,const unsigned char hash[WNT_HASH]);
int Wnc_CanReady(const WncCatalog *c);
int Wnc_CanStart(const WncCatalog *c);
#ifdef __cplusplus
}
#endif
#endif
