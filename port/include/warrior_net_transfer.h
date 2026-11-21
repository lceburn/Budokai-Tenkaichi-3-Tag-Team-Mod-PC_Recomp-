#ifndef WARRIOR_NET_TRANSFER_H
#define WARRIOR_NET_TRANSFER_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Zero-initialize state/budgets before use and cancel before destroying a budget.
   Transport-independent receive state. The lobby owns identity/ownership and
   readiness; this layer only accepts ordered, bounded, verified package bytes. */
enum { WNT_CHUNK=3072, WNT_HASH=32, WNT_TIMEOUT_MS=30000,
       WNT_ROOM_BYTES=512*1024*1024, WNT_RECEIVING=1, WNT_VERIFIED=2 };
typedef struct WntBudget {unsigned bytes,forms;} WntBudget;
typedef struct WntReceive {
    unsigned char *data;
    unsigned size,received,id,generation,forms;
    unsigned char hash[WNT_HASH];
    uint64_t deadline;
    int state;
    WntBudget *budget;
} WntReceive;
void Wnt_Digest(const void *data,unsigned size,unsigned char hash[WNT_HASH]);
int Wnt_Begin(WntReceive *r,WntBudget *budget,unsigned id,unsigned generation,
              unsigned size,unsigned forms,const unsigned char hash[WNT_HASH],uint64_t now);
int Wnt_Chunk(WntReceive *r,unsigned id,unsigned generation,unsigned offset,
              const void *data,unsigned size,uint64_t now);
int Wnt_Finish(WntReceive *r,unsigned id,unsigned generation,uint64_t now);
int Wnt_Expire(WntReceive *r,uint64_t now);
/* Transfer ownership of verified bytes to a retained room package. */
unsigned char *Wnt_Take(WntReceive *r,unsigned *size);
void Wnt_Cancel(WntReceive *r);
/* A sender borrows immutable package bytes until completion/cancellation. emit
   returns one only when the complete frame was queued. Zero is backpressure,
   never a reason to advance the offset or discard bytes. Wire values are BE32. */
enum { WNT_SEND_BEGIN=1, WNT_SEND_CHUNK=2, WNT_SEND_END=3 };
typedef struct WntSend {
    const unsigned char *data;
    unsigned size,offset,id,generation,forms;
    unsigned char hash[WNT_HASH];
    uint64_t deadline;
    int state;
} WntSend;
typedef int (*WntEmit)(void *context,int kind,const void *data,unsigned size);
int Wnt_SendBegin(WntSend *s,const void *data,unsigned size,unsigned id,
                  unsigned generation,unsigned forms,uint64_t now);
/* One bounded frame per step: 1 queued, 0 blocked/idle, -1 expired/invalid.
   Completion means queued to the reliable transport, not remote verification. */
int Wnt_SendStep(WntSend *s,WntEmit emit,void *context,uint64_t now);
void Wnt_SendCancel(WntSend *s);
#ifdef __cplusplus
}
#endif
#endif
