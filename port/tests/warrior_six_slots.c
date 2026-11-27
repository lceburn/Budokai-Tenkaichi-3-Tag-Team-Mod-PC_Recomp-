#include "plat_game.h"
#include "plat_warriors.h"
#include "warrior_net_pack.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
const char *Port_FileRoot(void){return getenv("BT3_DATA");}
void *Port_LowAlloc(size_t n){return malloc(n);}
int MenuLabels_MusicIds(const char *root,int *ids,int n){(void)root;(void)ids;(void)n;return 0;}
int main(void) {
    int i;assert(PortWarriors_Count()==6 && PortGame_CharacterCount()==257);
    for(i=0;i<6;i++) {
        int c=PortWarriors_LocalCharacter(i),id=PortGame_Model(c,0,0);
        assert(c!=255 && PortGame_WarriorCharacter(i)==c && PortGame_WarriorIndex(c)==i);
        assert(PortGame_ModelCharacter(id)==c && PortGame_IsCharacter(c));
        assert(PortGame_WarriorFormTarget(c,0)==PortWarriors_LocalCharacter((i+1)%6));
        assert(PortGame_CustomSlot(NULL,c)!=NULL);
    }
    assert(!PortGame_IsCharacter(255) && PortGame_WarriorIndex(255)==-1);
    { int grid[7][9]={{0}};for(i=0;i<7;i++)grid[i][0]=164;
      assert(PortGame_WarriorGrid(grid,7,7)==7 && grid[0][1]==6);
      for(i=0;i<6;i++)assert(grid[0][i+2]==PortWarriors_LocalCharacter(i)); }
    { unsigned char *bytes=NULL;unsigned size=0,base=0;
      assert(WarriorNetPack_ExportLocal(5,&bytes,&size,&base));
      char room[WNP_ROOT_SIZE],paths[6][WNP_ROOT_SIZE];const char *roots[6];
      assert(size && base==5 && WarriorNetPack_Check(bytes,size)==6);
      assert(WarriorNetPack_CreateRoom(room));
      assert(WarriorNetPack_Install(bytes,size,room,0,paths,6)==6);
      for(i=0;i<6;i++)roots[i]=paths[i];
      assert(PortWarriors_Session(roots,6));
      assert(PortGame_IsCustomCharacter(PORT_WARRIOR_SESSION_FIRST+5));
      PortGame_LocalPicker(1);
      assert(PortGame_WarriorIndex(256)==PORT_WARRIOR_LOCAL_KEY+5);
      assert(PortGame_ModelCharacter(PortGame_Model(256,0,0))==256);
      PortGame_LocalPicker(0);
      assert(PortWarriors_Session(NULL,0) && WarriorNetPack_RemoveRoom(room));
      assert(PortWarriors_LocalCount()==6);WarriorNetPack_Free(bytes); }
    puts("PASS six saved fighters, sixth-form links, preserved IDs, equipment/grid routing and six-form network export");
}
