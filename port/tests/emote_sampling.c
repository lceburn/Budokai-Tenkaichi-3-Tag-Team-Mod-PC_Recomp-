#include <assert.h>
#include <stdio.h>
#include "../../src/battle/bobj_a.c"
Vec4 D_002EC2C0={0,0,0,1};
void Quat_Unpack(Quat *q,u64 packed){*q=(Quat){0,0,0,1};}
void Quat_Slerp(Quat *q,Quat *a,Quat *b,float t){*q=*b;}
void Vec4_Copy(Vec4 *d,Vec4 *s){*d=*s;}
int main(void){
 unsigned char data[256] __attribute__((aligned(16)))={0};
 BobjAnim *motion=(BobjAnim *)data;motion->length=60;motion->track[2]=40;
 BobjTrack *track=(BobjTrack *)(data+160);track->count=1;track->u.key[0].y=.25f;
 BobjBone hip={0};hip.id=2;hip.rest=(Vec4){0,-10,0,1};
 BobjPart part={0};part.pos=(Vec4){1,-9,2,1};part.rot.w=1;
 Vec4 pos;Quat rot;
 BtlObjEmote_Sample(motion,&hip,&part,8,1,0,1,&pos,&rot);assert(pos.y==-9.75f);
 BtlObjEmote_Sample(motion,&hip,&part,8,1,2,2,&pos,&rot);assert(pos.y==-9.5f);
 track->u.key[0].y=-9.75f;BtlObjEmote_Sample(motion,&hip,&part,8,1,1,1,&pos,&rot);assert(pos.y==-9.75f);
 track->flags=1;BtlObjEmote_Sample(motion,&hip,&part,8,1,2,1,&pos,&rot);assert(pos.y==-10);
 motion->track[2]=0;BtlObjEmote_Sample(motion,&hip,&part,8,1,2,1,&pos,&rot);assert(pos.x==1 && pos.y==-9 && pos.z==2);
 puts("PASS emote sampling: imported hip-relative path preserved, own-stock absolute pose, borrowed donor-relative scaling, rotation-only/missing bones");
}
