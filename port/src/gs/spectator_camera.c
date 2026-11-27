/* Adapted from NauTaro/DBZ-BT4-Tag-Team-Decomp, AGPL-3.0; see docs/PRESENTATION_UPDATES.md. */
/* Host-only presentation state: not serialized, hashed or replayed by netcode. */
#include "presentation.h"
#include "spectator_camera.h"
#include "../../../include/battle/tag_camera_controls.h"
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <string.h>
extern int Port_Setting(const char *,int);
extern void Port_SettingSave(const char *,int),Port_SettingsWrite(void);
extern void PortInput_Spectator(float *axes);
extern void PortInput_SpectatorSlot(int player,float *axes);
extern int gPortOverlayOpen,gPortResim;
static SpecCamera sCamera;
static int sReset,sSeenReset,sCount=2,sInputActive,sLastZoom=100,sZoomHeld;
static Uint64 sLastTick;
static int sManual,sBattleActive,sSelectHeld;
static int sPhoto,sPhotoPaused,sPhotoSquare,sPhotoPreviousMode,sPhotoPreviousManual,sPhotoRestore;
int PortSpectator_PhotoActive(void) { return sPhoto && sPhotoPaused && !gPortResim; }
int PortSpectator_PhotoViewRefresh(void) { return !gPortResim && (sPhoto || sPhotoRestore); }
void PortSpectator_PhotoFrameDone(void) { if(!gPortResim && sPhotoRestore)sPhotoRestore--; }
static void photo_exit(void) {
    if(!sPhoto)return;
    sPhoto=0;sPhotoRestore=2;PortSpectator_SetMode(sPhotoPreviousMode);
    __atomic_store_n(&sManual,sPhotoPreviousManual,__ATOMIC_RELAXED);
}
void PortSpectator_PhotoPause(int frozen) {
    if(gPortResim)return;
    sPhotoPaused=frozen!=0;
    if(!sPhotoPaused || !Port_Setting("spectator_photo_mode",0))photo_exit();
}
static SpecCamera sViewCamera[15];
static Uint64 sViewLastTick[15];
static int sViewReset[15],sAutoInput[4];
static unsigned sCycleButton[4];
int PortSpectator_Mode(void) { return (int)spec_clamp(Port_Setting("spectator_mode",Port_Setting("spectator_camera",0)?1:0),0,3); }
void PortSpectator_SetMode(int mode) {
    mode=(int)spec_clamp(mode,0,3);Port_SettingSave("spectator_mode",mode);
    Port_SettingSave("spectator_camera",mode!=0);Port_SettingsWrite();
    __atomic_add_fetch(&sReset,1,__ATOMIC_RELAXED);
    __atomic_store_n(&sManual,1,__ATOMIC_RELAXED);
}
int PortSpectator_Manual(void) { return __atomic_load_n(&sManual,__ATOMIC_RELAXED); }
void PortSpectator_UseAutomatic(void) { if(!gPortResim)__atomic_store_n(&sManual,0,__ATOMIC_RELAXED); }
void PortSpectator_BattleActive(int active) { __atomic_store_n(&sBattleActive,active!=0,__ATOMIC_RELAXED); }
unsigned PortSpectator_InputButtons(int player,unsigned buttons) {
    int enabled,held;
    if(player!=0 || gPortResim)return buttons;
    held=(buttons&1u)!=0;
    if(sPhotoPaused && Port_Setting("spectator_photo_mode",0) && (buttons&0x8000u) && !sPhotoSquare && !gPortOverlayOpen) {
        if(sPhoto)photo_exit();
        else {
            sPhotoPreviousMode=PortSpectator_Mode();sPhotoPreviousManual=PortSpectator_Manual();sPhoto=1;
            PortSpectator_SetMode(sPhotoPreviousMode?sPhotoPreviousMode:1);
        }
    }
    sPhotoSquare=(buttons&0x8000u)!=0;
    if(PortSpectator_PhotoActive()) {
        if(held && !sSelectHeld && !gPortOverlayOpen)PortSpectator_SetMode(TagCamera_SelectNext(PortSpectator_Mode()));
        sSelectHeld=held;return buttons&~1u;
    }
    enabled=__atomic_load_n(&sBattleActive,__ATOMIC_RELAXED) && Port_Setting("spectator_select_cycle",0);
    if(enabled && held && !sSelectHeld && !gPortOverlayOpen)PortSpectator_SetMode(TagCamera_SelectNext(PortSpectator_Mode()));
    sSelectHeld=held;
    return enabled?buttons&~1u:buttons;
}
int PortSpectator_Target(void) { return (int)spec_clamp(Port_Setting("spectator_target",0),0,14); }
void PortSpectator_SetTarget(int target) { Port_SettingSave("spectator_target",(int)spec_clamp(target,0,14));Port_SettingsWrite(); }
int PortSpectator_Count(void) { return __atomic_load_n(&sCount,__ATOMIC_RELAXED); }
void PortSpectator_FighterCount(int count) { if(count>0 && count<=15)__atomic_store_n(&sCount,count,__ATOMIC_RELAXED); }
int PortSpectator_Speed(void) { return (int)spec_clamp(Port_Setting("spectator_speed",100),25,400); }
void PortSpectator_SetSpeed(int speed) { Port_SettingSave("spectator_speed",(int)spec_clamp(speed,25,400));Port_SettingsWrite(); }
int PortSpectator_Zoom(void) { return (int)spec_clamp(Port_Setting("spectator_zoom",100),100,300); }
void PortSpectator_SetZoom(int zoom) { Port_SettingSave("spectator_zoom",(int)spec_clamp(zoom,100,300));Port_SettingsWrite(); }
void PortSpectator_ResetCamera(void) { PortSpectator_SetZoom(100);__atomic_add_fetch(&sReset,1,__ATOMIC_RELAXED); }
void PortSpectator_EndBattle(void) {
    sPhoto=sPhotoPaused=sPhotoSquare=sPhotoRestore=0;
    sCamera.live=0;sLastTick=0;__atomic_store_n(&sInputActive,0,__ATOMIC_RELAXED);
    memset(sViewCamera,0,sizeof(sViewCamera));memset(sViewLastTick,0,sizeof(sViewLastTick));PortSpectator_ClearViews();
    __atomic_store_n(&sManual,0,__ATOMIC_RELAXED);PortSpectator_BattleActive(0);sSelectHeld=0;
    Port_SettingSave("spectator_mode",0);Port_SettingSave("spectator_camera",0);
    __atomic_add_fetch(&sReset,1,__ATOMIC_RELAXED);
}
void PortSpectator_SetInputActive(int active) { __atomic_store_n(&sInputActive,active!=0,__ATOMIC_RELAXED); }
int PortSpectator_ConsumesInput(void) { return PortSpectator_PhotoActive() || (__atomic_load_n(&sInputActive,__ATOMIC_RELAXED) && PortSpectator_Mode()!=0); }
unsigned PortSpectator_FilterButtons(unsigned buttons) {
    if(PortSpectator_PhotoActive())return buttons&0x0008u;
    if(PortSpectator_ConsumesInput() && !Port_Setting("spectator_button_passthrough",0))return buttons&0x0009u;
    return buttons;
}
void PortSpectator_ClearViews(void) { int i;for(i=0;i<4;i++)__atomic_store_n(&sAutoInput[i],0,__ATOMIC_RELAXED); }
void PortSpectator_StopView(int view) { if(view>=0 && view<15){sViewCamera[view].live=0;sViewLastTick[view]=0;} }
void PortSpectator_ViewInput(int player,int active,unsigned button) {
    if(player<0 || player>=4)return;
    __atomic_store_n(&sCycleButton[player],button,__ATOMIC_RELAXED);
    __atomic_store_n(&sAutoInput[player],active!=0,__ATOMIC_RELAXED);
}
int PortSpectator_ConsumesPlayer(int player) {
    return (player==0 && PortSpectator_ConsumesInput()) ||
        (player>=0 && player<4 && __atomic_load_n(&sAutoInput[player],__ATOMIC_RELAXED));
}
unsigned PortSpectator_FilterPlayer(int player,unsigned buttons) {
    if(player==0 && PortSpectator_ConsumesInput())return PortSpectator_FilterButtons(buttons);
    return PortSpectator_ConsumesPlayer(player)?buttons&(0x0009u|__atomic_load_n(&sCycleButton[player],__ATOMIC_RELAXED)):buttons;
}
int PortSpectator_ViewCamera(int view,int mode,int player,const float *native_eye,const float *native_rot,
    const float *targets,int count,float tan_y,float aspect,float *eye,float *rotation,float *zoom) {
    Uint64 now;float dt;SpecInput input={0};int reset;
    if(gPortResim || view<0 || view>=15)return 0;
    now=SDL_GetTicksNS();dt=sViewLastTick[view]?(float)(now-sViewLastTick[view])/1e9f:1.f/60;sViewLastTick[view]=now;
    reset=__atomic_load_n(&sReset,__ATOMIC_RELAXED);
    if(reset!=sViewReset[view]){sViewReset[view]=reset;sViewCamera[view].live=0;}
    if(!gPortOverlayOpen && PortSpectator_ConsumesPlayer(player))PortInput_SpectatorSlot(player,(float *)&input);
    input.boost=0; /* L3 remains the next-view button when spectating after defeat. */
    if(!spec_step(&sViewCamera[view],mode,0,PortSpectator_Speed(),sViewCamera[view].live?sViewCamera[view].zoom:100,dt,
        native_eye,native_rot,targets,count,input,tan_y,aspect))return 0;
    memcpy(eye,sViewCamera[view].eye,16);memcpy(rotation,sViewCamera[view].rotation,16);*zoom=sViewCamera[view].zoom/100;
    return 1;
}
void PortSpectator_ResolveEye(const float *eye) { memcpy(sCamera.eye,eye,16); }
int PortSpectator_Camera(const float *native_eye,const float *native_rot,const float *targets,int count,
                         float tan_y,float aspect,float *eye,float *rotation,float *zoom) {
    Uint64 now;float dt,config_zoom;SpecInput input={0};int generation,mode,result;
    if(gPortResim)return 0;
    mode=PortSpectator_Mode();now=SDL_GetTicksNS();
    dt=sLastTick?(float)(now-sLastTick)/1000000000.f:1.f/60;sLastTick=now;
    generation=__atomic_load_n(&sReset,__ATOMIC_RELAXED);
    if(generation!=sSeenReset){sSeenReset=generation;sCamera.live=0;}
    if(!gPortOverlayOpen && __atomic_load_n(&sInputActive,__ATOMIC_RELAXED))PortInput_Spectator((float *)&input);
    config_zoom=(float)PortSpectator_Zoom();
    if(!sCamera.live || (int)config_zoom!=sLastZoom)sCamera.zoom=config_zoom;
    result=spec_step(&sCamera,mode,PortSpectator_Target(),PortSpectator_Speed(),sCamera.zoom,dt,native_eye,native_rot,targets,count,input,tan_y,aspect);
    if(!result)return 0;
    sLastZoom=(int)(sCamera.zoom+.5f);
    if(sLastZoom!=(int)config_zoom)Port_SettingSave("spectator_zoom",sLastZoom);
    if(sZoomHeld && input.zoom==0)Port_SettingsWrite();
    sZoomHeld=input.zoom!=0;
    memcpy(eye,sCamera.eye,16);memcpy(rotation,sCamera.rotation,16);*zoom=sCamera.zoom/100;
    return 1;
}
