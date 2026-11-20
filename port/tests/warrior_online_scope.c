/* Disc-only network eligibility and restoration of the local creator roster. */
#include "plat_game.h"
#include "plat_warriors.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
const char *Port_FileRoot(void){return getenv("BT3_DATA");}
void *Port_LowAlloc(size_t n){return malloc(n);}
int MenuLabels_MusicIds(const char *root,int *ids,int capacity){(void)root;(void)ids;(void)capacity;return 0;}
extern void PortGame_OnlineCustom(int);
int main(void){
    int count,stock[250],i;assert(PortGame_IsBT4() && PortGame_WarriorModelsEnabled());
    count=PortGame_CharacterCount();assert(count>250);
    for(i=0;i<250;i++)stock[i]=PortGame_IsOnlineCharacter(i);
    assert(!PortGame_IsOnlineCharacter(-1) && !PortGame_IsOnlineCharacter(164));
    for(i=250;i<count;i++)assert(PortGame_IsCharacter(i) && !PortGame_IsOnlineCharacter(i));
    PortGame_OnlineCustom(1);assert(PortGame_WarriorModelsEnabled());assert(PortGame_CharacterCount()==250);
    for(i=0;i<250;i++)assert(stock[i]==PortGame_IsCharacter(i) && stock[i]==PortGame_IsOnlineCharacter(i));
    for(i=250;i<count;i++)assert(!PortGame_IsCharacter(i) && !PortGame_IsOnlineCharacter(i));
    PortGame_OnlineCustom(1);assert(PortGame_CharacterCount()==250);
    PortGame_OnlineCustom(0);assert(PortGame_CharacterCount()==count);
    for(i=250;i<count;i++)assert(PortGame_IsCharacter(i) && !PortGame_IsOnlineCharacter(i));
    PortGame_OnlineCustom(0);assert(PortGame_CharacterCount()==count);
    puts("PASS: creator stays enabled, online roster is disc-only, custom loadouts always rejected, local roster restored");
}
