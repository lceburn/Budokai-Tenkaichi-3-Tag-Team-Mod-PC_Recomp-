/* Private lobby implementation. Included after the config/transport helpers.
   Nothing here is rollback state or part of the permanent warrior library. */
typedef struct CustomBlob {
    unsigned char *data;
    unsigned size,forms;
    char roots[PORT_WARRIOR_PLAYABLE_MAX][WNP_ROOT_SIZE];
} CustomBlob;
static CustomBlob customBlobs[PORT_WARRIOR_SESSION_MAX];
static WntBudget customBudget;
static unsigned customRetainedBytes,customRegistered;
static char customRoot[WNP_ROOT_SIZE];
static struct {
    unsigned char *data;
    unsigned size,base,forms,fighter,revision,id;
    Uint64 deadline;
    int state,local,cpu;
    TlLoadout selection;
} customOffer;
static int customProbeDone;
extern void Port_FileWarriorClose(void);

static int custom_emit(void *context,int kind,const void *data,unsigned size) {
    Peer *p=context;
    /* Preserve room/input/control traffic headroom. Blocking must never call
       send_packet's disconnect-on-overflow path or advance the sender. */
    if(p->fd==TN_BAD || p->gone || size>MAX_PACKET-8 ||
       p->tn-p->off+size+8>QUEUE-32768)return 0;
    send_packet(p,kind==WNT_SEND_BEGIN?TN_CUSTOM_BEGIN:
                  kind==WNT_SEND_CHUNK?TN_CUSTOM_CHUNK:TN_CUSTOM_END,data,size);
    return p->fd!=TN_BAD && !p->gone;
}
static void custom_offer_clear(void) {
    /* The sender borrows these bytes. Cancel it before releasing the offer. */
    if(!host)Wnt_SendCancel(&peers[0].customTx);
    WarriorNetPack_Free(customOffer.data);memset(&customOffer,0,sizeof(customOffer));
}
static void custom_cleanup(void) {
    unsigned i;
    if(customWorldLive){customCleanupPending=1;return;}
    for(i=0;i<PEERS;i++) {
        Wnt_SendCancel(&peers[i].customTx);Wnt_Cancel(&peers[i].customRx);
        peers[i].customRequest=0;
    }
    custom_offer_clear();
    Port_FileWarriorClose();
    if(PortWarriors_SessionActive())PortWarriors_Session(NULL,0);
    for(i=0;i<PORT_WARRIOR_SESSION_MAX;i++) {
        WarriorNetPack_Free(customBlobs[i].data);memset(&customBlobs[i],0,sizeof(customBlobs[i]));
    }
    if(customRoot[0]) {
        if(!WarriorNetPack_RemoveRoom(customRoot))
            fprintf(stderr,"custom assets: refused unsafe or incomplete temporary-room cleanup\n");
        else fprintf(stderr,"custom assets: restored local registry and removed temporary room\n");
        customRoot[0]=0;
    }
    /* A new room cannot retain room-scoped IDs from the previous config. */
    for(i=0;i<TN_FIGHTERS;i++)if(config.chara[i]>=PORT_WARRIOR_SESSION_FIRST)config.chara[i]=0;
    for(i=0;i<TL_MEMBERS;i++) {
        if(partyRoom.members[i].loadout.chara>=PORT_WARRIOR_SESSION_FIRST)partyRoom.members[i].loadout.chara=0;
        if(config.lobby.members[i].loadout.chara>=PORT_WARRIOR_SESSION_FIRST)config.lobby.members[i].loadout.chara=0;
    }
    customRetainedBytes=customRegistered=0;memset(&customBudget,0,sizeof(customBudget));
    memset(customLocalRoom,0,sizeof(customLocalRoom));
    customCleanupPending=customProbeDone=0;
}
static int custom_activate(void) {
    const char *roots[PORT_WARRIOR_SESSION_MAX];unsigned i,j,n=0;
    if(customWorldLive)return 0;
    for(i=0;i<customCatalog.count;i++) {
        if(!customBlobs[i].data || customBlobs[i].forms!=customCatalog.entries[i].forms)return 0;
        for(j=0;j<customBlobs[i].forms;j++)roots[n++]=customBlobs[i].roots[j];
    }
    Port_FileWarriorClose();
    if(!PortWarriors_Session(roots,(int)n))return 0;
    customRegistered=customCatalog.count;
    fprintf(stderr,"custom assets: registered entries=%u forms=%u\n",customRegistered,n);return 1;
}
static int custom_install(unsigned index,unsigned char *bytes,unsigned size,unsigned forms) {
    CustomBlob *b;int n;
    if(index>=customCatalog.count || index>=PORT_WARRIOR_SESSION_MAX ||
       forms>PORT_WARRIOR_PLAYABLE_MAX || customBlobs[index].data ||
       size>WNT_ROOM_BYTES-customRetainedBytes || customWorldLive)return 0;
    if(!customRoot[0] && !WarriorNetPack_CreateRoom(customRoot))return 0;
    b=&customBlobs[index];
    n=WarriorNetPack_Install(bytes,size,customRoot,(int)index,b->roots,PORT_WARRIOR_PLAYABLE_MAX);
    if(n!=(int)forms)return 0;
    b->data=bytes;b->size=size;b->forms=forms;customRetainedBytes+=size;
    return Wnc_Verified(&customCatalog,index,customCatalog.entries[index].hash);
}
static int custom_fighter_owned(int owner,unsigned fighter,unsigned revision) {
    return phase==2 && !customWorldLive && !config.practice && !tournament.active &&
        revision==(unsigned)config.revision && owner>=0 && owner<TN_PLAYERS &&
        fighter<(unsigned)(config.size[0]+config.size[1]) &&
        (config.owner[fighter]==owner || (owner==0 && config.owner[fighter]<0));
}
#include "tag_net_deferred.h"
/* Takes ownership of bytes only on success. IDs are append-only and registration
   precedes config publication. No partial form family is admitted. */
static int custom_admit(int member,unsigned generation,unsigned fighter,unsigned base,
                         unsigned revision,unsigned char *bytes,unsigned size,unsigned forms,const TlLoadout *selection,int cpuLevel) {
    WncCatalog previous=customCatalog;unsigned char hash[WNT_HASH];
    int first;unsigned index;
    if(!selection || cpuLevel< -1 || cpuLevel>4 || base>=forms || selection->costume<0 ||
       !WarriorNetPack_CheckSelection(bytes,size,base,(unsigned)selection->costume) ||
       !custom_fighter_owned(customCatalog.members[member].owner,fighter,revision))return 0;
    for(index=0;index<8;index++)if(selection->items[index]<0 || selection->items[index]>350)return 0;
    Wnt_Digest(bytes,size,hash);first=Wnc_Admit(&customCatalog,member,generation,forms,hash);
    if(first<0)return 0;
    for(index=0;index<customCatalog.count;index++)if(customCatalog.entries[index].first==(unsigned)first)break;
    if(index==previous.count) {
        if(!custom_install(index,bytes,size,forms)){customCatalog=previous;return 0;}
        if(!custom_activate()){failed("Custom character resources could not be registered. Room stopped.");return 1;}
    }else WarriorNetPack_Free(bytes); /* Same family already has stable room IDs. */
    custom_pick_clear(fighter);
    {TagNetConfig c=config;c.chara[fighter]=first+(int)base;c.costume[fighter]=selection->costume;
     if(c.owner[fighter]<0)c.cpuLevel[fighter]=cpuLevel;
     memcpy(c.items[fighter],selection->items,sizeof(c.items[fighter]));apply_edit(&c);}
    fprintf(stderr,"custom assets: admitted owner=%u first=%d forms=%u\n",customCatalog.entries[index].owner,first,forms);
    return 1;
}
static int custom_share(int local,unsigned fighter,const TlLoadout *selection,int cpuLevel) {
    unsigned char *bytes=NULL;unsigned size=0,base=0,forms;int owner=host?0:role;
    TlLoadout chosen={0};unsigned k;
    if(selection)chosen=*selection;
    if(local<0 || local>=PORT_WARRIOR_PLAYABLE_MAX || chosen.costume<0 || chosen.costume>3 || cpuLevel< -1 || cpuLevel>4)return 0;
    for(k=0;k<8;k++)if(chosen.items[k]<0 || chosen.items[k]>350)return 0;
    if(customOffer.state || !custom_fighter_owned(owner,fighter,(unsigned)config.revision))return 0;
    if(!WarriorNetPack_ExportLocal(local,&bytes,&size,&base)) {
        say("Could not prepare that saved character for this room.");return 0;
    }
    forms=(unsigned)WarriorNetPack_Check(bytes,size);
    if(!forms){WarriorNetPack_Free(bytes);return 0;}
    if(!customSync)memset(ready,0,sizeof(ready));
    if(host) {
        int ok=custom_admit(0,customSelfGeneration,fighter,base,(unsigned)config.revision,bytes,size,forms,&chosen,cpuLevel);
        if(ok)customLocalRoom[local]=config.chara[fighter];
        if(!ok){WarriorNetPack_Free(bytes);say("That character family cannot be added to this room.");}
        return ok;
    }
    if(!peers[0].hello || !customCatalogReceived){WarriorNetPack_Free(bytes);return 0;}
    customOffer.data=bytes;customOffer.size=size;customOffer.base=base;customOffer.forms=forms;
    customOffer.fighter=fighter;customOffer.revision=(unsigned)config.revision;
    customOffer.local=local;customOffer.selection=chosen;
    customOffer.cpu=cpuLevel;
    customOffer.id=1;customOffer.state=1;customOffer.deadline=SDL_GetTicks()+WNT_TIMEOUT_MS;
    {unsigned char wire[100];
     put32(wire,customOffer.revision);put32(wire+4,fighter);put32(wire+8,base);
     put32(wire+12,customOffer.id);put32(wire+16,customSelfGeneration);
     put32(wire+20,size);put32(wire+24,forms);Wnt_Digest(bytes,size,wire+28);
     put32(wire+60,(unsigned)chosen.costume);for(k=0;k<8;k++)put32(wire+64+k*4,(unsigned)chosen.items[k]);
     put32(wire+96,(unsigned)cpuLevel);
     send_packet(&peers[0],TN_CUSTOM_OFFER,wire,sizeof(wire));}
    say("Sharing custom character and linked forms...");return 1;
}
static int custom_packet(Peer *p,int type,const unsigned char *data,unsigned n) {
    unsigned id=n>=4?get32(data):0,gen=n>=8?get32(data+4):0;
    Uint64 now=SDL_GetTicks();
    if(type<TN_CUSTOM_GET || type>TN_CUSTOM_RESULT)return 0;
    if(type==TN_CUSTOM_OFFER && host && n==100) {
        WncCatalog proposed=customCatalog;unsigned forms=get32(data+24),size=get32(data+20);
        int member=(int)(p-peers)+1;unsigned char reply[12];int ok;unsigned k;TlLoadout selection={0};
        selection.costume=(int)get32(data+60);for(k=0;k<8;k++)selection.items[k]=(int)get32(data+64+k*4);
        id=get32(data+12);gen=get32(data+16);
        ok=selection.costume>=0 && selection.costume<4 && (int)get32(data+96)>=-1 && (int)get32(data+96)<=4;
        for(k=0;k<8;k++)if(selection.items[k]<0 || selection.items[k]>350)ok=0;
        ok=ok && gen==p->customGeneration && !p->customRx.state &&
            get32(data+8)<forms && custom_fighter_owned(p->role,get32(data+4),get32(data)) &&
            Wnc_Admit(&proposed,member,gen,forms,data+28)>=0 &&
            customRetainedBytes<=WNT_ROOM_BYTES && customBudget.bytes<=WNT_ROOM_BYTES-customRetainedBytes &&
            size<=WNT_ROOM_BYTES-customRetainedBytes-customBudget.bytes &&
            Wnt_Begin(&p->customRx,&customBudget,id,gen,size,forms,data+28,now);
        put32(reply,id);put32(reply+4,gen);put32(reply+8,ok);
        if(ok) {
            p->customFighter=get32(data+4);p->customBase=get32(data+8);p->customRevision=get32(data);
            p->customSelection=selection;
            p->customCpu=(int)get32(data+96);
            if(!customSync)memset(ready,0,sizeof(ready));ready_packet();
        }
        send_packet(p,TN_CUSTOM_ACCEPT,reply,sizeof(reply));return 1;
    }
    if(type==TN_CUSTOM_ACCEPT && !host && n==12 && customOffer.state==1 &&
       id==customOffer.id && gen==customSelfGeneration) {
        if(!get32(data+8)){custom_offer_clear();say("The host could not add that character family. Try again from the lobby.");return 1;}
        if(!Wnt_SendBegin(&p->customTx,customOffer.data,customOffer.size,id,gen,customOffer.forms,now)) {
            failed("Could not start the custom character transfer.");return 1;
        }
        customOffer.state=2;return 1;
    }
    if(type==TN_CUSTOM_GET && host && n==8 && gen==p->customGeneration && id && id<=customCatalog.count) {
        CustomBlob *b=&customBlobs[id-1];
        if(!p->customTx.state && b->data)
            Wnt_SendBegin(&p->customTx,b->data,b->size,id,gen,b->forms,now);
        return 1;
    }
    if(type==TN_CUSTOM_BEGIN && n==48) {
        if(host) {
            WntReceive *r=&p->customRx;
            if(!r->state || id!=r->id || gen!=r->generation || get32(data+8)!=r->size ||
               get32(data+12)!=r->forms || memcmp(data+16,r->hash,WNT_HASH)) {
                failed("Custom character transfer metadata changed. Room stopped.");
            }
        }else {
            unsigned index=id-1,size=get32(data+8);WncEntry *e;
            if(!id || id!=p->customRequest || id>customCatalog.count || gen!=customSelfGeneration) {
                failed("An unexpected custom character transfer was received.");return 1;
            }
            e=&customCatalog.entries[index];
            if(get32(data+12)!=e->forms || memcmp(data+16,e->hash,WNT_HASH) ||
               size>WNT_ROOM_BYTES-customRetainedBytes || customWorldLive ||
               !Wnt_Begin(&p->customRx,&customBudget,id,gen,size,e->forms,e->hash,now))
                failed("The custom character download is invalid or exceeds the room limit.");
        }
        return 1;
    }
    if(type==TN_CUSTOM_CHUNK && n>12 && n<=12+WNT_CHUNK) {
        if(!Wnt_Chunk(&p->customRx,id,gen,get32(data+8),data+12,n-12,now))
            failed("The custom character download is incomplete or out of order.");
        return 1;
    }
    if(type==TN_CUSTOM_END && n==8) {
        unsigned char *bytes;unsigned size,forms=p->customRx.forms;
        if(!Wnt_Finish(&p->customRx,id,gen,now)) {
            failed("Custom character verification failed. Room stopped.");return 1;
        }
        bytes=Wnt_Take(&p->customRx,&size);
        if(host) {
            unsigned char reply[16];int ok=custom_admit((int)(p-peers)+1,gen,
                p->customFighter,p->customBase,p->customRevision,bytes,size,forms,&p->customSelection,p->customCpu);
            if(!ok)WarriorNetPack_Free(bytes);
            put32(reply,id);put32(reply+4,gen);put32(reply+8,ok);
            put32(reply+12,ok?(unsigned)config.chara[p->customFighter]:0);send_packet(p,TN_CUSTOM_RESULT,reply,sizeof(reply));
        }else {
            if(!custom_install(id-1,bytes,size,forms)) {
                WarriorNetPack_Free(bytes);failed("Could not load temporary custom character files.");return 1;
            }
            p->customRequest=0;p->customRequestDeadline=0;
            if(Wnc_CanReady(&customCatalog) && !custom_activate())
                failed("Could not register the downloaded custom characters.");
        }
        fprintf(stderr,"custom assets: received verified package bytes=%u forms=%u\n",size,forms);return 1;
    }
    if(type==TN_CUSTOM_RESULT && !host && n==16 && customOffer.state &&
       id==customOffer.id && gen==customSelfGeneration) {
        int ok=get32(data+8)!=0;
        if(ok && get32(data+12)>=PORT_WARRIOR_SESSION_FIRST && get32(data+12)<PORT_WARRIOR_SESSION_FIRST+PORT_WARRIOR_SESSION_MAX)
            customLocalRoom[customOffer.local]=(int)get32(data+12);
        custom_offer_clear();
        say(ok?"Custom character shared. Waiting for room verification.":"The room changed before sharing finished. Select your character again.");return 1;
    }
    failed("Invalid custom character transfer message. Room stopped.");return 1;
}
static void custom_assets_tick(void) {
    unsigned i;Uint64 now=SDL_GetTicks();
    for(i=0;i<PEERS;i++)if(peers[i].hello && peers[i].fd!=TN_BAD) {
        Peer *p=&peers[i];unsigned step;
        if(Wnt_Expire(&p->customRx,now)){failed("Custom character transfer timed out.");return;}
        if(p->customRequest && !p->customRx.state && now>=p->customRequestDeadline) {
            failed("The host did not start the custom character download.");return;
        }
        for(step=0;step<4 && p->customTx.state;step++) {
            int result=Wnt_SendStep(&p->customTx,custom_emit,p,now);
            if(result<0){failed("Custom character sharing stalled.");return;}
            if(!result)break;
            if(!host && customOffer.state==2)customOffer.deadline=now+WNT_TIMEOUT_MS;
        }
    }
    if(!host && customOffer.state && now>=customOffer.deadline){failed("The host did not finish the custom character request.");return;}
    if(!host && (customSync || config.practice || phase>=3) && customCatalogReceived && !customWorldLive && !peers[0].customRequest && !peers[0].customRx.state) {
        for(i=0;i<customCatalog.count;i++)if(!customBlobs[i].data) {
            unsigned char request[8];peers[0].customRequest=i+1;
            peers[0].customRequestDeadline=now+WNT_TIMEOUT_MS;
            put32(request,i+1);put32(request+4,customSelfGeneration);send_packet(&peers[0],TN_CUSTOM_GET,request,sizeof(request));break;
        }
    }
    custom_sync_progress();
    /* Private test entry, deliberately not a release UI. */
    if(!customProbeDone && phase==2 && !config.practice && role>=0 && role<TN_PLAYERS &&
       (host || customCatalogReceived) && getenv("BT3_TAG_CUSTOM_TEST_INDEX")) {
        for(i=0;i<(unsigned)(config.size[0]+config.size[1]);i++)if(config.owner[i]==role) {
            customProbeDone=1;custom_choose(atoi(getenv("BT3_TAG_CUSTOM_TEST_INDEX")),i,NULL,-1);break;
        }
    }
}
