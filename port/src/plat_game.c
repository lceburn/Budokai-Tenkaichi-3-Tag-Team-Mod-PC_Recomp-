/* Native BT4 B14 REV2 resource profile. No game assets are embedded here. */
#include "plat_game.h"
#include "plat_warriors.h"
#include "warrior_limits.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

extern const char *Port_FileRoot(void);
extern void *Port_LowAlloc(size_t size);
static int initialized, bt4, region, spanish;
static unsigned short costumes[250];
static int models[251];
static void *metadata[250][4];
static char names[250][256];
static signed char damageModels[250][16];
static signed char voiceBanks[2][250];
static int damaged_available(int c,int outfit);
static unsigned char extraCustom[250-97][56];
static int customLoaded;
static int onlineCustom;

static void fail(const char *message) {
    fprintf(stderr, "native disc: %s\n", message);
    exit(2);
}
static FILE *open_data(const char *relative) {
    char path[1600];
    snprintf(path, sizeof(path), "%s/%s", Port_FileRoot(), relative);
    return fopen(path, "rb");
}
static uint32_t u32(const unsigned char *p) {
    return p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static void init(void) {
    FILE *f;
    char marker[8192];
    unsigned char table[32768];
    size_t size;
    unsigned offset;
    int i;
    if (initialized) return;
    initialized = 1;
    f = open_data("native-disc.json");
    if (!f) return;
    size = fread(marker, 1, sizeof(marker)-1, f); fclose(f); marker[size] = 0;
    if (strstr(marker, "\"bt3\"") && strstr(marker, "\"bt3-us-canonical-v1\"")) {
        if(strstr(marker,"\"eu\"")) region=1;
        else if(strstr(marker,"\"jp\"")) region=2;
        else if(!strstr(marker,"\"us\"")) fail("Unknown BT3 disc region.");
        fprintf(stderr,"native disc: BT3 %s, canonical resources enabled\n",region==1?"Europe":region==2?"Japan":"USA");
        return;
    }
    if (!strstr(marker, "\"bt4\"") || !strstr(marker, "\"b14-rev2\""))
        fail("Unsupported disc profile. Use the native disc importer.");
    spanish = strstr(marker,"\"es\"") != NULL;
    f = open_data("pzs4us1/00003.bin");
    if (!f) fail("BT4 character table is missing.");
    size = fread(table, 1, sizeof(table), f); fclose(f);
    if (size < 8) fail("BT4 character table is incomplete.");
    offset = u32(table+4) & ~3u;
    if (offset > size || size-offset < 250*60)
        fail("BT4 character table is incomplete.");
    models[0] = 5650;
    for (i=0; i<250; ++i) {
        unsigned c = table[offset+i*60+40] | table[offset+i*60+41]<<8;
        if (c > 16) fail("Unsupported BT4 costume table.");
        costumes[i] = c;
        models[i+1] = models[i] + c*2;
    }
    bt4 = 1;
    f=open_data("native-roster.tsv");
    if(f) {
        char line[600];
        while(fgets(line,sizeof(line),f)) {
            char *name=strchr(line,'\t'),*form;
            int c=atoi(line);
            if(!name || c<0 || c>=250) continue;
            form=strchr(++name,'\t');
            if(!form) continue;
            *form++=0;form[strcspn(form,"\r\n")]=0;
            snprintf(names[c],sizeof(names[c]),"%s%s%s",name,*form?" ":"",form);
        }
        fclose(f);
    }
    fprintf(stderr, "native disc: BT4 B14 REV2, separate resources enabled\n");
}
int PortGame_IsBT4(void) { init(); return bt4; }
int PortGame_Region(void) { init(); return region; }
const char *PortGame_DefaultLanguage(void) { init(); return spanish?"es":"en"; }
int PortGame_VolumeCount(int volume) {
    static const int bt3Counts[]={1,3399,65201};
    static const int bt4Counts[]={50503,3400,32775,30000};
    init();
    if(volume<0 || volume>=(bt4?4:3))return 0;
    return bt4?bt4Counts[volume]-(spanish && volume==2):bt3Counts[volume];
}
static int warrior_count(void) {
    int n;
    if(!PortGame_IsBT4() || !PortGame_WarriorModelsEnabled() || onlineCustom)return 0;
    n=PortWarriors_Count();
    if(PortWarriors_Error())fail(PortWarriors_Error());
    if(n>PORT_WARRIOR_PLAYABLE_MAX)fail("This import prototype supports at most four playable custom forms.");
    return n;
}
int PortGame_IsCustomCharacter(int c) {
    return c>=PORT_WARRIOR_FIRST_CHARACTER && c<PORT_WARRIOR_FIRST_CHARACTER+warrior_count();
}
int PortGame_CharacterCount(void) { return PortGame_IsBT4() ? 250+warrior_count() : 161; }
static int warrior_named_donor(int c,int voice) {
    const char *name;int i;
    name=voice?PortWarriors_VoiceName(c-250):PortWarriors_TemplateName(c-250);
    if(name)for(i=0;i<250;i++)if(names[i][0] && !strcmp(name,names[i]))return i;
    fail("A custom warrior's named donor is absent from this disc roster.");return -1;
}
int PortGame_TableCharacter(int c) {
    return PortGame_IsCustomCharacter(c)?warrior_named_donor(c,0):c;
}
int PortGame_UsesBT4Rules(int c) {
    return PortGame_IsBT4() && (!PortGame_IsCustomCharacter(c) || PortWarriors_UsesBT4Rules(c-250));
}
void *PortGame_GaugeTable(void *original) {
    static unsigned char *table;
    int n=warrior_count(),i;
    if(!n)return original;
    if(!table)table=Port_LowAlloc((250+PORT_WARRIOR_PLAYABLE_MAX)*16);
    if(!table)fail("Could not allocate custom warrior gauge table.");
    memcpy(table,original,250*16);
    for(i=0;i<n;i++)memcpy(table+(250+i)*16,(unsigned char *)original+PortGame_TableCharacter(250+i)*16,16);
    return table;
}
static int warrior_file(int c,int role) {
    return PORT_WARRIOR_FILE_BASE+(c-250)*PORT_WARRIOR_FILE_STRIDE+role;
}
int PortGame_CheckPortraitHeader(const void *header,int headerSize,int fileSize) {
    const unsigned char *p=header;
    if(!p || headerSize<8 || fileSize<8 || fileSize>0x16800)return 0;
    return u32(p)>=0x60 && u32(p)<=0x20800 && u32(p+4)<=(unsigned)fileSize-8;
}
int PortGame_WarriorPortraitValid(int c) {
    unsigned char header[8];char relative[64],path[1600];FILE *f;long size;int valid;
    if(!PortGame_IsCustomCharacter(c))return 0;
    snprintf(relative,sizeof(relative),"zwarriors/%d/%d.bin",c-250,PORT_WARRIOR_PORTRAIT);
    if(PortWarriors_Alias(relative,path,sizeof(path))!=1 || !(f=fopen(path,"rb")))return 0;
    fseek(f,0,SEEK_END);size=ftell(f);rewind(f);
    valid=fread(header,1,8,f)==8 && size<=0x16800 &&
          PortGame_CheckPortraitHeader(header,8,(int)size);
    fclose(f);return valid;
}
int PortGame_MenuPortrait(int c) {
    if(PortGame_IsCustomCharacter(c)) {
        if(PortGame_WarriorPortraitValid(c))return warrior_file(c,PORT_WARRIOR_PORTRAIT);
        c=PortGame_TableCharacter(c);
    }
    return (PortGame_IsBT4()?0x71C:0x2F9)+c;
}
int PortGame_CheckWarriorChip(const void *data,int size) {
    const unsigned char *p=data,*e;unsigned at,pixels,palette;
    uint64_t tex;
    if(!p || size!=5504 || u32(p)!=1 || u32(p+4)!=8)return 0;
    at=u32(p+4)*4;e=p+at;
    if(u32(e)>5504/4 || u32(e+4)>5504/4)return 0;
    pixels=u32(e)*4;palette=u32(e+4)*4;
    if(pixels<96 || pixels>5504 || palette<96 || palette>5504 ||
       u32(e+8)>5504-pixels || u32(e+12)>5504-palette ||
       u32(e+8)<4096+96 || u32(e+12)<1024+96)return 0;
    tex=u32(e+48)|((uint64_t)u32(e+52)<<32);
    return ((tex>>26)&15)==6 && ((tex>>30)&15)==6 && ((tex>>20)&63)==19;
}
int PortGame_WarriorChip(int c) {
    unsigned char data[5505];char relative[64],path[1600];FILE *f;size_t size;
    if(!PortGame_IsCustomCharacter(c))return -1;
    snprintf(relative,sizeof(relative),"zwarriors/%d/%d.bin",c-250,PORT_WARRIOR_CHIP);
    if(PortWarriors_Alias(relative,path,sizeof(path))!=1 || !(f=fopen(path,"rb")))return -1;
    size=fread(data,1,sizeof(data),f);fclose(f);
    return PortGame_CheckWarriorChip(data,(int)size)?warrior_file(c,PORT_WARRIOR_CHIP):-1;
}
int PortGame_WarriorGrid(void *cells,int count,int capacity) {
    int (*grid)[9]=cells,groups[PORT_WARRIOR_PLAYABLE_MAX],n=warrior_count();
    int i,j,k,end=count,added=0,padded,changed;
    if(!n || !grid || count<0 || count>capacity)return count;
    for(i=0;i<n;i++)groups[i]=i;
    /* Treat supported links as undirected for selection grouping. The battle
       still exposes only the explicitly declared, directed transformations. */
    do {
        changed=0;
        for(i=0;i<n;i++)for(j=0;j<4;j++) {
            int target=PortGame_WarriorFormTarget(250+i,j)-250,low,high;
            if(target<0 || target>=n || groups[target]==groups[i])continue;
            low=groups[i]<groups[target]?groups[i]:groups[target];
            high=groups[i]>groups[target]?groups[i]:groups[target];
            for(k=0;k<n;k++)if(groups[k]==high)groups[k]=low;
            changed=1;
        }
    }while(changed);
    for(i=0;i<n;i++)if(groups[i]==i)added++;
    while(end>0 && grid[end-1][0]==164)end--; /* native filler marker */
    padded=(end+added+6)/7*7;
    if(padded>capacity)return count; /* No partial writes on a full roster. */
    for(i=0;i<n;i++)if(groups[i]==i) {
        int forms=0;memset(grid[end],0,sizeof(grid[end]));grid[end][0]=250+i;
        for(j=0;j<n;j++)if(groups[j]==i)grid[end][2+forms++]=250+j;
        grid[end][1]=forms>1?forms:0;end++;
    }
    while(end<padded){memset(grid[end],0,sizeof(grid[end]));grid[end++][0]=164;}
    return padded;
}
int PortGame_IsCharacter(int c) {
    if(PortGame_IsCustomCharacter(c))return 1;
    return PortGame_IsBT4() ? c>=0 && c<250 && (c<161 || c>164) && costumes[c]!=0 : c>=0 && c<161;
}
/* Network loadouts are limited to disc fighters even outside a scoped menu. */
int PortGame_IsOnlineCharacter(int c) {
    return c>=0 && c<(PortGame_IsBT4()?250:161) && PortGame_IsCharacter(c);
}
int PortGame_StageCount(void) { return PortGame_IsBT4()?99:36; }
const char *PortGame_CharacterName(int c) {
    if(PortGame_IsCustomCharacter(c))return PortWarriors_Name(c-250);
    return PortGame_IsBT4() && c>=0 && c<250 && names[c][0]?names[c]:NULL;
}
const char *PortGame_WarriorAttackName(int c,int slot) {
    return PortGame_IsCustomCharacter(c)?PortWarriors_AttackName(c-250,slot):NULL;
}
int PortGame_WarriorAuraColour(int c) {
    return PortGame_IsCustomCharacter(c)?PortWarriors_AuraColour(c-250):-1;
}
static int musicIds[256],musicCount=-1;
int PortGame_MusicCount(void) {
    if(!PortGame_IsBT4())return 24;
    if(musicCount<0) {
        extern int MenuLabels_MusicIds(const char *,int *,int);
        musicCount=MenuLabels_MusicIds(Port_FileRoot(),musicIds,256);
        if(!musicCount)fail("The battle music catalogue is missing or damaged.");
    }
    return musicCount;
}
int PortGame_MusicAt(int index) {
    int count=PortGame_MusicCount();
    return index>=0 && index<count?(PortGame_IsBT4()?musicIds[index]:index):24;
}
int PortGame_HasMusic(int track) {
    if(track==24)return 1;
    for(int i=0;i<PortGame_MusicCount();i++)if(PortGame_MusicAt(i)==track)return 1;
    return 0;
}
int PortGame_MusicFile(int track) { return (PortGame_IsBT4()?2070:0x10B16)+track; }
int PortGame_StreamFile(int id) {
    if(!PortGame_IsBT4()) return id;
    if(id>=0x10BC7 && id<=0x10BCA) return 0x14E + id - 0x10BC7;
    if(id>=0x10BDB && id<=0x10BE1) return 0x158 + id - 0x10BDB;
    return id;
}
int PortGame_Costumes(int c) {
    if(PortGame_IsCustomCharacter(c))return PortWarriors_Costumes(c-250);
    if(PortGame_IsBT4())return c>=0 && c<250 ? costumes[c] : 0;
    if(c>=0 && c<161) {
        static int loaded;
        if(!loaded) {
            unsigned char table[32768];unsigned offset;size_t size;int i;
            FILE *f=open_data("pzs3us1/00003.bin");
            if(!f)return 0;
            size=fread(table,1,sizeof(table),f);fclose(f);
            if(size<8)return 0;
            offset=u32(table+4)&~3u;
            if(offset>size || size-offset<161*60)return 0;
            for(i=0;i<161;i++)costumes[i]=table[offset+i*60+10] | table[offset+i*60+11]<<8;
            loaded=1;
        }
        return costumes[c];
    }
    return 0;
}
int PortGame_Model(int c, int outfit, int damaged) {
    if(PortGame_IsCustomCharacter(c)) {
        if(outfit<0 || outfit>=PortWarriors_Costumes(c-250))fail("Invalid custom warrior costume.");
        return warrior_file(c,PortWarriors_ModelRole(outfit,damaged));
    }
    if (!PortGame_IsBT4()) return 1424+c*10+outfit+(damaged?4:0);
    if (c<0 || c>=250 || !costumes[c] || outfit<0 || outfit>=costumes[c]) {
        fprintf(stderr,"native disc: model request fighter=%d costume=%d damaged=%d\n",c,outfit,damaged);
        fail("Invalid fighter or costume requested.");
    }
    if(damaged) {
        /* Some B14 costumes have no damaged mesh. Preserve the same intact
           costume, rather than bind the empty placeholder as a model. */
        if(!damaged_available(c,outfit)) damaged=0;
    }
    return models[c]+outfit+(damaged?costumes[c]:0);
}
static int damaged_available(int c,int outfit) {
    if(!damageModels[c][outfit]) {
        char path[128];unsigned char header[8];FILE *f;int id=models[c]+outfit+costumes[c];
        snprintf(path,sizeof(path),"pzs4us%d/%05d.bin",PortGame_Partition(id),PortGame_FileIndex(id));
        f=open_data(path);
        if(!f) fail("Missing BT4 damaged costume resource.");
        damageModels[c][outfit]=fread(header,1,8,f)==8 && u32(header)>0 ? 1 : -1;
        fclose(f);
    }
    return damageModels[c][outfit]>0;
}
int PortGame_ModelCharacter(int id) {
    int i;
    if(id>=PORT_WARRIOR_FILE_BASE && id<PORT_WARRIOR_FILE_BASE+warrior_count()*PORT_WARRIOR_FILE_STRIDE)
        return 250+(id-PORT_WARRIOR_FILE_BASE)/PORT_WARRIOR_FILE_STRIDE;
    if (!PortGame_IsBT4()) return (id-1424)/10;
    for (i=0; i<250; ++i) if (id>=models[i] && id<models[i+1]) return i;
    return -1;
}
/* B14 Majin Time Patroller outfit 2 has an accidentally relocated prefix in
   both body-volume files. Its intact counterpart elsewhere on the same disc
   confirms every byte of this recovery, including the final corner's Y.
   Match the complete original section before touching the in-memory copy. */
void PortGame_RepairModelBody(void *body,int size,int model) {
    unsigned char *p=body;unsigned hash=2166136261u,*words=body;int i;
    if(!PortGame_IsBT4() || !body || size!=9792 || (model!=7205 && model!=7207))return;
    for(i=0;i<size;i++)hash=(hash^p[i])*16777619u;
    if(hash!=0x855116afu)return;
    for(i=0;i<0x314/4;i++)words[i]-=0x1e450u;
    words[0x314/4]=words[0x2b4/4]^0x80000000u;
}
int PortGame_Animation(int c) { return PortGame_IsCustomCharacter(c)?warrior_file(c,PORT_WARRIOR_ANIMATION):PortGame_IsBT4()?5150+c:1432+c*10; }
int PortGame_Combat(int c) { return PortGame_IsCustomCharacter(c)?warrior_file(c,PORT_WARRIOR_EFFECTS):PortGame_IsBT4()?5400+c:1433+c*10; }
static int voice_bank_available(int c,int jp) {
    if(!voiceBanks[jp][c]) {
        unsigned char header[20];char path[128];FILE *f;long size;unsigned i,prev;
        int good=0,id=c+(jp?0x1D10C:0x1D012);
        snprintf(path,sizeof(path),"pzs4us%d/%05d.bin",PortGame_Partition(id),PortGame_FileIndex(id));
        f=open_data(path);
        if(f) {
            fseek(f,0,SEEK_END);size=ftell(f);rewind(f);
            good=fread(header,1,20,f)==20 && u32(header)==3;
            prev=20;
            for(i=4;good && i<20;i+=4) {
                unsigned off=u32(header+i);
                if(off<prev || off>(unsigned)size) good=0;
                prev=off;
            }
            fclose(f);
        }
        voiceBanks[jp][c]=good?1:-1;
    }
    return voiceBanks[jp][c]>0;
}
int PortGame_Voice(int c,int jp) {
    if(PortGame_IsCustomCharacter(c))c=warrior_named_donor(c,1);
    if(!PortGame_IsBT4()) return c+(jp?3195:3034);
    if(c<0 || c>=250) fail("Invalid BT4 voice character.");
    jp=jp!=0;
    /* B14's Japanese Time Patroller bank is an invalid placeholder. Its
       English bank still contains the fighter's own combat samples. */
    if(!voice_bank_available(c,jp)) {
        if(!voice_bank_available(c,!jp)) fail("Missing BT4 fighter combat sounds.");
        jp=!jp;
    }
    return c+(jp?0x1D10C:0x1D012);
}
int PortGame_VoiceLine(int c,int line,int jp) {
    if(PortGame_IsCustomCharacter(c))c=warrior_named_donor(c,1);
    return (PortGame_IsBT4()?(jp?0x16E6A:0x10CC2):(jp?0xCC32:0x8D4E))+100*c+line;
}
int PortGame_Member(int c,int side) {
    if(PortGame_IsCustomCharacter(c))return warrior_file(c,PORT_WARRIOR_MEMBER_1+(side!=0));
    return (PortGame_IsBT4()?0xD48:8)+c*2+side;
}
int PortGame_Partition(int id) {
    if(id>=PORT_WARRIOR_FILE_BASE)return 7;
    if (PortGame_IsBT4()) return id>=0x20000?3:id>=0x10CC0?0:id>=0xD48?2:id>0?1:0;
    return id<=0?0:id<0xD48?1:2;
}
int PortGame_FileIndex(int id) {
    if(id>=PORT_WARRIOR_FILE_BASE)return id-PORT_WARRIOR_FILE_BASE;
    if (PortGame_IsBT4()) return id>=0x20000?id-0x20000:id>=0x10CC0?id-0x10CC0:id>=0xD48?id-0xD48:id>0?id-1:id;
    return id<=0?id:id<0xD48?id-1:id-0xD48;
}
/* These tables are immutable apart from the game's idempotent relocation of
   the first metadata section. Keep them alive across model changes and menus. */
int PortGame_WarriorModelsEnabled(void) {
    const char *enabled = getenv("BT3_WARRIOR_MODELS");
    return enabled && !strcmp(enabled,"1");
}
int PortGame_ModelCapacity(void) {
    return PortGame_WarriorModelsEnabled()?PORT_WARRIOR_MODEL_CAPACITY:0xE6800;
}
int PortGame_WarriorFormTarget(int chara,int slot) {
    int index=chara-PORT_WARRIOR_FIRST_CHARACTER,target;
    if(!PortGame_IsCustomCharacter(chara) || slot<0 || slot>=4)return 0xFF;
    target=PortWarriors_FormTarget(index,slot);
    /* First supported manifest family: cost, standard sequence, basic change,
       no prop override, preserve damage. Other families remain unavailable. */
    if(target<0 || target==index || !PortGame_IsCustomCharacter(250+target) ||
       PortWarriors_FormField(index,slot,0)<0 || PortWarriors_FormField(index,slot,0)>5 ||
       PortWarriors_FormField(index,slot,1)!=0 || PortWarriors_FormField(index,slot,2)!=2 ||
       PortWarriors_FormField(index,slot,3)!=-1 || PortWarriors_FormField(index,slot,4)!=0)return 0xFF;
    return 250+target;
}
int PortGame_WarriorFormField(int chara,int slot,int field) {
    if(PortGame_WarriorFormTarget(chara,slot)==0xFF || field<0 || field>=5)return -1;
    return PortWarriors_FormField(chara-250,slot,field);
}
int PortGame_UseEmbeddedModel(const void *data, int size) {
    const unsigned char *p = data;
    unsigned count, i, offset, tableEnd;
    /* This switch is for the isolated import prototype. Normal BT4 models
       continue to use their disc's separate metadata without inspecting them. */
    if (!PortGame_WarriorModelsEnabled() || !p || size<12) return 0;
    count=u32(p);
    /* The generated samples carry the BT3 model's 252-entry layout, including
       parameters, AI and both lip tables. Do not infer it from a filename. */
    if((count!=252 && count!=253) || (unsigned)size<(count+2)*4) return 0;
    tableEnd=(count+2)*4;
    for(i=1;i<=count+1;i++) {
        offset=u32(p+i*4)&~3u;
        if(offset && (offset<tableEnd || offset>(unsigned)size)) return 0;
    }
    if(count==253) {
        unsigned start=u32(p+253*4)&~3u,end=u32(p+254*4)&~3u;
        if(start<tableEnd || end<start || end-start<16 ||
           (memcmp(p+start,"BT3WARRI",8) && memcmp(p+start,"BT4WARRI",8)))return 0;
    }
    /* An empty parameter record is not a self-contained model. All readers
       of the first parameter block need at least the original 0x100 bytes. */
    offset=u32(p+4)&~3u;
    if(!offset || offset==(u32(p+8)&~3u) || (unsigned)size-offset<0x100) return 0;
    for(i=2;i<=count+1;i++) {
        unsigned next=u32(p+i*4)&~3u;
        if(next>offset && next-offset<0x100) return 0;
    }
    return 1;
}

int PortGame_EmbeddedBT4(const void *data,int size) {
    const unsigned char *p=data;
    return PortGame_UseEmbeddedModel(data,size) && u32(p)==253 &&
        !memcmp(p+(u32(p+253*4)&~3u),"BT4WARRI",8);
}
void *PortGame_Metadata(int c,int kind) {
    char path[128];
    FILE *f;
    long size;
    void *data;
    int id;
    if (!PortGame_IsBT4() || c<0 || c>=250 || kind<0 || kind>=4) return NULL;
    if (metadata[c][kind]) return metadata[c][kind];
    id = 4150+c+kind*250;
    snprintf(path,sizeof(path),"pzs4us%d/%05d.bin",PortGame_Partition(id),PortGame_FileIndex(id));
    f=open_data(path);
    if (!f) fail("Missing BT4 fighter metadata.");
    fseek(f,0,SEEK_END); size=ftell(f); rewind(f);
    if (size<8 || size>2*1024*1024) fail("Invalid BT4 fighter metadata size.");
    data=Port_LowAlloc(size);
    if (!data || fread(data,1,size,f)!=(size_t)size) fail("Could not read BT4 fighter metadata.");
    fclose(f);
    metadata[c][kind]=data;
    return data;
}

static void custom_path(char *path,size_t capacity,const char *suffix) {
    const char *root=getenv("BT3_SAVES");
    snprintf(path,capacity,"%s/native-bt4-custom%s",root?root:"saves",suffix);
}
void *PortGame_CustomSlot(void *original,int slot) {
    static unsigned char warriorCustom[PORT_WARRIOR_PLAYABLE_MAX][56];
    if(PortGame_IsCustomCharacter(slot))return warriorCustom[slot-250];
    if(slot<0 || slot>=(PortGame_IsBT4()?250:97)) fail("Invalid equipment slot.");
    if(slot<97) return (unsigned char *)original+slot*56;
    if(!customLoaded) {
        char path[1600],header[8];FILE *f;
        custom_path(path,sizeof(path),".bin");f=fopen(path,"rb");
        if(f) {
            if(fread(header,1,8,f)!=8 || memcmp(header,"BT4CUST1",8) ||
               fread(extraCustom,1,sizeof(extraCustom),f)!=sizeof(extraCustom) || fgetc(f)!=EOF)
                fail("Invalid BT4 equipment save; restore a known-good copy before continuing.");
            fclose(f);
        }
        customLoaded=1;
    }
    return extraCustom[slot-97];
}
static unsigned char offlineCustom[sizeof(extraCustom)];
static int offlineCustomLoaded;
void PortGame_OnlineCustom(int enter) {
    if(!PortGame_IsBT4())return;
    if(enter && !onlineCustom) {
        memcpy(offlineCustom,extraCustom,sizeof(extraCustom));offlineCustomLoaded=customLoaded;
        memset(extraCustom,0,sizeof(extraCustom));customLoaded=1;onlineCustom=1;
    } else if(!enter && onlineCustom) {
        memcpy(extraCustom,offlineCustom,sizeof(extraCustom));customLoaded=offlineCustomLoaded;onlineCustom=0;
    }
}
int PortGame_SaveCustom(void) {
    char path[1600],temporary[1600];FILE *f;int good;
    if(!PortGame_IsBT4() || !customLoaded || onlineCustom) return 1;
    custom_path(path,sizeof(path),".bin");custom_path(temporary,sizeof(temporary),".tmp");
    f=fopen(temporary,"wb");if(!f) return 0;
    good=fwrite("BT4CUST1",1,8,f)==8 && fwrite(extraCustom,1,sizeof(extraCustom),f)==sizeof(extraCustom);
    if(fclose(f)) good=0;
    if(!good) return 0;
#ifdef _WIN32
    return MoveFileExA(temporary,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
    return rename(temporary,path)==0;
#endif
}

/* BT4 has motions larger than the original 48 KiB work layer. Keep the game's
   object layout intact; give its decoded layers separate, reusable native
   storage. Reset keys when the object table is recreated, not during a form
   change: both animation layers must survive an overlapping transformation. */
static struct { void *key,*data; } animationBuffers[64];
static int animationBufferCount;
void PortGame_ResetAnimations(void) { animationBufferCount=0; }
int PortGame_AnimationCapacity(void) { return PortGame_IsBT4()?0x10000:0xC000; }
void *PortGame_AnimationBuffer(void *original) {
    int i;
    if(!PortGame_IsBT4()) return original;
    for(i=0;i<animationBufferCount;i++) if(animationBuffers[i].key==original)
        return animationBuffers[i].data;
    if(i>=64) fail("Too many native animation layers.");
    if(!animationBuffers[i].data) animationBuffers[i].data=Port_LowAlloc(0x10000);
    if(!animationBuffers[i].data) fail("Could not allocate a native animation layer.");
    animationBuffers[i].key=original;animationBufferCount++;
    return animationBuffers[i].data;
}

/* Some B14 motions contain stale bone offsets, including Kid Gohan's tail in
   motion 38. A malformed track must not reach quaternion unpacking or retarget
   writes. Keep the affected bone at its authored rest pose; preserve every
   valid track and all timing/events. This only edits the decoded work buffer. */
int PortGame_CheckAnimation(void *data,int size) {
    unsigned char *b=(unsigned char*)data;
    unsigned i,j,bad=0,header;
    if(!b || size<148 || size>PortGame_AnimationCapacity()) return -1;
    header=148+16*b[1];
    if(header>(unsigned)size) return -1;
    for(i=0;i<71;i++) {
        unsigned at=6+2*i,off=(b[at]|(unsigned)b[at+1]<<8)*4;
        unsigned n,flags,stride,rot,bytes,invalid=0;
        if(!off) continue;
        if(off<header || off>(unsigned)size-4) invalid=1;
        else {
            flags=b[off]|(unsigned)b[off+1]<<8;
            n=b[off+2]|(unsigned)b[off+3]<<8;
            stride=(flags&1)?8:24;rot=(flags&1)?0:16;
            bytes=n*stride+((flags&1)&&n>1?n*2:0);
            if(!n || bytes>(unsigned)size-off-4) invalid=1;
            else for(j=0;j<n;j++) {
                const unsigned char *key=b+off+4+j*stride;
                if((key[rot+7]>>4)>3) {invalid=1;break;}
                if(!(flags&1) && ((u32(key)&0x7f800000)==0x7f800000 ||
                   (u32(key+4)&0x7f800000)==0x7f800000 ||
                   (u32(key+8)&0x7f800000)==0x7f800000)) {invalid=1;break;}
            }
        }
        if(invalid) {b[at]=b[at+1]=0;bad++;}
    }
    return (int)bad;
}

int PortGame_AdjustWarriorAnimation(const void *model,int modelSize,int motion,void *animation,int animationSize) {
    const unsigned char *m=model,*table;
    unsigned char *a=animation;
    unsigned start,end,count,i,j,seen[71],seenCount=0;
    float scale;
    if(!PortGame_UseEmbeddedModel(model,modelSize) || motion<0 || motion>=414 ||
       !a || animationSize<148 || animationSize>PortGame_AnimationCapacity()) return 0;
    i=u32(m)==253?253:26;
    start=u32(m+i*4)&~3u;end=u32(m+(i+1)*4)&~3u;
    if(!start || end<start || end-start<16) return 0;
    table=m+start;
    if(memcmp(table,"BT3WARRI",8) && memcmp(table,"BT4WARRI",8)) return 0;
    count=u32(table+12);
    if(count>(end-start-16)/8) return -1;
    memcpy(&scale,table+8,4);
    /* The extension stores a default body/motion ratio followed by
       {u16 firstMotion,u16 motionCount,f32 ratio} overrides. */
    for(i=0;i<count;i++) {
        const unsigned char *row=table+16+8*i;
        unsigned first=row[0]|(unsigned)row[1]<<8,n=row[2]|(unsigned)row[3]<<8;
        float ratio;
        memcpy(&ratio,row+4,4);
        if(!n || first>=414 || n>414-first || !(ratio>0.0f && ratio<=16.0f)) return -1;
        if((unsigned)motion>=first && (unsigned)motion-first<n) scale=ratio;
    }
    if(!(scale>0.0f && scale<=16.0f)) return -1;
    /* Validate every translation track before modifying any. Tracks may be
       shared by node IDs, so each offset must be scaled only once. */
    for(i=0;i<71;i++) {
        unsigned off=(a[6+2*i]|(unsigned)a[7+2*i]<<8)*4,n,flags;
        if(!off) continue;
        if(off<148+16u*a[1] || off>(unsigned)animationSize-4) return -1;
        flags=a[off]|(unsigned)a[off+1]<<8;n=a[off+2]|(unsigned)a[off+3]<<8;
        if(flags&1) continue;
        if(!n || n>((unsigned)animationSize-off-4)/24) return -1;
        for(j=0;j<seenCount && seen[j]!=off;j++) {}
        if(j==seenCount) seen[seenCount++]=off;
    }
    for(i=0;i<seenCount;i++) {
        unsigned off=seen[i],n=a[off+2]|(unsigned)a[off+3]<<8,k;
        for(j=0;j<n;j++) for(k=0;k<3;k++) {
            float value;
            unsigned char *p=a+off+4+j*24+k*4;
            memcpy(&value,p,4);value*=scale;memcpy(p,&value,4);
        }
    }
    return 1;
}
