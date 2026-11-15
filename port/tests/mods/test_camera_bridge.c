/* Camera bridge with real host state: extra views, zoom, ownership and rollback. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../src/mods_game.c"

int gPortResim;
void *gBtlChars,*gBtlSeq;
static unsigned char fighter[0x600];
static float position[3]={1,2,3},rotation[3];
static int calls,cut;
void *BtlChar_FindByObjId(int id){(void)id;return fighter;}
void *BtlChar_FindBySide(int id){return BtlChar_FindByObjId(id);}
int BtlChar_TestFlag(void *chr,unsigned flag){(void)chr;(void)flag;return 0;}
int ChrCam_IsCutActive(void *chr){(void)chr;return cut;}
int Battle_IsSplitScreen(void){return 1;}
int TagTeam_Active(void){return 1;}
int TagTeam_ViewActor(int view){return view;}
int TagTeam_CameraActor(int actor){return actor;}
int TagTeam_Team(int actor){return actor%2;}
int TagTeam_Target(int actor){return actor==14?3:14;}
int BattleSide_GetObjId(int actor){return actor+100;}
int BtlCharApi_GetHp(int object){return object*100;}
void Mods_Tick(void){}
int Mods_WantCamera(void){return 1;}
void Mods_Camera(void){
    ModsCam cam;calls++;PortMods_CamGet(&cam);
    assert(cam.view==3 || cam.view==14);
    assert(cam.hp==(cam.actor+100)*100);
    if(cam.cut)return;
    cam.zoom=cam.view==3?.7f:1.2f;cam.collide=cam.view==3;
    cam.pos[0]+=10;PortMods_CamSet(&cam);
}
int main(void){
    Port_ModsCamera(3,103,Port_Addr(position),Port_Addr(rotation));
    assert(position[0]==11 && Port_ModsCamZoom(3)==700 && Port_ModsCamCollide(3));
    Port_ModsCamera(14,114,Port_Addr(position),Port_Addr(rotation));
    assert(position[0]==21 && Port_ModsCamZoom(14)==1200 && !Port_ModsCamCollide(14));
    assert(Port_ModsCamZoom(3)==700 && Port_ModsCamZoom(1)==1000);
    cut=1;Port_ModsCamera(3,103,Port_Addr(position),Port_Addr(rotation));
    assert(position[0]==21 && Port_ModsCamZoom(3)==1000);
    gPortResim=1;Port_ModsCamera(3,103,Port_Addr(position),Port_Addr(rotation));
    assert(calls==3 && position[0]==21);
    Port_ModsCamera(15,114,Port_Addr(position),Port_Addr(rotation));assert(calls==3);
    puts("PASS camera bridge: independent views 4/15; owner health; zoom/collision output; cuts and rollback.");
}
