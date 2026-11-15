#ifndef BTL_EFFECT_OWNER_H
#define BTL_EFFECT_OWNER_H
#include "common.h"

/* Keep the original two owner bits and stage-reset group separate. */
s32 BtlScene_CharOwnerTag(s32 objId);
void BtlScene_ResetCharacter(s32 objId);
#ifdef PORT
/* Task +0x0B is a class-free byte, initialized to zero by BtlTask_Init.
   Store the actual deflector (plus one) until post-update, independently of
   the shooter's current lock target. It is inside rollback's task state. */
static inline void BtlEffect_RecordDeflector(void *task,s32 objId) {
    if(task && objId>=0 && objId<255)((u8 *)task)[0x0B]=(u8)(objId+1);
}
static inline s32 BtlEffect_TakeDeflector(void *task,s32 fallback) {
    u8 *value=(u8 *)task+0x0B;s32 id=*value?*value-1:fallback;*value=0;return id;
}
#endif

#endif
