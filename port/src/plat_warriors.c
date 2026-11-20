/* Experimental generated-warrior resource routing. No recipe execution,
   installation, roster slots or game-data rewriting occurs here. */
#include "plat_warriors.h"
#include "plat_game.h"
#include "warrior_limits.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

extern const char *Port_FileRoot(void);
enum { MAX_WARRIORS=32, MAX_NAME=192, MAX_RELATIVE=256 };
typedef struct WarriorFiles {
    char previewRoot[768];
    char folder[MAX_NAME];
    char name[MAX_NAME],templateName[MAX_NAME],voiceName[MAX_NAME];
    char attackName[5][32];
    int auraColour,bt4Rules;
    char formFolder[4][MAX_NAME];
    int formTarget[4],formFields[4][5];
    unsigned formSeen;
    char resource[PORT_WARRIOR_FILE_ROLES][MAX_RELATIVE];
    int costumes;
} WarriorFiles;
static WarriorFiles warriors[MAX_WARRIORS];
static int initialized, count;
static char error[256];
static WarriorFiles previewSaved;
static int previewActive,previewCount;
static int warrior_path(char *out,unsigned size,const WarriorFiles *w,const char *file) {
    int n=w->previewRoot[0]?snprintf(out,size,"%s/%s/%s",Port_FileRoot(),w->previewRoot,file):
        snprintf(out,size,"%s/mods/%s/%s",Port_FileRoot(),w->folder,file);
    return n>=0 && (unsigned)n<size;
}

static int failure(const char *message) {
    snprintf(error,sizeof(error),"%s",message);
    count=0;
    return 0;
}
static int same_name(const char *a,const char *b) {
    while(*a && *b) {
        if(tolower((unsigned char)*a++)!=tolower((unsigned char)*b++))return 0;
    }
    return *a==*b;
}
/* Components must stay inside the selected warrior folder on both hosts. */
static int safe_relative(const char *text,int nested) {
    const char *part=text,*p=text;
    if(!*text || text[0]=='/' || strchr(text,'\\') || strchr(text,':'))return 0;
    for(;;p++) {
        if((unsigned char)*p<32 && *p)return 0;
        if(*p=='/' || !*p) {
            size_t n=(size_t)(p-part);
            if(!n || (n==1 && part[0]=='.') || (n==2 && part[0]=='.' && part[1]=='.') ||
               part[n-1]=='.' || part[n-1]==' ')return 0;
            if(!*p)return 1;
            if(!nested)return 0;
            part=p+1;
        }
    }
}
/* Returns 1 for a line, 0 at EOF, -1 for a truncated/invalid line. */
static int line_read(FILE *f,char *line,size_t size) {
    size_t n;
    if(!fgets(line,(int)size,f))return ferror(f)?-1:0;
    n=strlen(line);
    if(n && line[n-1]!='\n' && !feof(f))return -1;
    while(n && (line[n-1]=='\n' || line[n-1]=='\r'))line[--n]=0;
    return 1;
}
static int ends(const char *text,const char *suffix) {
    size_t a=strlen(text),b=strlen(suffix);
    return a>=b && !strcmp(text+a-b,suffix);
}
static int role(const char *path,int *memberCount) {
    if(ends(path,"_3p_dmg.pak"))return PORT_WARRIOR_DAMAGE_3;
    if(ends(path,"_4p_dmg.pak"))return PORT_WARRIOR_DAMAGE_4;
    if(ends(path,"_3p.pak"))return PORT_WARRIOR_MODEL_3;
    if(ends(path,"_4p.pak"))return PORT_WARRIOR_MODEL_4;
    if(ends(path,"_1p_dmg.pak"))return PORT_WARRIOR_DAMAGE_1;
    if(ends(path,"_2p_dmg.pak"))return PORT_WARRIOR_DAMAGE_2;
    if(ends(path,"_1p.pak"))return PORT_WARRIOR_MODEL_1;
    if(ends(path,"_2p.pak"))return PORT_WARRIOR_MODEL_2;
    if(ends(path,"_anm.pak"))return PORT_WARRIOR_ANIMATION;
    if(ends(path,"_eff.pak"))return PORT_WARRIOR_EFFECTS;
    if(ends(path,"_img.cdbt"))return PORT_WARRIOR_PORTRAIT;
    if(!strncmp(path,"pzs3us1/",8) && ends(path,".bin") && *memberCount<2)
        return PORT_WARRIOR_MEMBER_1+(*memberCount)++;
    return -1;
}
static int identity_field(char *out,unsigned size,const char *value) {
    if(out[0] || !value[0] || strlen(value)>=size)return failure("Invalid or duplicate warrior identity field");
    snprintf(out,size,"%s",value);return 1;
}
static int load_identity(WarriorFiles *warrior) {
    char path[1024],line[1024],*value,body[MAX_NAME]={0},moves[MAX_NAME]={0};FILE *f;int result,pass,mode=0,i;
    int auraSeen=0,rulesSeen=0;warrior->auraColour=-1;
    for(i=0;i<4;i++)warrior->formTarget[i]=-1;
    for(pass=0;pass<2;pass++) {
        if(!warrior_path(path,sizeof(path),warrior,pass?"mod.txt":"warrior.txt"))return failure("Warrior identity path is too long");
        f=fopen(path,"rb");
        if(!f)return failure("Enabled warrior has no recipe or generated manifest");
        while((result=line_read(f,line,sizeof(line)))==1) {
            if(!line[0] || line[0]=='#')continue;
            value=strchr(line,'=');
            if(!value) {fclose(f);return failure("Malformed warrior identity manifest");}
            *value++=0;
            if(!pass) {
                char *out=NULL;
                if(!strcmp(line,"aura")) {
                    unsigned rgb;const char *p=value;
                    if(auraSeen++ || strlen(value)!=6){fclose(f);return failure("Invalid or duplicate custom aura colour");}
                    while(*p && isxdigit((unsigned char)*p))p++;
                    if(*p){fclose(f);return failure("Invalid custom aura colour");}
                    rgb=(unsigned)strtoul(value,NULL,16);warrior->auraColour=(int)(((rgb&255)<<16)|(rgb&0xff00)|(rgb>>16));
                }
                static const char *const attackKeys[]={"blast1_1name","blast1_2name","blast2_1name","blast2_2name","ultimatename"};
                int attack;
                for(attack=0;attack<5;attack++)if(!strcmp(line,attackKeys[attack])) {
                    const unsigned char *p=(const unsigned char *)value;
                    while(*p && *p>=32 && *p!=127)p++;
                    if(*p || !identity_field(warrior->attackName[attack],32,value)) {fclose(f);return failure("Invalid or duplicate custom attack name");}
                }
                if(!strcmp(line,"name"))out=warrior->name;
                else if(!strcmp(line,"style"))out=warrior->templateName;
                else if(!strcmp(line,"voice"))out=warrior->voiceName;
                else if(!strcmp(line,"body"))out=body;
                else if(!strcmp(line,"moves"))out=moves;
                if(out && !identity_field(out,MAX_NAME,value)) {fclose(f);return 0;}
            } else if(!strcmp(line,"rules")) {
                if(rulesSeen++ || (strcmp(value,"bt4") && strcmp(value,"bt3"))) {
                    fclose(f);return failure("Invalid or duplicate warrior rules profile");
                }
                warrior->bt4Rules=!strcmp(value,"bt4");
            } else if(!strcmp(line,"mode")) {
                if(mode++ || strcmp(value,"new")) {fclose(f);return failure("Only generated mode=new warriors are supported");}
            } else if(!strncmp(line,"trans",5) && strlen(line)==6 && line[5]>='0' && line[5]<='3') {
                int slot=line[5]-'0',field;char *p=value,*end;
                if(warrior->formSeen&(1u<<slot)) {fclose(f);return failure("Duplicate custom form reference");}
                warrior->formSeen|=1u<<slot;
                for(field=0;field<5;field++) {
                    long number=strtol(p,&end,10);
                    if(end==p || *end!=',' || number< -32768 || number>32767) {
                        fclose(f);return failure("Malformed generated transformation record");
                    }
                    warrior->formFields[slot][field]=(int)number;p=end+1;
                }
                if(*p=='@') {
                    if(!safe_relative(p+1,0) || strlen(p+1)>=MAX_NAME) {
                        fclose(f);return failure("Unsafe custom form reference");
                    }
                    snprintf(warrior->formFolder[slot],MAX_NAME,"%s",p+1);
                } else if(strcmp(p,"-1")) {
                    fclose(f);return failure("Numeric non-custom transformation targets are not supported yet");
                }
            }
        }
        fclose(f);
        if(result<0)return failure("Invalid or overlong warrior identity manifest line");
    }
    /* The original creator omits inherited voice/style fields on default saves. */
    if(!warrior->templateName[0])snprintf(warrior->templateName,MAX_NAME,"%s",moves[0]?moves:body);
    if(!warrior->voiceName[0])snprintf(warrior->voiceName,MAX_NAME,"%s",body);
    if(!mode || !warrior->name[0] || !warrior->templateName[0] || !warrior->voiceName[0])
        return failure("Warrior identity is incomplete");
    return 1;
}
static int load_files(WarriorFiles *warrior) {
    FILE *f,*resource;
    char path[1024],line[MAX_RELATIVE];
    int result,index,members=0,i,lines=0;
    if(!warrior_path(path,sizeof(path),warrior,".warrior_built"))
        return failure("Warrior build path is too long");
    f=fopen(path,"rb");
    if(!f)return failure("Enabled warrior has no generated build manifest");
    if(line_read(f,line,sizeof(line))!=1 || strncmp(line,"#warrior ",9)) {
        fclose(f);return failure("Unrecognized warrior build manifest");
    }
    while((result=line_read(f,line,sizeof(line)))==1) {
        if(!line[0])continue;
        if(++lines>13 || !safe_relative(line,1)) {
            fclose(f);return failure("Unsupported or unsafe generated resource list");
        }
        index=role(line,&members);
        if(index<0 || warrior->resource[index][0]) {
            fclose(f);return failure("Unsupported or duplicate generated resource role");
        }
        for(i=0;i<PORT_WARRIOR_FILE_ROLES;i++)if(same_name(warrior->resource[i],line)) {
            fclose(f);return failure("Duplicate generated resource path");
        }
        if(!warrior_path(path,sizeof(path),warrior,line)) {
            fclose(f);return failure("Generated resource path is too long");
        }
        resource=fopen(path,"rb");
        if(!resource) {fclose(f);return failure("Enabled warrior is missing a generated resource");}
        for(i=0;i<4;i++)if(index==PortWarriors_ModelRole(i,0) || index==PortWarriors_ModelRole(i,1)) {
            long size;
            if(fseek(resource,0,SEEK_END) || (size=ftell(resource))<=0 || size>PORT_WARRIOR_MODEL_CAPACITY) {
                fclose(resource);fclose(f);return failure("Generated warrior model exceeds the supported capacity");
            }
            break;
        }
        fclose(resource);
        snprintf(warrior->resource[index],sizeof(warrior->resource[index]),"%s",line);
    }
    fclose(f);
    if(result<0)return failure("Invalid or overlong warrior build manifest line");
    for(i=PORT_WARRIOR_ANIMATION;i<PORT_WARRIOR_RESOURCES;i++)if(!warrior->resource[i][0])
        return failure("Enabled warrior has an incomplete generated resource set");
    for(i=0;i<4;i++) {
        int model=!!warrior->resource[PortWarriors_ModelRole(i,0)][0];
        int damage=!!warrior->resource[PortWarriors_ModelRole(i,1)][0];
        if(model!=damage || (model && i!=warrior->costumes))
            return failure("Generated costumes must be contiguous intact/damaged pairs");
        if(model)warrior->costumes++;
    }
    if(!warrior->costumes)return failure("Enabled warrior has no generated costumes");
    return 1;
}
static void init(void) {
    FILE *f;char path[1024],line[MAX_NAME];int result,i;
    if(initialized)return;
    initialized=1;
    if(!PortGame_WarriorModelsEnabled())return;
    if(snprintf(path,sizeof(path),"%s/mods/enabled.txt",Port_FileRoot())>=(int)sizeof(path)) {
        failure("Warrior enable-list path is too long");return;
    }
    f=fopen(path,"rb");
    if(!f)return; /* No enable list: no custom resources registered. */
    while((result=line_read(f,line,sizeof(line)))==1) {
        if(!line[0] || line[0]=='#')continue;
        if(!safe_relative(line,0) || count>=MAX_WARRIORS) {
            failure("Invalid enabled warrior folder or too many warriors");break;
        }
        for(i=0;i<count;i++)if(same_name(warriors[i].folder,line))break;
        if(i!=count) {failure("Duplicate enabled warrior folder");break;}
        snprintf(warriors[count].folder,sizeof(warriors[count].folder),"%s",line);
        if(!load_files(&warriors[count]) || !load_identity(&warriors[count]))break;
        count++;
    }
    fclose(f);
    if(result<0)failure("Invalid or overlong warrior enable-list line");
    if(!error[0]) {
        int w,slot,target;
        for(w=0;w<count && !error[0];w++)for(slot=0;slot<4;slot++) {
            if(!warriors[w].formFolder[slot][0])continue;
            for(target=0;target<count;target++)if(!strcmp(warriors[target].folder,warriors[w].formFolder[slot]))break;
            if(target==count) {failure("Enabled warrior references a form that is not enabled");break;}
            warriors[w].formTarget[slot]=target;
        }
    }
    if(error[0])fprintf(stderr,"warriors: resource registration failed: %s\n",error);
}
int PortWarriors_Count(void) {init();return count;}
void PortWarriors_Reload(void) {
    previewActive=0;
    memset(warriors,0,sizeof(warriors));memset(error,0,sizeof(error));
    count=0;initialized=0;init();
}
/* Authoring viewer only. Slot 250 is temporarily rebound; saved files and the
   enabled library are never modified. Restore before opening any game mode. */
int PortWarriors_Preview(int on) {
    WarriorFiles draft;char path[1024],savedError[256];FILE *f;int savedCount,good;
    init();
    if(previewActive){warriors[0]=previewSaved;count=previewCount;previewActive=0;}
    if(!on)return 1;
    memset(&draft,0,sizeof(draft));
    snprintf(path,sizeof(path),"%s/.warrior-preview-%u-path.txt",Port_FileRoot(),(unsigned)on);
    f=fopen(path,"rb");if(!f)return 0;
    good=line_read(f,draft.previewRoot,sizeof(draft.previewRoot))==1;fclose(f);
    if(!good || strncmp(draft.previewRoot,".warrior-builds/",16) || !safe_relative(draft.previewRoot,1))return 0;
    savedCount=count;memcpy(savedError,error,sizeof(error));
    good=load_files(&draft) && load_identity(&draft);
    count=savedCount;memcpy(error,savedError,sizeof(error));
    if(!good)return 0;
    previewSaved=warriors[0];previewCount=count;previewActive=1;
    warriors[0]=draft;if(count<1)count=1;
    return 1;
}
int PortWarriors_ModelRole(int outfit,int damaged) {
    static const int models[4]={PORT_WARRIOR_MODEL_1,PORT_WARRIOR_MODEL_2,PORT_WARRIOR_MODEL_3,PORT_WARRIOR_MODEL_4};
    static const int damage[4]={PORT_WARRIOR_DAMAGE_1,PORT_WARRIOR_DAMAGE_2,PORT_WARRIOR_DAMAGE_3,PORT_WARRIOR_DAMAGE_4};
    return outfit>=0 && outfit<4?(damaged?damage:models)[outfit]:-1;
}
int PortWarriors_Costumes(int index) {init();return index>=0 && index<count?warriors[index].costumes:0;}
const char *PortWarriors_Error(void) {init();return error[0]?error:NULL;}
const char *PortWarriors_Folder(int index) {
    init();return index>=0 && index<count?warriors[index].folder:NULL;
}
const char *PortWarriors_Name(int index) {
    init();return index>=0 && index<count?warriors[index].name:NULL;
}
const char *PortWarriors_AttackName(int index,int slot) {
    init();
    return index>=0 && index<count && slot>=0 && slot<5 && warriors[index].attackName[slot][0]?warriors[index].attackName[slot]:NULL;
}
int PortWarriors_AuraColour(int index) {
    init();return index>=0 && index<count?warriors[index].auraColour:-1;
}
int PortWarriors_UsesBT4Rules(int index) {
    init();return index>=0 && index<count?warriors[index].bt4Rules:0;
}
const char *PortWarriors_TemplateName(int index) {
    init();return index>=0 && index<count?warriors[index].templateName:NULL;
}
const char *PortWarriors_VoiceName(int index) {
    init();return index>=0 && index<count?warriors[index].voiceName:NULL;
}
int PortWarriors_FormTarget(int index,int form) {
    init();return index>=0 && index<count && form>=0 && form<4?warriors[index].formTarget[form]:-1;
}
int PortWarriors_FormField(int index,int form,int field) {
    init();return index>=0 && index<count && form>=0 && form<4 && field>=0 && field<5?
        warriors[index].formFields[form][field]:0;
}
void PortWarriors_TraceNode(int character,int node,int active,const void *matrix) {
    static unsigned char seen[PORT_WARRIOR_PLAYABLE_MAX][169];
    const uint32_t *words=matrix;int index=character-PORT_WARRIOR_FIRST_CHARACTER;
    if(!getenv("BT3_WARRIOR_TRACE") || index<0 || index>=PORT_WARRIOR_PLAYABLE_MAX ||
       node<0 || node>=169 || seen[index][node])return;
    seen[index][node]=1;
    fprintf(stderr,"warriornode character=%d node=%d active=%d position=%08x,%08x,%08x\n",
            character,node,active,words[12],words[13],words[14]);
}
int PortWarriors_Alias(const char *relative,char *out,unsigned size) {
    unsigned warrior,resource,flat;int length=0,written;
    if(strncmp(relative,"zwarriors/",10))return 0;
    if(out && size)out[0]=0;
    init();
    if(!out || !size || error[0])return -1;
    if(sscanf(relative,"zwarriors/%u/%u.bin%n",&warrior,&resource,&length)!=2 || !length || relative[length]) {
        length=0;
        if(sscanf(relative,"zwarriors/%u.bin%n",&flat,&length)!=1 || !length || relative[length])return -1;
        warrior=flat/PORT_WARRIOR_FILE_STRIDE;resource=flat%PORT_WARRIOR_FILE_STRIDE;
    }
    if(warrior>=(unsigned)count || resource>=PORT_WARRIOR_FILE_ROLES)return -1;
    if(resource!=PORT_WARRIOR_CHIP && !warriors[warrior].resource[resource][0])return -1;
    written=warrior_path(out,size,&warriors[warrior],resource==PORT_WARRIOR_CHIP?"chip.bin":warriors[warrior].resource[resource]);
    if(!written) {out[0]=0;return -1;}
    if(resource==PORT_WARRIOR_CHIP) {
        FILE *f=fopen(out,"rb");
        if(!f){out[0]=0;return -1;}
        fclose(f);
    }
    return 1;
}
