/* Optional real-disc mapping probe. Reads private data; does not launch a fight. */
#include "plat_game.h"
#include "plat_warriors.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
const char *Port_FileRoot(void) {return getenv("BT3_DATA");}
void *Port_LowAlloc(size_t n) {return malloc(n);}
int MenuLabels_MusicIds(const char *root,int *ids,int capacity) {
    (void)root;(void)ids;(void)capacity;return 0;
}
int main(int argc,char **argv) {
    unsigned original[250*4],*extended;int i,count;
    assert(argc==2);count=atoi(argv[1]);
    assert(PortGame_IsBT4());assert(PortGame_CharacterCount()==250+count);
    for(i=0;i<count;i++) {
        int c=250+i,id=PortGame_Model(c,0,0);char relative[80],path[1024];FILE *f;
        assert(PortGame_IsCharacter(c) && PortGame_IsCustomCharacter(c));
        assert(!PortGame_UsesBT4Rules(c));
        assert(PortGame_TableCharacter(c)==52);
        assert(!strcmp(PortGame_CharacterName(c),PortWarriors_Name(i)));
        assert(PortGame_Costumes(c)==2 && PortGame_ModelCharacter(id)==c);
        assert(PortGame_Partition(id)==7);
        assert(PortGame_FileIndex(id)==i*PORT_WARRIOR_FILE_STRIDE);
        assert(PortGame_Voice(c,0)==PortGame_Voice(80,0));
        assert(PortGame_Animation(c)==id+PORT_WARRIOR_ANIMATION);
        assert(PortGame_Combat(c)==id+PORT_WARRIOR_EFFECTS);
        assert(PortGame_Member(c,0)==id+PORT_WARRIOR_MEMBER_1);
        assert(PortGame_Member(c,1)==id+PORT_WARRIOR_MEMBER_2);
        if(count==2) {
            assert(PortGame_WarriorFormTarget(c,0)==250+(1-i));
            assert(PortGame_WarriorFormField(c,0,0)==1);
            assert(PortGame_WarriorFormTarget(c,1)==0xFF);
            assert(PortGame_WarriorFormTarget(c,-1)==0xFF);
            assert(PortGame_WarriorFormTarget(c,4)==0xFF);
        }
        snprintf(relative,sizeof(relative),"zwarriors/%05d.bin",PortGame_FileIndex(id));
        assert(PortWarriors_Alias(relative,path,sizeof(path))==1);
        f=fopen(path,"rb");assert(f);fclose(f);
    }
    assert(!PortGame_IsCustomCharacter(249) && !PortGame_IsCustomCharacter(250+count));
    assert(PortGame_WarriorFormTarget(52,0)==0xFF);
    assert(PortGame_WarriorFormTarget(250+count,0)==0xFF);
    assert(PortGame_WarriorFormField(52,0,0)==-1);
    assert(PortGame_UsesBT4Rules(52));
    {
        int grid[165][9],copy[165][9],j;
        memset(grid,0,sizeof(grid));
        for(j=155;j<165;j++)grid[j][0]=164;
        memcpy(copy,grid,sizeof(grid));
        assert(PortGame_WarriorGrid(grid,161,165)==161);
        assert(!memcmp(copy,grid,155*sizeof(grid[0])));
        if(count==2) {
            assert(grid[155][0]==250 && grid[155][1]==2);
            assert(grid[155][2]==250 && grid[155][3]==251);
            assert(grid[156][0]==164);
            assert(PortGame_MenuPortrait(250)==PortGame_Model(250,0,0)+PORT_WARRIOR_PORTRAIT);
            assert(PortGame_WarriorPortraitValid(250) && PortGame_WarriorPortraitValid(251));
            assert(PortGame_WarriorChip(250)==PortGame_Model(250,0,0)+PORT_WARRIOR_CHIP);
            assert(PortGame_WarriorChip(251)==PortGame_Model(251,0,0)+PORT_WARRIOR_CHIP);
            memcpy(grid,copy,sizeof(grid));
            assert(PortGame_WarriorGrid(grid,155,155)==155);
            assert(!memcmp(copy,grid,sizeof(grid)));
        } else assert(!memcmp(copy,grid,sizeof(grid)));
        assert(PortGame_MenuPortrait(52)==0x71C+52);
    }
    for(i=0;i<250*4;i++)original[i]=i+1;
    extended=PortGame_GaugeTable(original);
    assert(!memcmp(original,extended,sizeof(original)));
    for(i=0;i<count;i++)assert(!memcmp(extended+(250+i)*4,original+52*4,16));
    puts("PASS: custom slot resources, donor resolution and gauge-table extension");return 0;
}
