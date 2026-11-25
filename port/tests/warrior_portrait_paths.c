#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/plat_file.c"
#include "../src/plat_warriors.c"
int PortGame_IsBT4(void){return 1;}
int PortGame_WarriorModelsEnabled(void){return 1;}
int PortStages_Alias(const char *r,char *o,unsigned n){return 0;}
int PortSongs_Alias(const char *r,char *o,unsigned n){return 0;}
int main(int argc,char **argv){
 char path[1600];assert(argc==2 && PortWarriors_Count()==1);
 assert(Port_FilePath(7,PORT_WARRIOR_PORTRAIT,NULL,path,sizeof(path)) && strstr(path,"/mods/Local/Fighter_img.cdbt"));
 assert(!Port_FilePath(7,99*16+PORT_WARRIOR_PORTRAIT,NULL,path,sizeof(path)) && !path[0]);
 assert(PortWarriors_Preview(1));
 assert(Port_FilePath(7,PORT_WARRIOR_PORTRAIT,NULL,path,sizeof(path)) && strstr(path,"/.warrior-builds/probe/Preview/Fighter_img.cdbt"));
 assert(PortWarriors_Preview(0));
 const char *room=argv[1];assert(PortWarriors_Session(&room,1));
 assert(Port_FilePath(7,PORT_WARRIOR_PORTRAIT,NULL,path,sizeof(path)) && strstr(path,"/.warrior-sessions/") && strlen(path)>260);
 assert(Port_FilePath(7,PORT_WARRIOR_LOCAL_KEY*16+PORT_WARRIOR_PORTRAIT,NULL,path,sizeof(path)) && strstr(path,"/mods/Local/"));
 assert(PortWarriors_Session(NULL,0));
 assert(Port_FilePath(7,PORT_WARRIOR_PORTRAIT,NULL,path,sizeof(path)) && strstr(path,"/mods/Local/"));
 puts("PASS portrait paths: local selection, authoring preview, long temporary-room paths, local picker during room, cleanup, no invalid-ID fallback");
}
