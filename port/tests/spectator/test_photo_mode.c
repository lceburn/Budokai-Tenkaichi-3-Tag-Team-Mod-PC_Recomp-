#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../src/gs/spectator_camera.c"
int gPortOverlayOpen,gPortResim;
static int enabled=1,mode,manualCamera,selectCycle;
int Port_Setting(const char *key,int fallback) {
    if(!strcmp(key,"spectator_photo_mode"))return enabled;
    if(!strcmp(key,"spectator_mode"))return mode;
    if(!strcmp(key,"spectator_camera"))return manualCamera;
    if(!strcmp(key,"spectator_select_cycle"))return selectCycle;
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
    PortSpectator_PhotoPause(0);PortSpectator_InputButtons(0,0x400);assert(!PortSpectator_PhotoActive());
    PortSpectator_InputButtons(0,0);PortSpectator_PhotoPause(1);
    PortSpectator_InputButtons(0,0x400);assert(PortSpectator_PhotoActive() && mode==1);
    assert(PortSpectator_FilterButtons(0xffff)==8); // only normal Resume reaches pause menu
    PortSpectator_InputButtons(0,0);PortSpectator_InputButtons(0,1);
    assert(mode==TagCamera_SelectNext(1)); // Select works without changing global Select option
    PortSpectator_PhotoPause(0);assert(!PortSpectator_PhotoActive() && mode==0 && !PortSpectator_Manual());
    PortSpectator_InputButtons(0,0);PortSpectator_PhotoPause(1);PortSpectator_InputButtons(0,0x400);
    assert(PortSpectator_PhotoActive());
    enabled=0;PortSpectator_PhotoPause(1);assert(!PortSpectator_PhotoActive() && mode==0);
    enabled=1;PortSpectator_InputButtons(0,0);gPortOverlayOpen=1;PortSpectator_InputButtons(0,0x400);
    assert(!PortSpectator_PhotoActive());gPortOverlayOpen=0;
    PortSpectator_EndBattle();assert(!PortSpectator_PhotoActive());
    puts("PASS: Photo Mode requires frozen pause, filters menu input, cycles cameras, restores prior view, respects disable/overlay and clears at teardown");
}
