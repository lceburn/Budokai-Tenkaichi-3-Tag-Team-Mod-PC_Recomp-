/* Registry transaction tests; synthetic resources only, no game data. */
#include "plat_warriors.h"
#include "plat_game.h"
#include "warrior_net_pack.h"
#include "warrior_net_transfer.h"
#include "warrior_net_catalog.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#include <sys/stat.h>
#endif
const char *Port_FileRoot(void){return getenv("BT3_DATA");}
void *Port_LowAlloc(size_t size){return malloc(size);}
int MenuLabels_MusicIds(const char *root,int *ids,int capacity){(void)root;(void)ids;(void)capacity;return 0;}
extern void PortGame_OnlineCustom(int enter);
typedef struct SendLoop {WntReceive receive;WntBudget budget;int block;unsigned chunks;} SendLoop;
static unsigned read32(const unsigned char *p) {
    return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];
}
static unsigned little32(const unsigned char *p) {return p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24;}
static void little_put(unsigned char *p,unsigned v) {unsigned i;for(i=0;i<4;i++)p[i]=(unsigned char)(v>>(8*i));}
static void malformed_resources(unsigned char *pack,unsigned size) {
    unsigned at=12,offset[PORT_WARRIOR_FILE_ROLES],length[PORT_WARRIOR_FILE_ROLES],i,saved,mesh,record,texture;
    for(i=0;i<8;i++){unsigned n=read32(pack+at);at+=4+n;assert(at<size);}
    at+=104;
    for(i=0;i<PORT_WARRIOR_FILE_ROLES;i++){length[i]=read32(pack+at);offset[i]=at+4;at+=4+length[i];assert(at<=size);}
    saved=little32(pack+offset[0]+4);little_put(pack+offset[0]+4,0xffffffffu);
    assert(!WarriorNetPack_Check(pack,size));little_put(pack+offset[0]+4,saved);
    mesh=offset[0]+little32(pack+offset[0]+12);record=mesh+little32(pack+mesh+0x6c);
    saved=little32(pack+record);little_put(pack+record,63);
    assert(!WarriorNetPack_Check(pack,size));little_put(pack+record,saved);
    texture=offset[0]+little32(pack+offset[0]+48);saved=little32(pack+texture);
    little_put(pack+texture,0xffffffffu);assert(!WarriorNetPack_Check(pack,size));little_put(pack+texture,saved);
    saved=little32(pack+offset[PORT_WARRIOR_ANIMATION]);little_put(pack+offset[PORT_WARRIOR_ANIMATION],0xffffffffu);
    assert(!WarriorNetPack_Check(pack,size));little_put(pack+offset[PORT_WARRIOR_ANIMATION],saved);
    saved=little32(pack+offset[PORT_WARRIOR_PORTRAIT]+4);little_put(pack+offset[PORT_WARRIOR_PORTRAIT]+4,length[PORT_WARRIOR_PORTRAIT]);
    assert(!WarriorNetPack_Check(pack,size));little_put(pack+offset[PORT_WARRIOR_PORTRAIT]+4,saved);
    assert(WarriorNetPack_Check(pack,size)==1);
    puts("PASS: malformed model offsets, mesh extents, texture tables, animations and portrait lengths rejected");
}
static int send_loop(void *context,int kind,const void *data,unsigned size) {
    SendLoop *loop=context;const unsigned char *p=data;
    if(loop->block)return 0;
    if(kind==WNT_SEND_BEGIN) {
        assert(size==48);
        assert(Wnt_Begin(&loop->receive,&loop->budget,read32(p),read32(p+4),
                         read32(p+8),read32(p+12),p+16,100));
    }else if(kind==WNT_SEND_CHUNK) {
        assert(size>12 && size<=12+WNT_CHUNK);loop->chunks++;
        assert(Wnt_Chunk(&loop->receive,read32(p),read32(p+4),read32(p+8),p+12,size-12,100));
    }else {assert(kind==WNT_SEND_END && size==8);assert(Wnt_Finish(&loop->receive,read32(p),read32(p+4),100));}
    return 1;
}
static void sender(const unsigned char *pack,unsigned size) {
    WntSend s={0},before;SendLoop loop={0};unsigned iterations=0;
    assert(!Wnt_SendBegin(&s,pack,size,0,3,1,100));
    assert(!Wnt_SendBegin(&s,pack,size,7,3,2,100));
    assert(Wnt_SendBegin(&s,pack,size,7,3,1,100));
    assert(!Wnt_SendBegin(&s,pack,size,8,3,1,100));
    before=s;loop.block=1;assert(!Wnt_SendStep(&s,send_loop,&loop,100));
    assert(!memcmp(&s,&before,sizeof(s)));loop.block=0;
    while(s.state) {
        before=s;loop.block=1;assert(!Wnt_SendStep(&s,send_loop,&loop,100));
        assert(!memcmp(&s,&before,sizeof(s)));loop.block=0;
        assert(Wnt_SendStep(&s,send_loop,&loop,100)==1);
        assert(++iterations<=3+size/WNT_CHUNK);
    }
    assert(loop.chunks==(size+WNT_CHUNK-1)/WNT_CHUNK);
    assert(loop.receive.state==WNT_VERIFIED && !memcmp(loop.receive.data,pack,size));
    Wnt_Cancel(&loop.receive);
    assert(!loop.budget.bytes && !loop.budget.forms);
    assert(!Wnt_SendStep(&s,send_loop,&loop,100));
    assert(Wnt_SendBegin(&s,pack,size,7,3,1,100));
    loop.block=1;assert(Wnt_SendStep(&s,send_loop,&loop,100+WNT_TIMEOUT_MS)==-1);
    assert(!s.state && !s.data);Wnt_SendCancel(&s);
    puts("PASS: bounded sender frames round trip with backpressure, cancellation and stall timeout");
}
static void catalog(const unsigned char *pack,unsigned size) {
    WncCatalog host,client,previous;unsigned char hash[WNT_HASH],wrong[WNT_HASH],digest[WNT_HASH],wire[WNC_WIRE_MAX];
    unsigned revision,n,i;int id;
    Wnc_Init(&host);Wnc_Init(&client);Wnt_Digest(pack,size,hash);memcpy(wrong,hash,sizeof(hash));wrong[0]^=1;
    assert(Wnc_Member(&host,0,1,0,1) && Wnc_Member(&host,1,1,1,7));
    assert(Wnc_Admit(&host,1,6,1,hash)==-1);
    assert(Wnc_Admit(&host,0,1,1,hash)==PORT_WARRIOR_SESSION_FIRST);
    assert(Wnc_Admit(&host,1,7,1,hash)==PORT_WARRIOR_SESSION_FIRST+1);
    revision=host.revision;
    assert(Wnc_Admit(&host,1,7,1,hash)==PORT_WARRIOR_SESSION_FIRST+1 && host.revision==revision);
    assert(!Wnc_CanReady(&host));
    assert(!Wnc_Verified(&host,0,wrong) && Wnc_Verified(&host,0,hash));
    Wnc_Digest(&host,digest);
    assert(!Wnc_Ack(&host,1,6,revision,digest));
    assert(!Wnc_Ack(&host,1,7,revision-1,digest));
    assert(!Wnc_Ack(&host,1,7,revision,wrong));
    assert(Wnc_Ack(&host,0,1,revision,digest) && Wnc_Ack(&host,1,7,revision,digest));
    assert(!Wnc_CanStart(&host));assert(Wnc_Verified(&host,1,hash) && Wnc_CanStart(&host));
    n=Wnc_Encode(&host,wire,sizeof(wire));assert(n && Wnc_Decode(&client,wire,n));
    assert(!Wnc_CanReady(&client));assert(Wnc_Verified(&client,0,hash) && Wnc_Verified(&client,1,hash));
    assert(Wnc_CanReady(&client));
    host.locked=1;assert(Wnc_Admit(&host,0,1,1,wrong)==-1);
    assert(Wnc_Member(&host,1,0,1,7)); /* Active resources survive disconnect. */
    assert(host.forms==2 && host.entries[1].first==PORT_WARRIOR_SESSION_FIRST+1);
    assert(Wnc_Member(&host,1,1,1,8) && !Wnc_CanStart(&host));
    assert(!Wnc_Ack(&host,1,7,revision,digest));assert(Wnc_Ack(&host,1,8,revision,digest));
    assert(Wnc_CanStart(&host));host.locked=0;
    for(i=2;i<PORT_WARRIOR_SESSION_MAX;i++) {
        memcpy(wrong,hash,sizeof(hash));wrong[0]^=(unsigned char)i;
        if(i==PORT_WARRIOR_SESSION_MAX-2) {
            previous=host;assert(Wnc_Admit(&host,0,1,4,wrong)==-1);
            assert(!memcmp(&host,&previous,sizeof(host))); /* Never admit part of a form family. */
        }
        id=Wnc_Admit(&host,0,1,1,wrong);assert(id==PORT_WARRIOR_SESSION_FIRST+(int)i);
    }
    memcpy(wrong,hash,sizeof(hash));wrong[0]^=99;
    assert(Wnc_Admit(&host,0,1,1,wrong)==-1 && host.forms==PORT_WARRIOR_SESSION_MAX);
    assert(!Wnc_CanStart(&host)); /* Previous ready acknowledgements are stale. */
    n=Wnc_Encode(&host,wire,sizeof(wire));assert(Wnc_Decode(&client,wire,n));
    assert(client.entries[0].verified && client.entries[1].verified && !client.entries[2].verified);
    previous=client;
    for(i=0;i<n;i++)assert(!Wnc_Decode(&client,wire,i));
    wire[31]^=1;assert(!Wnc_Decode(&client,wire,n));wire[31]^=1;
    assert(!memcmp(&client,&previous,sizeof(client)));
    assert(Wnc_Member(&host,19,1,-1,12));assert(Wnc_Admit(&host,19,12,1,hash)==-1);
    puts("PASS: shared catalog IDs, nineteen-form admission, stale readiness, generations and immutable active mappings");
}
static void transfer(const unsigned char *pack,unsigned size) {
    WntBudget budget={0},full={WNT_ROOM_BYTES,PORT_WARRIOR_SESSION_MAX};
    WntReceive r={0},other={0};unsigned char hash[WNT_HASH],*retained;unsigned at,retainedSize=0;
    Wnt_Digest(pack,size,hash);
    sender(pack,size);
    assert(!Wnt_Begin(&other,&full,1,1,size,1,hash,0) && !other.data);
    assert(!Wnt_Begin(&r,&budget,0,1,size,1,hash,0));
    assert(!Wnt_Begin(&r,&budget,1,0,size,1,hash,0));
    assert(Wnt_Begin(&r,&budget,7,3,size,1,hash,100));
    assert(!Wnt_Take(&r,&retainedSize) && !retainedSize && r.data);
    assert(budget.bytes==size && budget.forms==1);
    assert(!Wnt_Begin(&r,&budget,8,3,size,1,hash,101));
    assert(!Wnt_Chunk(&r,7,2,0,pack,1,101)); /* Stale participant generation. */
    assert(!Wnt_Chunk(&r,7,3,1,pack,1,101)); /* Out of order. */
    assert(!Wnt_Chunk(&r,7,3,0,pack,WNT_CHUNK+1,101));
    assert(r.received==0 && !Wnt_Finish(&r,7,3,101));
    for(at=0;at<size;) {
        unsigned n=size-at;if(n>64)n=64;
        assert(Wnt_Chunk(&r,7,3,at,pack+at,n,200+at));at+=n;
    }
    assert(!Wnt_Chunk(&r,7,3,0,pack,1,201+size));
    assert(Wnt_Finish(&r,7,3,202+size) && r.state==WNT_VERIFIED);
    assert(!Wnt_Expire(&r,UINT64_MAX)); /* Validated data stays until the room releases it. */
    assert(!memcmp(r.data,pack,size));
    retained=Wnt_Take(&r,&retainedSize);
    assert(retained && retainedSize==size && !r.state && !r.data);
    assert(!budget.bytes && !budget.forms);
    Wnt_Cancel(&r); /* Room relay owns the buffer; cancelling staging must not free it. */
    assert(!memcmp(retained,pack,size));
    assert(!Wnt_Take(&r,&retainedSize));
    free(retained);
    assert(!budget.bytes && !budget.forms && !r.data);
    hash[0]^=1;assert(Wnt_Begin(&r,&budget,7,3,size,1,hash,0));
    for(at=0;at<size;){unsigned n=size-at;if(n>WNT_CHUNK)n=WNT_CHUNK;assert(Wnt_Chunk(&r,7,3,at,pack+at,n,1));at+=n;}
    assert(!Wnt_Finish(&r,7,3,2) && !r.data && !budget.bytes);
    hash[0]^=1;assert(Wnt_Begin(&r,&budget,7,3,size,2,hash,0));
    for(at=0;at<size;){unsigned n=size-at;if(n>WNT_CHUNK)n=WNT_CHUNK;assert(Wnt_Chunk(&r,7,3,at,pack+at,n,1));at+=n;}
    assert(!Wnt_Finish(&r,7,3,2) && !budget.forms); /* Wrong declared form count. */
    assert(Wnt_Begin(&r,&budget,9,4,size,1,hash,10));
    assert(Wnt_Expire(&r,10+WNT_TIMEOUT_MS) && !r.data && !budget.bytes);
    assert(!Wnt_Begin(&r,&budget,1,1,size,1,hash,UINT64_MAX));
    Wnt_Cancel(&r);Wnt_Cancel(&r);
    puts("PASS: bounded chunks, generations, corruption detection, budget release, timeout and cancellation");
}
int main(void) {
    char storage[PORT_WARRIOR_SESSION_MAX][128],path[1024];
    const char *roots[PORT_WARRIOR_SESSION_MAX],*bad[1];int i;unsigned char *localSlot;
    if(getenv("SESSION_BOOTSTRAP")) {
        assert(PortGame_IsCustomCharacter(PORT_WARRIOR_SESSION_FIRST));
        assert(PortGame_WarriorCharacter(0)==PORT_WARRIOR_SESSION_FIRST);
        assert(PortGame_WarriorFormTarget(PORT_WARRIOR_SESSION_FIRST,0)==PORT_WARRIOR_SESSION_FIRST+1);
        assert(!PortGame_IsCustomCharacter(PORT_WARRIOR_FIRST_CHARACTER) && !PortGame_IsCharacter(255));
        assert(PortGame_CharacterCount()==PORT_WARRIOR_SESSION_FIRST+PORT_WARRIOR_SESSION_MAX);
        assert(PortWarriors_Session(NULL,0) && PortGame_IsCustomCharacter(PORT_WARRIOR_FIRST_CHARACTER));
        puts("PASS: first-use session lookup initializes before choosing the character namespace");return 0;
    }
    if(getenv("PACK_CLOSURE")) {
        unsigned char *pack=NULL;unsigned size=0,base=0;char installed[4][WNP_ROOT_SIZE],room[WNP_ROOT_SIZE];const char *selected[2];
        assert(PortWarriors_Count()==3);
        assert(!WarriorNetPack_Export(-1,&pack,&size,&base) && !pack);
        assert(WarriorNetPack_Export(1,&pack,&size,&base)==2 && base==1 && WarriorNetPack_Check(pack,size)==2);
        {
            unsigned char *same=NULL;unsigned sameSize=0,sameBase=99;
            assert(WarriorNetPack_Export(0,&same,&sameSize,&sameBase)==2 && sameBase==0 && sameSize==size);
            assert(!memcmp(same,pack,size));WarriorNetPack_Free(same);
        }
        assert(WarriorNetPack_CreateRoom(room));
        assert(WarriorNetPack_Install(pack,size,room,PORT_WARRIOR_SESSION_MAX-1,installed,4)==2);
        selected[0]=installed[0];selected[1]=installed[1];assert(PortWarriors_Session(selected,2));
        assert(!strcmp(PortWarriors_Name(0),"A") && !strcmp(PortWarriors_Name(1),"B"));
        assert(PortWarriors_FormTarget(0,0)==1 && PortWarriors_FormTarget(1,0)==0);
        assert(PortWarriors_Session(NULL,0) && PortWarriors_Count()==3);
        assert(WarriorNetPack_RemoveRoom(room));
        WarriorNetPack_Free(pack);puts("PASS: selected fighter includes its cyclic form closure, excludes unused locals, and remaps links");return 0;
    }
    assert(PortWarriors_Count()==1 && !strcmp(PortWarriors_Name(0),"Local"));
    localSlot=PortGame_CustomSlot(NULL,250);localSlot[0]=46;
    {
        unsigned char *pack=NULL;unsigned size=0,n,base=0;char installed[4][WNP_ROOT_SIZE],room[WNP_ROOT_SIZE];const char *one[1];
        assert(WarriorNetPack_Export(0,&pack,&size,&base)==1 && !base && pack && size);
        assert(WarriorNetPack_Check(pack,size)==1);
        malformed_resources(pack,size);
        assert(WarriorNetPack_CheckSelection(pack,size,0,0));
        assert(!WarriorNetPack_CheckSelection(pack,size,1,0));
        assert(!WarriorNetPack_CheckSelection(pack,size,0,4));
        transfer(pack,size);
        catalog(pack,size);
        for(n=0;n<size;n++)assert(!WarriorNetPack_Check(pack,n));
        assert(WarriorNetPack_CreateRoom(room));
        assert(!WarriorNetPack_Install(pack,size,".warrior-sessions/../mods",0,installed,4));
        assert(!WarriorNetPack_Install(pack,size,".warrior-sessions/foreign",0,installed,4));
        assert(!WarriorNetPack_Install(pack,size,room,PORT_WARRIOR_SESSION_MAX,installed,4));
        assert(WarriorNetPack_Install(pack,size,room,0,installed,4)==1);
        one[0]=installed[0];assert(PortWarriors_Session(one,1));
        assert(!WarriorNetPack_RemoveRoom(room));
        assert(!strcmp(PortWarriors_Name(0),"Local"));
        {unsigned char *blocked=NULL;unsigned blockedBase=0;
        assert(!WarriorNetPack_Export(0,&blocked,&n,&blockedBase) && !blocked); /* Cannot re-export received session data. */
        assert(PortWarriors_LocalCount()==1);
        assert(WarriorNetPack_ExportLocal(0,&blocked,&n,&blockedBase)==1 && n==size && !blockedBase);
        assert(!memcmp(blocked,pack,size));WarriorNetPack_Free(blocked);
        assert(PortWarriors_SessionActive() && PortWarriors_Count()==1);
        {PortWarriorLocal snapshot;assert(PortWarriors_LocalCopy(0,&snapshot));
         assert(strstr(snapshot.file[PORT_WARRIOR_MODEL_1],"/mods/") &&
                !strstr(snapshot.file[PORT_WARRIOR_MODEL_1],".warrior-sessions"));}
        }
        assert(PortWarriors_Session(NULL,0));
        assert(!WarriorNetPack_Install(pack,size,room,0,installed,4));
        assert(!WarriorNetPack_RemoveRoom(".warrior-sessions/probe"));
#ifndef _WIN32
        {
            char outside[1024],link[1024];
            snprintf(outside,sizeof(outside),"%s/outside",Port_FileRoot());assert(!mkdir(outside,0700));
            snprintf(link,sizeof(link),"%s/%s/link",Port_FileRoot(),room);assert(!symlink(outside,link));
            assert(!WarriorNetPack_RemoveRoom(room));assert(!unlink(link));
        }
#endif
        assert(WarriorNetPack_RemoveRoom(room) && !WarriorNetPack_RemoveRoom(room));
        pack[0]^=1;assert(!WarriorNetPack_Check(pack,size));
        WarriorNetPack_Free(pack);
    }
    assert(PortGame_IsCustomCharacter(250) && !PortGame_IsOnlineCharacter(250));
    PortGame_OnlineCustom(1);assert(!PortGame_IsCustomCharacter(250));
    for(i=0;i<PORT_WARRIOR_SESSION_MAX;i++) {
        snprintf(storage[i],sizeof(storage[i]),".warrior-sessions/probe/form%02d",i);roots[i]=storage[i];
    }
    assert(!PortWarriors_Session(roots,PORT_WARRIOR_SESSION_MAX+1));
    bad[0]=".warrior-sessions/probe/../form00";assert(!PortWarriors_Session(bad,1));
    bad[0]="mods/Local";assert(!PortWarriors_Session(bad,1));
    bad[0]=".warrior-sessions/probe/missing";assert(!PortWarriors_Session(bad,1));
    assert(PortWarriors_Count()==1 && !PortWarriors_Error());
    assert(PortWarriors_Session(roots,PORT_WARRIOR_SESSION_MAX));
    assert(PortWarriors_SessionActive() && PortWarriors_Count()==PORT_WARRIOR_SESSION_MAX);
    {
        int cells[7][9]={{0}};unsigned char *room=PortGame_CustomSlot(NULL,PORT_WARRIOR_SESSION_FIRST);
        unsigned char saved=room[0];
        for(i=0;i<7;i++)cells[i][0]=164;cells[0][0]=0;cells[1][0]=3;
        PortGame_LocalPicker(1);
        assert(PortGame_IsCustomCharacter(250) && PortGame_IsCustomCharacter(PORT_WARRIOR_SESSION_FIRST));
        assert(!PortGame_IsOnlineCharacter(250) && PortGame_IsOnlineCharacter(PORT_WARRIOR_SESSION_FIRST));
        assert(PortGame_WarriorIndex(250)==PORT_WARRIOR_LOCAL_KEY);
        assert(PortGame_ModelCharacter(PortGame_Model(250,0,0))==250);
        assert(!strcmp(PortGame_CharacterName(250),"Local") && !strcmp(PortGame_CharacterName(PORT_WARRIOR_SESSION_FIRST),"Remote"));
        assert(PortGame_CustomSlot(NULL,250)==localSlot && room[0]==saved);
        assert(PortGame_WarriorGrid(cells,7,7)==7 && cells[0][0]==0 && cells[1][0]==3 && cells[2][0]==250);
        snprintf(path,sizeof(path),"zwarriors/%d/0.bin",PORT_WARRIOR_LOCAL_KEY);
        {char resolved[1024];assert(PortWarriors_Alias(path,resolved,sizeof(resolved))==1 && strstr(resolved,"mods/Local/"));}
        assert(PortWarriors_Count()==PORT_WARRIOR_SESSION_MAX);
        PortGame_LocalPicker(0);
        assert(!PortGame_IsCustomCharacter(250) && PortGame_IsOnlineCharacter(PORT_WARRIOR_SESSION_FIRST));
    }
    {
        unsigned char *room=PortGame_CustomSlot(NULL,PORT_WARRIOR_SESSION_FIRST);unsigned epoch=PortWarriors_SessionEpoch();
        assert(room!=localSlot && !room[0]);room[0]=77;
        assert(PortWarriors_Session(roots,PORT_WARRIOR_SESSION_MAX) && PortWarriors_SessionEpoch()==epoch);
        assert(((unsigned char *)PortGame_CustomSlot(NULL,PORT_WARRIOR_SESSION_FIRST))[0]==77);
    }
    for(i=0;i<PORT_WARRIOR_SESSION_MAX;i++) {
        int c=PortGame_WarriorCharacter(i),model=PortGame_Model(c,0,0);
        assert(c==PORT_WARRIOR_SESSION_FIRST+i && c!=255);
        assert(PortGame_WarriorIndex(c)==i && PortGame_IsOnlineCharacter(c));
        assert(PortGame_ModelCharacter(model)==c);
        assert(PortGame_WarriorFormTarget(c,0)==PortGame_WarriorCharacter((i+1)%PORT_WARRIOR_SESSION_MAX));
        assert(PortGame_CustomSlot(NULL,c)!=NULL);
        assert(!strcmp(PortWarriors_Name(i),"Remote"));
        assert(PortWarriors_FormTarget(i,0)==(i+1)%PORT_WARRIOR_SESSION_MAX);
    }
    assert(PortWarriors_Alias("zwarriors/18/0.bin",path,sizeof(path))==1);
    assert(strstr(path,".warrior-sessions/probe/form18/"));
    assert(!PortWarriors_Session(roots,1)); /* Missing linked form. */
    assert(PortWarriors_Count()==PORT_WARRIOR_SESSION_MAX && !PortWarriors_Error());
    PortWarriors_Reload();assert(PortWarriors_Count()==PORT_WARRIOR_SESSION_MAX);
    assert(PortWarriors_Session(NULL,0));assert(!PortWarriors_SessionActive());
    assert(!PortGame_IsOnlineCharacter(PORT_WARRIOR_SESSION_FIRST));
    PortGame_OnlineCustom(0);assert(PortGame_IsCustomCharacter(250));
    assert(((unsigned char *)PortGame_CustomSlot(NULL,250))[0]==46);
    assert(PortWarriors_Session(roots,PORT_WARRIOR_SESSION_MAX));
    assert(!((unsigned char *)PortGame_CustomSlot(NULL,PORT_WARRIOR_SESSION_FIRST))[0]);
    assert(PortWarriors_Session(NULL,0) && localSlot[0]==46);
    assert(PortWarriors_Count()==1 && !strcmp(PortWarriors_Name(0),"Local"));
    assert(PortWarriors_Alias("zwarriors/0/0.bin",path,sizeof(path))==1 && strstr(path,"mods/Local/"));
    assert(PortWarriors_Session(NULL,0));
    puts("PASS: nineteen session forms, links, routing, atomic rejection and local restoration");
}
