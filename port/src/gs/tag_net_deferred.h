/* Lightweight room choices precede heavy generated-file sharing. Included by
   tag_net_assets.h; only the host grants one package upload at a time. */
static int custom_share(int,unsigned,const TlLoadout *,int);
static void custom_pick_wire(unsigned fighter,unsigned char wire[248]) {
    const CustomPick *pick=&customPicks[fighter];unsigned k;
    memset(wire,0,248);put32(wire,(unsigned)config.revision);put32(wire+4,fighter);
    put32(wire+8,(unsigned)pick->owner);put32(wire+12,(unsigned)pick->selection.costume);
    put32(wire+16,(unsigned)pick->cpu);put32(wire+20,(unsigned)pick->selection.chara);
    for(k=0;k<8;k++)put32(wire+24+k*4,(unsigned)pick->selection.items[k]);
    if(pick->set)snprintf((char *)wire+56,192,"%s",pick->name);
}
static void custom_pick_snapshot(Peer *p) {
    unsigned i;unsigned char wire[248];
    for(i=0;i<TN_FIGHTERS;i++)if(customPicks[i].set) {
        custom_pick_wire(i,wire);send_packet(p,TN_CUSTOM_PICK,wire,sizeof(wire));
    }
}
static void custom_pick_clear(unsigned fighter) {
    unsigned char wire[248];
    if(fighter>=TN_FIGHTERS)return;
    memset(&customPicks[fighter],0,sizeof(customPicks[fighter]));customPicks[fighter].local=-1;
    if(host){custom_pick_wire(fighter,wire);broadcast(TN_CUSTOM_PICK,wire,sizeof(wire),NULL);}
    if(customGrant==(int)fighter)customGrant=-1;
}
static void custom_pick_config(const TagNetConfig *next) {
    int i;
    for(i=0;i<TN_FIGHTERS;i++)if(customPicks[i].set &&
        (i>=next->size[0]+next->size[1] || next->owner[i]!=config.owner[i] ||
         next->chara[i]!=config.chara[i] || next->costume[i]!=config.costume[i] ||
         memcmp(next->items[i],config.items[i],sizeof(config.items[i])))) {
        memset(&customPicks[i],0,sizeof(customPicks[i]));customPicks[i].local=-1;
    }
}
static int custom_pick_apply(int owner,const unsigned char *data,unsigned n,int local) {
    unsigned fighter,k;CustomPick pick;TagNetConfig next=config;unsigned char wire[248];
    if(n!=248 || customSync || get32(data)!=(unsigned)config.revision)return 0;
    fighter=get32(data+4);
    if(!custom_fighter_owned(owner,fighter,(unsigned)config.revision))return 0;
    memset(&pick,0,sizeof(pick));pick.owner=owner;pick.local=local;
    pick.selection.costume=(int)get32(data+12);pick.cpu=(int)get32(data+16);
    pick.selection.chara=(int)get32(data+20);
    if(pick.selection.costume<0 || pick.cpu< -1 || pick.cpu>4 || data[247])return 0;
    for(k=0;k<8;k++){pick.selection.items[k]=(int)get32(data+24+k*4);if(pick.selection.items[k]<0 || pick.selection.items[k]>350)return 0;}
    for(k=56;k<247 && data[k];k++)if(data[k]<32 || data[k]==127)return 0;
    snprintf(pick.name,sizeof(pick.name),"%s",(const char *)data+56);pick.set=pick.name[0]!=0;
    if(pick.set && pick.selection.costume>3)return 0;
    if(!pick.set) {
        next.chara[fighter]=pick.selection.chara;next.costume[fighter]=pick.selection.costume;
        memcpy(next.items[fighter],pick.selection.items,sizeof(pick.selection.items));
        if(next.owner[fighter]<0)next.cpuLevel[fighter]=pick.cpu;
        if(!valid(&next))return 0;
        custom_pick_clear(fighter);apply_edit(&next);return 1;
    }
    customPicks[fighter]=pick;
    // Publish a new selection revision even while its playable slot retains the
    // last verified fighter. Old READY packets cannot authorize a new choice.
    apply_edit(&next);custom_pick_wire(fighter,wire);broadcast(TN_CUSTOM_PICK,wire,sizeof(wire),NULL);
    say("Character selected. Files will sync after everyone readies up.");return 1;
}
static int custom_choose(int local,unsigned fighter,const TlLoadout *selection,int cpuLevel) {
    unsigned char wire[248]={0};PortWarriorLocal description;unsigned k;int owner=host?0:role;TlLoadout fallback={0};
    if(!selection)selection=&fallback;
    if(!selection || customSync || !custom_fighter_owned(owner,fighter,(unsigned)config.revision))return 0;
    if(local!=-2 && (local<0 || local>=PortWarriors_LocalCount() || !PortWarriors_LocalCopy(local,&description)))return 0;
    put32(wire,(unsigned)config.revision);put32(wire+4,fighter);put32(wire+8,(unsigned)owner);
    put32(wire+12,(unsigned)selection->costume);put32(wire+16,(unsigned)cpuLevel);put32(wire+20,(unsigned)selection->chara);
    for(k=0;k<8;k++)put32(wire+24+k*4,(unsigned)selection->items[k]);
    if(local!=-2)snprintf((char *)wire+56,192,"%s",description.name);
    if(host)return custom_pick_apply(owner,wire,sizeof(wire),local);
    if(!peers[0].hello || !customCatalogReceived)return 0;
    customPicks[fighter].local=local;
    send_packet(&peers[0],TN_CUSTOM_PICK,wire,sizeof(wire));return 1;
}
static int custom_pick_packet(Peer *p,int type,const unsigned char *data,unsigned n) {
    if(type==TN_CUSTOM_PICK) {
        unsigned fighter,k;CustomPick *pick;int local;
        if(host){custom_pick_apply(p->role,data,n,-1);return 1;}
        if(n!=248 || phase!=2 || get32(data)!=(unsigned)config.revision || (fighter=get32(data+4))>=TN_FIGHTERS || data[247])return 1;
        for(k=56;k<247 && data[k];k++)if(data[k]<32 || data[k]==127)return 1;
        pick=&customPicks[fighter];local=pick->local;
        memset(pick,0,sizeof(*pick));pick->owner=(int)get32(data+8);
        pick->local=pick->owner==role?local:-1;pick->selection.costume=(int)get32(data+12);
        pick->cpu=(int)get32(data+16);pick->selection.chara=(int)get32(data+20);
        for(k=0;k<8;k++)pick->selection.items[k]=(int)get32(data+24+k*4);
        snprintf(pick->name,sizeof(pick->name),"%s",(const char *)data+56);pick->set=pick->name[0]!=0;
        return 1;
    }
    if(type==TN_CUSTOM_SYNC) {
        unsigned mode,fighter;
        if(host || n!=16 || phase!=2 || config.practice || get32(data+4)!=(unsigned)config.revision)return 1;
        mode=get32(data);fighter=get32(data+8);
        if(mode!=1 && mode!=2){failed("Invalid character sync request.");return 1;}
        customSync=1;
        if(mode==2 && get32(data+12)==(unsigned)role) {
            CustomPick *pick;
            if(fighter>=TN_FIGHTERS || !customPicks[fighter].set || customPicks[fighter].local<0) {
                failed("The selected local character is no longer available.");return 1;
            }
            pick=&customPicks[fighter];
            if(!custom_share(pick->local,fighter,&pick->selection,pick->cpu))failed("Could not prepare the selected character for the match.");
        }
        return 1;
    }
    return 0;
}
static int custom_sync_step(void) {
    unsigned fighter;unsigned char wire[16];CustomPick pick;
    for(fighter=0;fighter<TN_FIGHTERS;fighter++)if(customPicks[fighter].set)break;
    if(fighter==TN_FIGHTERS)return 0;
    if(!customSync) {
        customSync=1;customGrant=-1;
        put32(wire,1);put32(wire+4,(unsigned)config.revision);put32(wire+8,~0u);put32(wire+12,~0u);
        broadcast(TN_CUSTOM_SYNC,wire,sizeof(wire),NULL);
        fprintf(stderr,"custom sync: all players ready; beginning selected-character transfers\n");
    }
    if(customGrant>=0)return 1;
    // Config publication waits for catalog verification. Do not grant a new
    // uploader a revision it has not received yet.
    if(!custom_local_ready() || !Wnc_CanStart(&customCatalog))return 1;
    customGrant=(int)fighter;pick=customPicks[fighter];
    if(pick.owner==0) {
        if(!custom_share(pick.local,fighter,&pick.selection,pick.cpu))failed("Could not prepare the host's selected character for the match.");
    }else {
        put32(wire,2);put32(wire+4,(unsigned)config.revision);put32(wire+8,fighter);put32(wire+12,(unsigned)pick.owner);
        broadcast(TN_CUSTOM_SYNC,wire,sizeof(wire),NULL);
    }
    return 1;
}
static void custom_sync_progress(void) {
    unsigned i;const char *action="Verifying custom characters";unsigned percent=0;
    if(!customSync || phase!=2)return;
    for(i=0;i<PEERS;i++) {
        Peer *p=&peers[i];
        if(p->customRx.state && p->customRx.size) {
            action="Downloading custom character";percent=(unsigned)((uint64_t)p->customRx.received*100/p->customRx.size);break;
        }
        if(p->customTx.state && p->customTx.size) {
            action="Sending custom character";percent=(unsigned)((uint64_t)p->customTx.offset*100/p->customTx.size);break;
        }
    }
    if(percent>99)percent=99;
    if(i<PEERS)snprintf(message,sizeof(message),"%s... %u%%",action,percent);
    else snprintf(message,sizeof(message),"Preparing and verifying custom characters...");
}
