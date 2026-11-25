#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#define PORT 1
#include "battle/btl_cam.h"
#include "battle/btl_obj.h"
static BtlObj object;
static BtlObjPart part;
static int custom=1,shown=1,wall,traceCalls;
int gStgColMode=7;
BtlObj *BtlObj_Get(int id){return id==0?&object:NULL;}
BtlObjPart *BtlObj_GetNode(BtlObj *o,int node){return &part;}
int BtlObj_IsNodeShown(BtlObj *o,int node){return shown;}
int PortGame_IsCustomCharacter(int c){return custom;}
void BtlCharApi_GetNodePos(int id,int node,Vec4 *p){*p=(Vec4){0,0,0,1};}
int BtlCam_TraceStage(Vec4 *out,Vec4 *from,Vec4 *to,float *frac,int *hit){
 traceCalls++;*out=*from;
 if(wall && from->z<0){out->z=-1;return 1;}
 return 0;
}
#include "../../src/battle/btl_cam_clearance.inc"
static void identity(Mtx44 *m){memset(m,0,sizeof(*m));for(int i=0;i<4;i++)m->m[i][i]=1;}
int main(void){
 struct {unsigned char root[64];BtlObjBound head;} rig;
 memset(&rig,0,sizeof(rig));
 BtlObjBound *root=(BtlObjBound *)rig.root;root->next=64;
 rig.head.enabled=1;rig.head.node=48;rig.head.last=1;rig.head.extent=(Vec4){3,4,5,0};
 object.mdl.bounds=root;object.chara=250;identity(&part.mtx);
 Vec4 head={0,0,0,1},ray={0,0,-1,0};float exit;
 assert(BtlCam_CustomRay(&object,&head,&ray,0,&exit));
 assert(fabsf(exit-7)<.001f);
 BtlCamView view;memset(&view,0,sizeof(view));view.pos=(Vec4){0,0,-1,1};
 BtlCam_CustomClearance(&view,0);assert(view.pos.z < -7 && gStgColMode==7);
 Vec4 safe=view.pos;traceCalls=0;BtlCam_CustomClearance(&view,0);assert(!memcmp(&safe,&view.pos,sizeof(safe)) && !traceCalls);
 view.pos=(Vec4){0,0,-1,1};wall=1;BtlCam_CustomClearance(&view,0);
 float distance=sqrtf(view.pos.x*view.pos.x+view.pos.y*view.pos.y+view.pos.z*view.pos.z);
 ray=view.pos;ray.x/=distance;ray.y/=distance;ray.z/=distance;
 assert(!BtlCam_CustomRay(&object,&head,&ray,distance,&exit) && gStgColMode==7);
 custom=0;view.pos=(Vec4){0,0,0,1};BtlCam_CustomClearance(&view,0);assert(view.pos.z==0);
 custom=1;shown=0;BtlCam_CustomClearance(&view,0);assert(view.pos.z==0);
 shown=1;part.mtx.m[0][0]=2;ray=(Vec4){1,0,0,0};assert(BtlCam_CustomRay(&object,&head,&ray,0,&exit) && fabsf(exit-8)<.001f);
 puts("PASS custom camera: compact root traversal, head/hair bounds, stage escape, scaled meshes, safe/stock/hidden views unchanged");
}
