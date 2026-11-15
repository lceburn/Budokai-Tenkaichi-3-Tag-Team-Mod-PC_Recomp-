#define PORT 1
#define PORT_FIGHTER_CAPACITY 15
#include "../../../src/battle/btl_char_cam_modes.c"
#include <assert.h>
#include <math.h>
#include <stdio.h>
Pad gPad[PAD_COUNT];
static int unlocked=1,speed=100;
int TagTeam_Active(void){return 1;}
int TagTeam_LockedOff(int actor){return actor==0 && unlocked;}
int TagTeam_InputPlayer(int actor){return actor;}
int Port_TagOption(int id){(void)id;return speed;}
int BtlChar_TestFlag(ChrCamChr *chr,int flag){(void)chr;return flag==0xc9;}
float BtlInput_GetStickX(ChrCamChr *chr){(void)chr;return 0;}
int BtlInput_TestAction(ChrCamChr *chr,int action,int mode){(void)chr;(void)action;(void)mode;return 0;}
float BtlUtil_WrapAngle(float x){return atan2f(sinf(x),cosf(x));}
float BtlUtil_MinF(float a,float b){return a<b?a:b;}
float BtlUtil_ApproachF(float a,float b,float step){(void)step;return b>a?b:a;}
ChrCamBody *BtlChar_GetPos(ChrCamChr *chr){return &chr->body;}
float Mathf_Sin(float v){return sinf(v);}
float Mathf_Cos(float v){return cosf(v);}
float BtlCharApi_GetHeight(int obj){(void)obj;return 20;}
float ChrCam_GetOffset(ChrCamChr *chr,Vec4 *ofs,int mode){(void)chr;(void)mode;*ofs=(Vec4){0,-20,-50,1};return -.05f;}
float ChrCam_GetBob(ChrCamChr *chr){(void)chr;return 0;}
void Vec4_Copy(Vec4 *dst,Vec4 *src){*dst=*src;}
int main(void){
    ChrCamChr chr={0};Vec4 eye,rot,target;float yaw;
    gPad[0].gameRight[0]=1;gPad[0].gameRight[1]=-1;
    TagCamera_Reset();ChrCam_UpdateFreeYaw(&chr);yaw=chr.cam.yaw;
    assert(yaw>.05f && tagFreePitch[0]>.05f);
    ChrCam_CalcFree(&chr,&eye,&rot,&target);assert(rot.x>-.001f);
    TagCamera_Reset();chr.cam.yaw=0;speed=200;ChrCam_UpdateFreeYaw(&chr);
    assert(fabsf(chr.cam.yaw-yaw*2)<.0001f);
    unlocked=0;chr.cam.yaw=0;ChrCam_UpdateFreeYaw(&chr);assert(chr.cam.yaw==0);
    TagCamera_Reset();assert(tagFreePitch[0]==0);
    puts("PASS: production unlocked camera uses both right-stick axes and sensitivity; locked-on camera input remains unchanged.");
}
