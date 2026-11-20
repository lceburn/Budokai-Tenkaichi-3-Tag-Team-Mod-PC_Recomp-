/* The embedded-model decision uses synthetic packs; no game data required.
 * cc -Iport/include port/tests/warrior_model.c port/src/plat_game.c port/src/plat_warriors.c -o test */
#include "plat_game.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *Port_FileRoot(void) { return "."; }
void *Port_LowAlloc(size_t n) { return malloc(n); }
int MenuLabels_MusicIds(const char *root,int *ids,int capacity) {
    (void)root;(void)ids;(void)capacity;return 0;
}
static void w32(unsigned char *p,unsigned n) {p[0]=n;p[1]=n>>8;p[2]=n>>16;p[3]=n>>24;}
static void enabled(const char *value);
static void wf(unsigned char *p,float f) {memcpy(p,&f,4);}
static float rf(const unsigned char *p) {float f;memcpy(&f,p,4);return f;}
static void animation_scale_checks(unsigned char *model) {
    unsigned char animation[176],original[176],*table=model+1280;
    w32(model+26*4,1280);w32(model+27*4,1536);
    memcpy(table,"BT3WARRI",8);wf(table+8,1.5f);w32(table+12,1);
    w32(table+16,2u<<16);wf(table+20,2.0f); /* idle motions 0..1 */
    memset(animation,0,sizeof(animation));
    animation[6]=animation[8]=37; /* two nodes share the same track */
    animation[150]=1;wf(animation+152,1);wf(animation+156,-5);wf(animation+160,3);
    memcpy(original,animation,sizeof(animation));
    assert(PortGame_AdjustWarriorAnimation(model,2048,0,animation,sizeof(animation))==1);
    assert(rf(animation+152)==2 && rf(animation+156)==-10 && rf(animation+160)==6);
    memcpy(animation,original,sizeof(animation));
    assert(PortGame_AdjustWarriorAnimation(model,2048,2,animation,sizeof(animation))==1);
    assert(rf(animation+156)==-7.5f);
    memcpy(animation,original,sizeof(animation));enabled("0");
    assert(!PortGame_AdjustWarriorAnimation(model,2048,0,animation,sizeof(animation)));
    assert(!memcmp(animation,original,sizeof(animation)));enabled("1");
    w32(table+12,1000);
    assert(PortGame_AdjustWarriorAnimation(model,2048,0,animation,sizeof(animation))==-1);
    assert(!memcmp(animation,original,sizeof(animation)));w32(table+12,1);
    wf(table+20,0.0f);
    assert(PortGame_AdjustWarriorAnimation(model,2048,0,animation,sizeof(animation))==-1);
    assert(!memcmp(animation,original,sizeof(animation)));wf(table+20,2.0f);
    animation[150]=2;
    assert(PortGame_AdjustWarriorAnimation(model,2048,0,animation,sizeof(animation))==-1);
    memcpy(animation,original,sizeof(animation));animation[148]=1; /* rotation only */
    assert(PortGame_AdjustWarriorAnimation(model,2048,0,animation,sizeof(animation))==1);
    assert(rf(animation+156)==-5);
}
static void enabled(const char *value) {
#ifdef _WIN32
    _putenv_s("BT3_WARRIOR_MODELS",value);
#else
    setenv("BT3_WARRIOR_MODELS",value,1);
#endif
}
int main(int argc,char **argv) {
    unsigned char data[2048];int i;
    {
        unsigned char chip[5504];unsigned char *entry=chip+32;
        memset(chip,0,sizeof(chip));w32(chip,1);w32(chip+4,8);
        w32(entry,24);w32(entry+4,0x438);
        w32(entry+8,0x1080);w32(entry+12,0x480);
        w32(entry+48,(19u<<20)|(6u<<26)|(6u<<30));w32(entry+52,1);
        assert(PortGame_CheckWarriorChip(chip,sizeof(chip)));
        assert(!PortGame_CheckWarriorChip(NULL,sizeof(chip)));
        assert(!PortGame_CheckWarriorChip(chip,sizeof(chip)-1));
        w32(entry,0x40000018u);assert(!PortGame_CheckWarriorChip(chip,sizeof(chip)));w32(entry,24);
        w32(entry+8,0xFFFFFFFFu);assert(!PortGame_CheckWarriorChip(chip,sizeof(chip)));w32(entry+8,0x1080);
        w32(chip+4,0x1000);assert(!PortGame_CheckWarriorChip(chip,sizeof(chip)));w32(chip+4,8);
        w32(entry+48,0);assert(!PortGame_CheckWarriorChip(chip,sizeof(chip)));
    }
    {
        unsigned char header[8];
        w32(header,0x20800);w32(header+4,100);
        assert(PortGame_CheckPortraitHeader(header,8,108));
        assert(!PortGame_CheckPortraitHeader(NULL,8,108));
        assert(!PortGame_CheckPortraitHeader(header,7,108));
        assert(!PortGame_CheckPortraitHeader(header,8,107));
        assert(!PortGame_CheckPortraitHeader(header,8,0x16801));
        w32(header,0x20801);assert(!PortGame_CheckPortraitHeader(header,8,108));
        w32(header,0);assert(!PortGame_CheckPortraitHeader(header,8,108));
    }
    memset(data,0,sizeof(data));
    w32(data,252);
    for(i=1;i<=253;i++) w32(data+i*4,i==1?1024:1280);
    enabled("");assert(!PortGame_UseEmbeddedModel(data,sizeof(data)) && PortGame_ModelCapacity()==0xE6800);
    enabled("0");assert(!PortGame_UseEmbeddedModel(data,sizeof(data)));
    enabled("1");assert(PortGame_UseEmbeddedModel(data,sizeof(data)) && PortGame_ModelCapacity()==0x200000);
    assert(!PortGame_UseEmbeddedModel(NULL,sizeof(data)));
    assert(!PortGame_UseEmbeddedModel(data,-1));
    assert(!PortGame_UseEmbeddedModel(data,1015));
    w32(data,28);assert(!PortGame_UseEmbeddedModel(data,sizeof(data)));w32(data,252);
    w32(data+4,0);assert(!PortGame_UseEmbeddedModel(data,sizeof(data)));w32(data+4,1024);
    w32(data+8,1024);assert(!PortGame_UseEmbeddedModel(data,sizeof(data)));w32(data+8,1280);
    w32(data+20,4096);assert(!PortGame_UseEmbeddedModel(data,sizeof(data)));w32(data+20,1280);
    w32(data+20,4);assert(!PortGame_UseEmbeddedModel(data,sizeof(data)));w32(data+20,1280);
    w32(data+8,1100);assert(!PortGame_UseEmbeddedModel(data,sizeof(data)));w32(data+8,1280);
    animation_scale_checks(data);
    {
        unsigned char extended[2048],animation[176];
        memcpy(extended,data,sizeof(data));w32(extended,253);
        w32(extended+253*4,1536);w32(extended+254*4,1792);
        memset(extended+1280,0x5A,256);memcpy(extended+1536,"BT3WARRI",8);
        wf(extended+1544,1.5f);w32(extended+1548,0);
        assert(PortGame_UseEmbeddedModel(extended,sizeof(extended)));
        memset(animation,0,sizeof(animation));animation[6]=37;animation[150]=1;
        wf(animation+152,1);wf(animation+156,-5);wf(animation+160,3);
        assert(PortGame_AdjustWarriorAnimation(extended,sizeof(extended),0,animation,sizeof(animation))==1);
        assert(rf(animation+156)==-7.5f && extended[1280]==0x5A);
        assert(!PortGame_EmbeddedBT4(extended,sizeof(extended)));
        memcpy(extended+1536,"BT4WARRI",8);
        assert(PortGame_EmbeddedBT4(extended,sizeof(extended)));
        extended[1536]='X';assert(!PortGame_UseEmbeddedModel(extended,sizeof(extended)));
        extended[1536]='B';w32(extended+254*4,1540);
        assert(!PortGame_UseEmbeddedModel(extended,sizeof(extended)));
    }
    for(i=1;i<argc;i++) {
        FILE *f=fopen(argv[i],"rb");long size;void *p;
        assert(f);fseek(f,0,SEEK_END);size=ftell(f);rewind(f);
        p=malloc(size);assert(p);assert(fread(p,1,size,f)==(size_t)size);fclose(f);
        assert(PortGame_UseEmbeddedModel(p,(int)size));free(p);
    }
    puts("PASS: embedded-model gating and bounds checks");return 0;
}
