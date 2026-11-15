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
    puts("PASS: production tag initialization provides finite full/split spectator projections and all four native viewports.");
}
