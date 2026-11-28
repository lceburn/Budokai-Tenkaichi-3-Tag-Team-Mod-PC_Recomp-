#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../src/gs/spectator_camera.c"
int gPortOverlayOpen,gPortResim;
static int enabled=1,mode,manualCamera,selectCycle,passthrough=1;
int Port_Setting(const char *key,int fallback) {
    if(!strcmp(key,"spectator_photo_mode"))return enabled;
    if(!strcmp(key,"spectator_mode"))return mode;
    if(!strcmp(key,"spectator_camera"))return manualCamera;
    if(!strcmp(key,"spectator_select_cycle"))return selectCycle;
    if(!strcmp(key,"spectator_button_passthrough"))return passthrough;
    return fallback;
}
void Port_SettingSave(const char *key,int value) {
    if(!strcmp(key,"spectator_mode"))mode=value;
    if(!strcmp(key,"spectator_camera"))manualCamera=value;
}
void Port_SettingsWrite(void){}
void PortInput_Spectator(float *axes){memset(axes,0,7*sizeof(float));}
void PortInput_SpectatorSlot(int player,float *axes){(void)player;PortInput_Spectator(axes);}
int main(void) {
    PortSpectator_BattleActive(1);
    PortSpectator_PhotoPause(0);PortSpectator_InputButtons(0,0x8000);assert(!PortSpectator_PhotoActive());
    PortSpectator_InputButtons(0,0);PortSpectator_PhotoPause(1);
    PortSpectator_InputButtons(0,0x400);assert(!PortSpectator_PhotoActive()); // L1 remains a camera control
    PortSpectator_InputButtons(0,0);
    PortSpectator_InputButtons(0,0x8000);assert(PortSpectator_PhotoActive() && mode==1);
    PortSpectator_InputButtons(0,0x8000);assert(PortSpectator_PhotoActive()); // held button toggles once
    assert(PortSpectator_FilterButtons(0xffff)==8); // only normal Resume reaches pause menu
    PortSpectator_InputButtons(0,0);PortSpectator_InputButtons(0,1);
    assert(mode==TagCamera_SelectNext(1)); // Select works without changing global Select option
    PortSpectator_PhotoPause(0);assert(!PortSpectator_PhotoActive() && mode==3 && PortSpectator_Manual());
    PortSpectator_InputButtons(0,0);PortSpectator_PhotoPause(1);PortSpectator_InputButtons(0,0x8000);
    assert(PortSpectator_PhotoActive());
    enabled=0;PortSpectator_PhotoPause(1);assert(!PortSpectator_PhotoActive() && mode==3);
    enabled=1;PortSpectator_InputButtons(0,0);gPortOverlayOpen=1;PortSpectator_InputButtons(0,0x8000);
    assert(!PortSpectator_PhotoActive());gPortOverlayOpen=0;
    {
        float nativeEye[4]={0,-20,-100,1},nativeRot[4]={0,0,0,1},targets[8]={-20,-10,0,10,20,-10,0,10};
        float eye[4],rotation[4],zoom;int selected;
        for(selected=1;selected<=3;selected++) {
            SpecCamera saved;int generation;
            PortSpectator_InputButtons(0,0);PortSpectator_PhotoPause(1);
            PortSpectator_InputButtons(0,0x8000);assert(PortSpectator_PhotoActive());
            passthrough=0;assert(PortSpectator_FilterButtons(0xffff)==8);
            passthrough=1;assert(PortSpectator_FilterButtons(0xffff)==8);
            PortSpectator_SetMode(selected);PortSpectator_SetInputActive(1);
            assert(PortSpectator_Camera(nativeEye,nativeRot,targets,2,.5f,1.7f,eye,rotation,&zoom));
            sCamera.eye[0]+=25;sCamera.rotation[1]+=.2f;sCamera.zoom=160;sLastZoom=100;
            saved=sCamera;generation=sReset;
            PortSpectator_PhotoPause(0);
            assert(mode==selected && sReset==generation && !memcmp(&saved,&sCamera,sizeof(saved)));
            assert(PortSpectator_Manual() && PortSpectator_PhotoViewRefresh());
            assert(PortSpectator_ConsumesPlayer(0)); // sticks still control the manual camera
            passthrough=1;assert(PortSpectator_FilterPlayer(0,0xffff)==0xffff);
            passthrough=0;assert(PortSpectator_FilterPlayer(0,0xffff)==9);
            passthrough=1;
            PortSpectator_PhotoFrameDone();PortSpectator_PhotoFrameDone();assert(!PortSpectator_PhotoViewRefresh());
        }
        PortSpectator_InputButtons(0,0);PortSpectator_PhotoPause(1);PortSpectator_InputButtons(0,0x8000);
        PortSpectator_SetMode(0);PortSpectator_PhotoPause(0);
        assert(mode==0 && !PortSpectator_ConsumesInput() && PortSpectator_FilterButtons(0xffff)==0xffff);
        selectCycle=1;PortSpectator_InputButtons(0,0);PortSpectator_InputButtons(0,1);
        assert(mode==1); // ordinary Select cycling still works after resuming
    }
    PortSpectator_EndBattle();assert(!PortSpectator_PhotoActive());
    puts("PASS: Photo Mode preserves selected mode/pose/zoom on resume, restores game input, retains cycling, pause/overlay guards and teardown");
}
