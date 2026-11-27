/* Exercise the production blink/speech selector, not a parallel dispatch. */
#define INCLUDE_RODATA(folder,name)
#include "../../src/battle/bobj_a.c"
#include <assert.h>
#include <stdio.h>
int main(void){
 BObj obj={0};BObjFace face={0};BObjTexTable eyes={0},speech[4]={{0}};
 BObjTexEntry eyeFrames[2]={{0}},mouthFrames[4]={{0}};int i;
 obj.face=&face;obj.mdl.eyes=&eyes;eyes.count=2;eyes.entries=eyeFrames;
 assert(!BtlObj_GetFaceTexture(&obj));
 for(i=1;i<=2;i++){face.eye=i;assert(BtlObj_GetFaceTexture(&obj)==&eyeFrames[i-1]);}
 for(i=0;i<4;i++){
  speech[i].count=1;speech[i].entries=&mouthFrames[i];obj.mdl.mouthB[4+i]=&speech[i];
  ((BobjMdl *)&obj.mdl)->mouthTexA[4+i]=(void*)1;
 }
 for(i=0;i<4;i++){face.shape=5+i;assert(BtlObj_GetFaceTexture(&obj)==&mouthFrames[i]);}
 face.shape=0;face.eye=0;assert(!BtlObj_GetFaceTexture(&obj));
 puts("PASS production face selector: default, both blink frames and all four alternate speech sets");
 return 0;
}
