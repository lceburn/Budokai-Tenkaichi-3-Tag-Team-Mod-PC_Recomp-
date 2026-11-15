#include "common.h"
#include "battle/btl_char_ctl.h"
#ifdef PORT
#include "battle/tag_damage.h"
#endif

/*
 * Character change requests and the "time stopped" word: 0x1D60A0..0x1D6438.
 *
 * The roster holds a ring of up to 7 requests (roster + 0x138, 0x140 bytes). A fighter that transforms, fuses,
 * switches member or needs an extra object pushes a request (BtlChange_RequestChara / BtlChange_RequestObject,
 * called from the fighter state machine); the battle loader (src/battle/battle_load.c) serves one at a time:
 *
 *   state 0  idle
 *   state 1  BtlChange_Update (end of every frame, also while paused) popped the request: it is now `cur`
 *   state 2  BtlChange_SetTaken   the loader created its job (through BtlChange_NotifyTaken)
 *   state 3  BtlChange_SetLoaded  the files are in memory (through BtlChange_NotifyLoaded); `loaded` = 1
 *   state 4  BtlChange_SetReady   the owning fighter allows the swap (the loader waits for this, BtlChange_IsReady)
 *   state 5  BtlChange_SetDone    the owning fighter is finished; the next BtlChange_Update clears `cur`
 *
 * roster + 0x274 (`timeStop`, BtlChars_IsTimeStopped) is written only by BtlChange_Update and BtlChange_Reset:
 * it is 1 on every frame that ends with an active request and 0 otherwise. A request that finishes and a queued
 * one that starts in the same BtlChange_Update keep it at 1 without a gap.
 */

extern void *memset(void *dst, s32 c, u32 n);

extern BtlCtlRoster *gBtlChars;

/* Reserves the next slot of the request ring, or returns NULL when it is full. */
BtlChangeReq *BtlChange_Alloc(void) {
    BtlChangeQueue *q = &gBtlChars->change;
    s32 w = q->write;

    if ((w + 1) % BTL_CHANGE_RING == q->read) {
        return NULL;
    }
    q->write = (w + 1) % BTL_CHANGE_RING;
    return &q->req[w];
}

/* Takes the oldest request out of the ring, or returns NULL when it is empty. */
BtlChangeReq *BtlChange_Pop(void) {
    BtlChangeQueue *q = &gBtlChars->change;
    s32 r = q->read;

    if (q->write == r) {
        return NULL;
    }
    q->read = (r + 1) % BTL_CHANGE_RING;
    return &q->req[r];
}

/* Empties the request queue and clears the time-stop word. */
void BtlChange_Reset(void) {
    memset(&gBtlChars->change, 0, sizeof(BtlChangeQueue));
}

/* Once per frame: retires a finished request, starts the next one, and sets the time-stop word while one is active. */
void BtlChange_Update(void) {
    BtlChangeQueue *q = &gBtlChars->change;

    if (q->state == BTL_CHANGE_DONE) {
        q->state = BTL_CHANGE_IDLE;
        q->loaded = 0;
        q->cur = NULL;
    }
    if (q->cur == NULL) {
        q->cur = BtlChange_Pop();
        if (q->cur == NULL) {
            q->timeStop = 0;
        } else {
            q->state = BTL_CHANGE_STARTED;
            q->loaded = 0;
            q->timeStop = 1;
        }
    } else {
        q->timeStop = 1;
    }
#ifdef PORT
    /* A cosmetic model-only swap has no action handler to acknowledge it. */
    if(q->cur && q->cur->kind==BTL_CHANGE_KIND_CHARA &&
       q->cur->slot==BTL_CHANGE_DAMAGE_SLOT && q->state==BTL_CHANGE_LOADED)
        q->state=BTL_CHANGE_READY;
#endif
}

#ifdef PORT
s32 BtlChange_IsBattleDamage(s32 player) {
    BtlChangeReq *req=gBtlChars->change.cur;
    return req && req->player==player && req->kind==BTL_CHANGE_KIND_CHARA && req->slot==BTL_CHANGE_DAMAGE_SLOT;
}
s32 BtlChange_RequestBattleDamage(s32 player,s32 chara,s32 costume) {
    BtlChangeQueue *q=&gBtlChars->change;BtlChangeReq *req;
    /* Never compete with a transformation/member switch or fill the ring. */
    if(q->cur || q->write!=q->read)return 0;
    req=BtlChange_Alloc();if(!req)return 0;
    memset(req,0,sizeof(*req));req->player=player;req->kind=BTL_CHANGE_KIND_CHARA;
    req->chara=chara;req->costume=costume;req->variant=1;
    req->animChara=req->unk18=req->voiceChara=-1;req->slot=BTL_CHANGE_DAMAGE_SLOT;
    return 1;
}
#endif

/* Queues a character model change (transformation, fusion, member switch) for a player. */
void BtlChange_RequestChara(s32 player, s32 chara, s32 costume, s32 variant, s32 animChara, s32 unk18, s32 voiceChara) {
    BtlChangeReq *req = BtlChange_Alloc();

    if (req != NULL) {
        req->player = player;
        req->chara = chara;
        req->costume = costume;
        req->variant = variant;
        req->animChara = animChara;
        req->unk18 = unk18;
        req->voiceChara = voiceChara;
        req->kind = BTL_CHANGE_KIND_CHARA;
        req->slot = 0;
    }
}

/* Queues the load of an extra object for a player. */
void BtlChange_RequestObject(s32 player, s32 id, s32 costume, s32 variant, s32 slot) {
    BtlChangeReq *req = BtlChange_Alloc();

    if (req != NULL) {
        req->player = player;
        req->kind = BTL_CHANGE_KIND_OBJECT;
        req->id = id;
        req->costume = costume;
        req->variant = variant;
        req->animChara = -1;
        req->unk18 = -1;
        req->voiceChara = -1;
        req->slot = slot;
    }
}

/* The loader has picked up the active request. */
void BtlChange_SetTaken(void) {
    BtlChangeQueue *q = &gBtlChars->change;

    if (q->cur != NULL) {
        q->state = BTL_CHANGE_TAKEN;
    }
}

/* The files of the active request are in memory. */
void BtlChange_SetLoaded(void) {
    BtlChangeQueue *q = &gBtlChars->change;

    if (q->cur != NULL) {
        q->state = BTL_CHANGE_LOADED;
        q->loaded = 1;
    }
}

/* Tells a player whether the files of its own active request are in memory. */
s32 BtlChange_IsLoadedFor(s32 player) {
    BtlChangeQueue *q = &gBtlChars->change;

    if (q->cur == NULL) {
        return 0;
    }
    if (q->cur->player != player) {
        return 0;
    }
    return q->loaded;
}

/* Tells whether the files of the active request are in memory. */
s32 BtlChange_IsLoaded(void) {
    BtlChangeQueue *q = &gBtlChars->change;

    if (q->cur != NULL) {
        return q->loaded;
    }
    return 0;
}

/* Returns the time-stop word: non-zero while a change request is active. */
s32 BtlChars_IsTimeStopped(void) {
    return gBtlChars->change.timeStop;
}

/* Returns the player that owns the active request, or -1. */
s32 BtlChange_GetPlayer(void) {
    BtlChangeReq *cur = gBtlChars->change.cur;

    if (cur != NULL) {
        return cur->player;
    }
    return -1;
}

/* The owning fighter is ready for the new model to be swapped in. */
void BtlChange_SetReady(s32 player) {
    BtlChangeQueue *q = &gBtlChars->change;

    if (q->cur != NULL && q->cur->player == player) {
        q->state = BTL_CHANGE_READY;
    }
}

/* The owning fighter has finished with the request; BtlChange_Update retires it. */
void BtlChange_SetDone(s32 player) {
    BtlChangeQueue *q = &gBtlChars->change;

    if (q->cur != NULL && q->cur->player == player) {
        q->state = BTL_CHANGE_DONE;
    }
}
