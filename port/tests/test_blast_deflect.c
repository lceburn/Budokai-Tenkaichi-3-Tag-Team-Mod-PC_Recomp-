#define PORT 1
#define PORT_FIGHTER_CAPACITY 15
#include "../../src/battle/eft_r.c"
#include <assert.h>
#include <stdio.h>
static int direction_owner,killed,terminated;
void Vec3_Lerp(EftRVec *out,EftRVec *a,EftRVec *b,float t){out->x=a->x*t+b->x*(1-t);}
void Vec4_Copy(EftRVec *out,EftRVec *in){*out=*in;}
int BtlCharApi_GetOpponentObjId(int owner){(void)owner;return 2;}
void BtlCharApi_GetDeflectDir(int owner,EftRVec *out){direction_owner=owner;*out=(EftRVec){1,0,0,0};}
void EftEmit_KillAll(EftRSet *set,EftRState *state){(void)set;(void)state;killed++;}
void EftEmit_TermState(EftRSet *set,EftRState *state){(void)set;(void)state;terminated++;}
int main(void) {
    EftKiBlast blast={0};EftRTask task={0};EftKiObj object={0};
    task.work=&blast;blast.arg.objId=0;blast.arg.srcId=0;blast.flags=EFT_KIBLAST_HOMING;
    task.hit=EFT_R_HIT_DEFLECT;BtlEffect_RecordDeflector(&task,7);
    EftKiBlast_Turn(&task,&blast);
    assert(direction_owner==7 && blast.arg.objId==7 && blast.arg.srcId==0);
    assert((blast.flags&EFT_KIBLAST_DEFLECTED) && !(blast.flags&EFT_KIBLAST_HOMING));
    assert(blast.pose.dir.x==1 && !((unsigned char *)&task)[0xb]);
    task.hit=EFT_R_HIT_REFLECT;blast.arg.life=90;blast.life=1;
    BtlEffect_RecordDeflector(&task,11);EftKiBlast_Turn(&task,&blast);
    assert(direction_owner==11 && blast.arg.objId==11 && blast.life==90);
    EftKiBlast_Term(&task);assert(killed==1 && terminated==1);
    object.b.arg.objId=0;task.hit=EFT_R_HIT_DEFLECT;
    BtlEffect_RecordDeflector(&task,9);EftKiObj_Turn(&task,&object);
    assert(direction_owner==9 && object.b.arg.objId==9);
    puts("PASS: actual multi-fighter deflector owns blast/object, reflection lifetime, source retention and persistent emitter cleanup.");
}
