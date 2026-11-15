/* Exercise the actual Lua HUD renderer with Hide all HUD, including a visible command. */
#include "../../src/gs/mods_ui.cpp"
#include <cassert>
static bool hidden;
static int reads;
volatile int gUiPresentX=0,gUiPresentY=0,gUiPresentW=1280,gUiPresentH=720;
extern "C" int Port_TagOption(int id){return id==TAG_OPT_HIDE_ALL_HUD && hidden;}
extern "C" int Mods_HudRead(ModsHud *out){
    *out={};out->w=640;out->count=1;out->cmd[0].kind=MODS_CMD_FILL;
    out->cmd[0].color=0xffffffff;out->cmd[0].f[2]=50;out->cmd[0].f[3]=20;reads++;return 1;
}
extern "C" int PortPanels_Read(FpState *out){*out={};return 0;}
extern "C" unsigned Mods_ImageGen(void){return 0;}
extern "C" const char *Mods_ImagePath(int,char *,int){return nullptr;}
ImFont *Ui_FontBold(void){return nullptr;}
SDL_GPUTexture *Ui_Texture(uint8_t *p,uint32_t,uint32_t){free(p);return nullptr;}
void Ui_TextureFree(SDL_GPUTexture *){}
bool FightPanels_Art(const char *,int,int,SDL_GPUTexture **,float *,float *,float *,float *,float *){return false;}
static int draw(){ImGui::NewFrame();ModsUi_Draw();ImGui::Render();return ImGui::GetDrawData()->TotalVtxCount;}
int main(){
    ImGui::CreateContext();auto &io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={1280,720};io.DeltaTime=1.f/30;
    unsigned char *p;int w,h;io.Fonts->GetTexDataAsRGBA32(&p,&w,&h);
    assert(draw()>0 && reads==1);hidden=true;assert(draw()==0 && reads==1);
    hidden=false;assert(draw()>0 && reads==2);
    ImGui::DestroyContext();puts("PASS: Hide all HUD suppresses the production Lua HUD renderer and restores it when disabled.");
}
