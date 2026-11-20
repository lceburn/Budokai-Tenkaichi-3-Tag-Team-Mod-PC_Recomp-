#ifdef PORT
#include "common.h"
#include "battle/view_a.h"
#include "battle/orbit_cam.h"
#include "sys/pad.h"
#include "menu/port_screen.h"

/* Model-viewer integration follows the public char-packages menu viewer:
   https://github.com/z3xox/Tenkaichi3Decomp/blob/char-packages/src/menu/port_modmgr.c
   Use this engine's ChrView and native BT4 resources; no second game window. */
extern u8 *BtlObj_Get(s32);
extern void BtlObj_UpdateAll(void),BtlObj_UpdateVisibility(s32),BtlObj_FinishVisibility(s32);
extern void BtlStage_Update(void),BtlScene_Update(void),BtlObjDraw_Draw(void);
extern void Gfx_MarkPass(s32),Gfx_AddDefaultEnv(void),BtlScene_Draw(s32);
extern s32 Ot_Draw(void),MapBgm_IsStopped(void);
extern void MapBgm_Stop(void),StgAmb_SetBgmScale(f32),StgAmb_SetSeScale(f32);
extern s32 Port_PageAspectMilli(void);
extern f32 cosf(f32),sinf(f32),sqrtf(f32);
extern struct WarriorViewStage {s32 unk[2],flags;} *gBtlStage;
static s32 opened,shown=-1;
static f32 yaw=3.59f,zoom=1;

void WarriorView_Open(void) {
    if(opened)return;
    ChrView_Init();opened=1;shown=-1;zoom=1;yaw=3.59f;
}
void WarriorView_Clear(void) {
    if(opened)ChrView_Hide();
    shown=-1;
}
void WarriorView_Close(void) {
    if(!opened)return;
    ChrView_Hide();ChrView_Term();opened=0;shown=-1;
}
void WarriorView_Draw(s32 chara,f32 cx,f32 feet,f32 height,s32 controls) {
    ChrViewInfo *info;ChrView *viewer;u8 *obj;f32 fx,fy,hf,dist,ox,ty,narrow,*m,scale,c,s;
    if(!opened)return;
    viewer=ChrView_Get();
    if(chara<0 || !PortGame_IsCharacter(chara)) {if(shown>=0)WarriorView_Clear();return;}
    if(shown!=chara) {
        ChrView_Hide();ChrView_Show(chara,0,0);shown=chara;zoom=1;yaw=3.59f;
    }
    if(ChrView_IsLoading() || !ChrView_IsVisible() || !viewer->cur)return;
    info=ChrView_GetCharaInfo(chara);if(!info)return;
    obj=BtlObj_Get(viewer->cur->obj);if(!obj)return;
    if(controls) {
        if(gPad[0].gameHeld&PADG_RSTICK_LEFT)yaw-=.04f;
        if(gPad[0].gameHeld&PADG_RSTICK_RIGHT)yaw+=.04f;
        if(gPad[0].gameHeld&PADG_L2)zoom+=.01f;
        if(gPad[0].gameHeld&PADG_R2)zoom-=.01f;
        if(zoom<.65f)zoom=.65f;if(zoom>1.8f)zoom=1.8f;
    }
    fx=Ps_X(cx)/512;fy=Ps_Y(feet)/448;hf=(Ps_Y(height)-Ps_Y(0))/448*zoom;
    dist=info->dist*.647f/(hf>.05f?hf:.05f);ox=-(fx-.492f)/.76f*dist;
    ty=(.4755f+.662f*hf-fy)/1.115f;narrow=1333.0f/Port_PageAspectMilli();
    OrbitCam_SetDistRange(dist*.5f,dist*2);OrbitCam_SetDist(dist);
    OrbitCam_SetTarget(0,info->targetY+ty*dist,0);OrbitCam_SetAngles(0,3.1415927f,0);
    m=(f32 *)(obj+0x9A0);scale=sqrtf(m[4]*m[4]+m[5]*m[5]+m[6]*m[6]);
    c=cosf(yaw);s=sinf(yaw);*(f32 *)(obj+0x950)=ox;*(f32 *)(obj+0x964)=yaw;
    m[0]=c*scale*narrow;m[1]=0;m[2]=-s*scale;m[8]=s*scale*narrow;m[9]=0;m[10]=c*scale;m[12]=ox;
    OrbitCam_Update(0);viewer->stageOn=0;gBtlStage->flags|=1;
    BtlObj_UpdateAll();BtlObj_UpdateVisibility(0);BtlObj_FinishVisibility(0);
    StgAmb_SetBgmScale(0);StgAmb_SetSeScale(0);BtlStage_Update();
    if(!MapBgm_IsStopped())MapBgm_Stop();
    BtlScene_Update();Gfx_MarkPass(2);BtlObjDraw_Draw();Gfx_MarkPass(0);
    Gfx_MarkPass(4);Gfx_AddDefaultEnv();BtlScene_Draw(1);Ot_Draw();Gfx_MarkPass(0);
}
#endif
