/* Adapted from NauTaro; AGPL-3.0. See docs/PRESENTATION_UPDATES.md. */
/* Test settings migration, input routing and rollback without physical input. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
struct Setting { const char *name;int value; } settings[20];
static int setting_count,writes,input_reads;
int gPortOverlayOpen,gPortResim;
int Port_Setting(const char *name,int fallback) { int i;for(i=0;i<setting_count;i++)if(!strcmp(settings[i].name,name))return settings[i].value;return fallback; }
void Port_SettingSave(const char *name,int value) {
    int i;for(i=0;i<setting_count;i++)if(!strcmp(settings[i].name,name)){settings[i].value=value;return;}
    settings[setting_count++]=(struct Setting){name,value};
}
void Port_SettingsWrite(void) { writes++; }
void PortInput_Spectator(float *axes) { int i;input_reads++;for(i=0;i<7;i++)axes[i]=0;axes[0]=.5f; }
static int last_player;
void PortInput_SpectatorSlot(int player,float *axes) { last_player=player;PortInput_Spectator(axes); }
#include "../../src/gs/spectator_camera.c"
int main(void) {
    float eye[4]={0,-20,-100,1},rot[4]={0,0,0,1},targets[8]={-20,-10,0,10,20,-10,0,10};
    float result_eye[4],result_rot[4],zoom;SpecCamera saved;int reads;
    assert(PortSpectator_Mode()==0);
    Port_SettingSave("spectator_camera",1);assert(PortSpectator_Mode()==1);
    PortSpectator_SetMode(2);PortSpectator_SetTarget(1);PortSpectator_SetSpeed(400);PortSpectator_SetZoom(150);
    PortSpectator_SetInputActive(1);
    assert(PortSpectator_Camera(eye,rot,targets,2,.517f,1.777f,result_eye,result_rot,&zoom));
    assert(sCamera.mode==2 && sCamera.target==1 && zoom==1.5f);
    assert(PortSpectator_Count()==2 && PortSpectator_Speed()==400);
    saved=sCamera;reads=input_reads;gPortResim=1;
    assert(!PortSpectator_Camera(eye,rot,targets,2,.517f,1.777f,result_eye,result_rot,&zoom));
    assert(!memcmp(&sCamera,&saved,sizeof(saved)) && input_reads==reads);gPortResim=0;
    gPortOverlayOpen=1;
    PortSpectator_Camera(eye,rot,targets,2,.517f,1.777f,result_eye,result_rot,&zoom);
    assert(input_reads==reads);gPortOverlayOpen=0;
    PortSpectator_SetInputActive(1);assert(PortSpectator_ConsumesInput());
    assert(PortSpectator_FilterButtons(0xffff)==0x0009); /* Start and Select remain usable */
    PortSpectator_SetInputActive(0);assert(PortSpectator_FilterButtons(0xffff)==0xffff); /* pause navigation */
    reads=input_reads;PortSpectator_Camera(eye,rot,targets,2,.517f,1.777f,result_eye,result_rot,&zoom);
    assert(input_reads==reads); /* pause directions must not move the camera */
    PortSpectator_ResetCamera();assert(PortSpectator_Zoom()==100);
    PortSpectator_Camera(eye,rot,targets,2,.517f,1.777f,result_eye,result_rot,&zoom);assert(zoom==1);
    PortSpectator_EndBattle();assert(!sCamera.live && !PortSpectator_ConsumesInput());
    PortSpectator_SetMode(0);assert(Port_Setting("spectator_camera",-1)==0);
    assert(!PortSpectator_Camera(eye,rot,targets,2,.517f,1.777f,result_eye,result_rot,&zoom));
    PortSpectator_ViewInput(1,1,2);
    assert(PortSpectator_ConsumesPlayer(1) && !PortSpectator_ConsumesPlayer(0));
    assert(PortSpectator_FilterPlayer(1,0xffff)==0x000b);
    assert(PortSpectator_FilterPlayer(2,0xffff)==0xffff);
    assert(PortSpectator_ViewCamera(3,1,1,eye,rot,targets,2,.517f,1.777f,result_eye,result_rot,&zoom));
    assert(last_player==1 && sViewCamera[3].live && !sViewCamera[4].live);
    saved=sViewCamera[3];gPortResim=1;
    assert(!PortSpectator_ViewCamera(3,3,1,eye,rot,targets,2,.517f,1.777f,result_eye,result_rot,&zoom));
    assert(!memcmp(&saved,&sViewCamera[3],sizeof(saved)));gPortResim=0;
    PortSpectator_ClearViews();assert(!PortSpectator_ConsumesPlayer(1));
    PortSpectator_EndBattle();assert(!sViewCamera[3].live);
    PortSpectator_SetMode(3);assert(PortSpectator_Manual());
    PortSpectator_SetMode(0);assert(PortSpectator_Manual() && !PortSpectator_Mode());
    PortSpectator_UseAutomatic();assert(!PortSpectator_Manual());
    Port_SettingSave("spectator_select_cycle",1);PortSpectator_BattleActive(1);
    for(int n=1;n<=4;n++) {
        assert(PortSpectator_InputButtons(0,1)==0);
        assert(PortSpectator_Mode()==n%4 && PortSpectator_Manual());
        PortSpectator_InputButtons(0,1);assert(PortSpectator_Mode()==n%4);
        PortSpectator_InputButtons(0,0);
    }
    assert(PortSpectator_InputButtons(1,1)==1);
    PortSpectator_EndBattle();assert(!PortSpectator_Mode() && !PortSpectator_Manual());
    assert(PortSpectator_InputButtons(0,1)==1); /* menus keep Select */
    assert(writes>0);puts("PASS: R7 setting migration; persistent mode/target/speed/zoom; rollback skips all camera input/state; F1 input isolation; Start/Select passthrough; pause navigation; camera reset; battle cleanup.");
    return 0;
}
