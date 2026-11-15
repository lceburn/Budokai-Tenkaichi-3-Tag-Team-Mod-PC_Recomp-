/* Exercise the production tag-camera initialization, not a duplicate formula. */
#define PORT 1
#define PORT_FIGHTER_CAPACITY 15
#include "../../../src/battle/btl_cam.c"
#include <assert.h>
#include <stdio.h>
#include <math.h>
static BtlCam memory;
BtlCam *gBtlCam=&memory;
View *gBtlCamView;
int TagTeam_Active(void){return 1;}
int TagTeam_CameraCount(void){return 4;}
int Battle_IsSplitScreen(void){return 1;}
void TagTeam_ViewRect(int view,int *r){r[0]=(view%2)*256;r[1]=(view/2)*224;r[2]=256;r[3]=224;}
float Port_WideFactor(void){return 4.f/3;}
void Vec4_Set(Vec4 *v,float x,float y,float z,float w){v->x=x;v->y=y;v->z=z;v->w=w;}
void Vec4_Copy(Vec4 *a,Vec4 *b){*a=*b;}
int PortSpectator_Target(void){return 2;}
int TagFusion_Owner(int actor){return actor;}
int TagTeam_CameraActor(int actor){return actor;}
int BattleSide_GetObjId(int actor){return 100+actor;}
void BtlCharApi_GetCamPose(int obj,Vec4 *pos,Vec4 *rot){*pos=(Vec4){obj,20,30,1};*rot=(Vec4){.1f,.2f,0,1};}
static int restored_single;
void BtlScene_SetSingleView(int value){restored_single=value;}
void Vu0Screen_LoadMtx(Mtx44 *m){(void)m;}
void Vu0Clip_LoadMtx(Mtx44 *m){(void)m;}
void View_ApplyScissor(ViewScissor *v){(void)v;}
void Mtx_StoreIdentity(Mtx44 *m){int i,j;for(i=0;i<4;i++)for(j=0;j<4;j++)m->m[i][j]=i==j;}
void Mtx_Mul(Mtx44 *a,Mtx44 *b,Mtx44 *out){Mtx44 result;int i,j,k;for(i=0;i<4;i++)for(j=0;j<4;j++){result.m[i][j]=0;for(k=0;k<4;k++)result.m[i][j]+=a->m[i][k]*b->m[k][j];}*out=result;}
int main(void){
    int v,i,j;BtlCam_InitViews();
    for(v=0;v<3;v++){
        View *view=&memory.layouts[v];
        assert(view->screenDist>0 && view->nearZ>0 && view->farZ>view->nearZ);
        for(i=0;i<4;i++)for(j=0;j<4;j++)assert(isfinite(view->view2screen.m[i][j]) && isfinite(view->view2clip.m[i][j]));
    }
    assert(memory.layouts[0].scissorX1==511 && memory.layouts[0].scissorY1==447);
    for(v=0;v<4;v++)assert(nativeViews[v].index==v && nativeViews[v].view.screenDist>0);
    BtlCam_FollowPose(&spectatorView,4);assert(spectatorView.pos.x==102 && spectatorView.rot.y==.2f);
    BtlCam_FollowPose(&spectatorView,2);assert(spectatorView.pos.x==100);
    {
        View authored=memory.layouts[0];BtlCamView copy;
        authored.pos=(Vec4){0,0,-100,1};authored.world2screen.m[3][2]=1234;
        BtlCam_CopyAuthoredView(&copy,&nativeViews[0],&authored);
        assert(copy.pos.z==-100 && copy.view.world2screen.m[3][2]==1234);
    }
    spectatorSavedViews[0]=nativeViews[0];spectatorChanged=1;spectatorDrawing=1;
    spectatorSavedCurrent=&nativeViews[0];spectatorSavedMatrix=&nativeViews[0].view;spectatorSavedSingle=0;
    nativeViews[0].pos.x=999;gBtlCam->cur=&spectatorView;gBtlCamView=&spectatorView.view;
    PortSpectator_EndDraw();assert(nativeViews[0].pos.x==spectatorSavedViews[0].pos.x);
    assert(gBtlCam->cur==&nativeViews[0] && gBtlCamView==&nativeViews[0].view && !spectatorDrawing && !restored_single);
    puts("PASS: production tag initialization provides finite full/split spectator projections and all four native viewports.");
}
