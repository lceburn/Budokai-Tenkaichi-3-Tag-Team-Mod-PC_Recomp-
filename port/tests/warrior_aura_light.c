/* Exercise the production body-light tint, including black and Default. */
#include "../../src/battle/eft_q.c"
#include <assert.h>
#include <stdio.h>
static EftGlowCfg colours;
EftGlowCfg *gEftGlowCfg=&colours;
static s32 selected=-1;
s32 BtlCharApi_GetChara(s32 id){return id;}
s32 PortGame_WarriorAuraColour(s32 id){(void)id;return selected;}
void Vec4_Scale(Vec4 *out,Vec4 *in,f32 scale){
    out->x=in->x*scale;out->y=in->y*scale;out->z=in->z*scale;out->w=in->w*scale;
}
int main(void){
    EftGlow stock={0},black={0},red={0},reset={0};s32 i;
    for(i=0;i<9*EFT_GLOW_ROW;i++)Vec4_Set(&colours.color[i],30,200,70,128);
    EftGlow_SetType(&stock,0,2,0);
    selected=0;EftGlow_SetType(&black,0,2,0);
    assert(black.colorA.x==black.colorA.y && black.colorA.y==black.colorA.z && black.colorA.y<stock.colorA.y);
    assert(black.colorA.w==stock.colorA.w && black.colorB.w==stock.colorB.w);
    for(i=0;i<4;i++)assert(black.color[i].x==black.color[i].y && black.color[i].y==black.color[i].z && black.color[i].w==stock.color[i].w);
    selected=255;EftGlow_SetType(&red,0,2,0);
    assert(red.colorA.x>red.colorA.y && red.colorA.y==red.colorA.z && red.colorA.w==stock.colorA.w);
    selected=-1;EftGlow_SetType(&reset,0,2,0);
    assert(reset.colorA.x==stock.colorA.x && reset.colorA.y==stock.colorA.y && reset.colorA.z==stock.colorA.z);
    puts("PASS production aura body light: black, colour, alpha preservation and Default reset");
    return 0;
}
void Vec4_Set(Vec4 *out,f32 x,f32 y,f32 z,f32 w){out->x=x;out->y=y;out->z=z;out->w=w;}
