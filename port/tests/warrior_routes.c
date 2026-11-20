/* Host-loader integration test, driven by synthetic folders in the Python test. */
#include "plat_warriors.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *Port_FileRoot(void) { return getenv("BT3_DATA"); }
int PortGame_WarriorModelsEnabled(void) {
    const char *value=getenv("BT3_WARRIOR_MODELS");return value && !strcmp(value,"1");
}
void Port_StateTouch(void *p,unsigned long n) {(void)p;(void)n;}
int PortStages_Alias(const char *r,char *o,unsigned n) {(void)r;(void)o;(void)n;return 0;}
int PortSongs_Alias(const char *r,char *o,unsigned n) {(void)r;(void)o;(void)n;return 0;}
int Port_StageFits(const char *r,long n) {(void)r;(void)n;return 1;}
int Port_StageBufSize(void) {return 0;}
extern FILE *Port_FileOpenHost(const char *r,char *out,size_t n);
extern long Port_FileSize(const char *r);
extern size_t Port_FileReadAt(const char *r,long offset,void *buf,size_t n);
extern void Port_FileWarriorClose(void);

int main(int argc,char **argv) {
    int expected,i,j;long size;char relative[128],path[1024],tiny[1],data[3],direct[3];FILE *f;
    assert(argc==3);expected=atoi(argv[1]);
    assert(PortWarriors_Count()==expected);
    assert(!PortWarriors_UsesBT4Rules(-1) && !PortWarriors_UsesBT4Rules(expected));
    if(expected)assert(PortWarriors_UsesBT4Rules(0)==(getenv("WARRIOR_BT4_RULES")!=NULL));
    if(expected>1)assert(!PortWarriors_UsesBT4Rules(1));
    assert(PortWarriors_AuraColour(-1)==-1 && PortWarriors_AuraColour(expected)==-1);
    if(getenv("WARRIOR_AURA_COLOUR") && expected) {
        assert(PortWarriors_AuraColour(0)==(int)strtol(getenv("WARRIOR_AURA_COLOUR"),NULL,16));
        if(expected>1)assert(PortWarriors_AuraColour(1)==-1);
    }
    if(getenv("WARRIOR_ROUTES_PREVIEW")) {
        char before[192],resolved[1024];int valid=!strcmp(getenv("WARRIOR_ROUTES_PREVIEW"),"valid");
        snprintf(before,sizeof(before),"%s",PortWarriors_Name(0));
        assert(PortWarriors_Preview(1)==valid);
        assert(PortWarriors_Count()==expected && !PortWarriors_Error());
        if(valid) {
            assert(!strcmp(PortWarriors_Name(0),"Preview Name"));
            assert(PortWarriors_Alias("zwarriors/0/0.bin",resolved,sizeof(resolved))==1);
            assert(strstr(resolved,".warrior-builds/test/Preview/"));
        }
        assert(PortWarriors_Preview(0));
        assert(PortWarriors_Count()==expected && !strcmp(PortWarriors_Name(0),before));
    }
    assert((PortWarriors_Error()!=NULL)==atoi(argv[2]));
    assert(PortWarriors_Folder(-1)==NULL && PortWarriors_Folder(expected)==NULL);
    assert(PortWarriors_AttackName(-1,0)==NULL && PortWarriors_AttackName(expected,0)==NULL);
    assert(PortWarriors_AttackName(0,-1)==NULL && PortWarriors_AttackName(0,5)==NULL);
    if(getenv("WARRIOR_ATTACK_NAMES") && expected) {
        assert(!strcmp(PortWarriors_AttackName(0,0),"First custom attack"));
        assert(!strcmp(PortWarriors_AttackName(0,4),"Last custom attack"));
        assert(PortWarriors_AttackName(0,1)==NULL);
        if(expected>1)assert(PortWarriors_AttackName(1,0)==NULL);
    }
    assert(PortWarriors_Alias("pzs4us1/00000.bin",path,sizeof(path))==0);
    assert(PortWarriors_Alias("zwarriors/9999/0.bin",path,sizeof(path))==-1);
    assert(PortWarriors_Alias("zwarriors/0/9.bin",path,sizeof(path))==-1);
    assert(PortWarriors_Alias("zwarriors/0/0.bin/../test",path,sizeof(path))==-1);
    assert(Port_FileOpenHost("zwarriors/9999/0.bin",path,sizeof(path))==NULL);
    for(i=0;i<expected;i++)for(j=0;j<PORT_WARRIOR_FILE_ROLES;j++) {
        int k,required=j>=PORT_WARRIOR_ANIMATION && j<PORT_WARRIOR_RESOURCES;
        int costumes=PortWarriors_Costumes(i);
        assert(costumes>=1 && costumes<=4);
        for(k=0;k<costumes;k++)if(j==PortWarriors_ModelRole(k,0) || j==PortWarriors_ModelRole(k,1))required=1;
        snprintf(relative,sizeof(relative),"zwarriors/%d/%d.bin",i,j);
        if(!required){assert(PortWarriors_Alias(relative,path,sizeof(path))==-1);continue;}
        assert(PortWarriors_Folder(i));
        assert(PortWarriors_Name(i) && PortWarriors_TemplateName(i) && PortWarriors_VoiceName(i));
        if(expected==2)assert(PortWarriors_FormTarget(i,0)==1-i);
        assert(PortWarriors_Alias(relative,path,sizeof(path))==1);
        assert(PortWarriors_Alias(relative,tiny,sizeof(tiny))==-1 && !tiny[0]);
        f=Port_FileOpenHost(relative,path,sizeof(path));assert(f);
        fseek(f,0,SEEK_END);size=ftell(f);assert(size>=3);
        assert(!fseek(f,1,SEEK_SET));assert(fread(direct,1,2,f)==2);fclose(f);
        assert(Port_FileSize(relative)==size);
        assert(Port_FileReadAt(relative,1,data,2)==2);
        assert(!memcmp(data,direct,2));
    }
    /* Non-warrior paths keep their original fallback behavior. */
    f=Port_FileOpenHost("original.bin",path,sizeof(path));assert(f);fclose(f);
    if(expected) {
        char identity[1024];
        Port_FileWarriorClose();
        assert(PortWarriors_Alias("zwarriors/0/0.bin",path,sizeof(path))==1);
        f=fopen(path,"wb");assert(f);assert(fwrite("abcd",1,4,f)==4);fclose(f);
        assert(Port_FileSize("zwarriors/0/0.bin")==4);
        assert(Port_FileSize("original.bin")==3);
        snprintf(identity,sizeof(identity),"%s/mods/%s/warrior.txt",Port_FileRoot(),PortWarriors_Folder(0));
        f=fopen(identity,"wb");assert(f);
        fputs("name=Reloaded Warrior\nstyle=Template Fighter\nvoice=Voice Fighter\n",f);fclose(f);
        PortWarriors_Reload();assert(PortWarriors_Count()==expected);
        assert(!strcmp(PortWarriors_Name(0),"Reloaded Warrior"));
        Port_FileWarriorClose();
    }
    puts("PASS: warrior discovery and host-loader routing");return 0;
}
