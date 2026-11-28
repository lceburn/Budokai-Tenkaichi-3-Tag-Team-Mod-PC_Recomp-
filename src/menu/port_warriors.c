#ifdef PORT
#include "common.h"
#include "menu/menu_a.h"
#include "menu/port_screen.h"
#include "menu/warrior_native.h"
#include "menu/warrior_fields.h"
#include "plat_warriors.h"
#include "menu/tbo_art.h"
#include "sys/pad.h"

extern void Job_Run(void),Port_MenuPage(s32),PortTbo_TextMode(s32);
extern s32 PortTbo_TextRead(void),Port_PageAspectMilli(void),gPortOverlayOpen;
extern char *strchr(const char *,s32),*strstr(const char *,const char *);
extern void WarriorView_Open(void),WarriorView_Clear(void),WarriorView_Close(void);
extern void WarriorView_Draw(s32,f32,f32,f32,s32);
extern void WarriorView_SetVariant(s32,s32);

enum { ZW_LIBRARY,ZW_EDIT,ZW_CHOOSE,ZW_NAME_PAD,ZW_COLOUR,ZW_FORMS,ZW_BUILD,ZW_NOTICE,ZW_DISCARD,ZW_SURFACES,ZW_SCALE,ZW_DELETE };
static s32 page,tab,row,libraryRow,warrior,leaving,field,choice,choiceCount,parameter,channel,keyCursor;
static s32 choices[252],rgb[3];
static char text[512],value[512],notice[512];
static s32 lastPage,lastRow,lastTab;
static s32 surface,previewStarted,previewState;
static s32 surfaceDamaged;
static s32 layerPart;
static const char *layerKeys[]={"hair","head","face","chest","waist","upperarms","forearms","hands","hips","shins","feet","accessories","tail"};
static s32 playtestRequest;
s32 gPortWarriorPlaytest;
static const char keys[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

static void zw_page(s32 next){if(page==ZW_NAME_PAD)PortTbo_TextMode(0);page=next;if(page==ZW_NAME_PAD)PortTbo_TextMode(1);}
static void zw_info(char *message){Ps_Copy(notice,message,sizeof(notice));zw_page(ZW_NOTICE);}
static s32 zw_count(void){s32 i,n=0;for(i=0;i<ZW_FIELD_COUNT;i++)if(zwFields[i].tab==tab && zw_native_editable(i))n++;return n+(tab==3 || tab==1);}
static s32 zw_field(s32 at){s32 i;for(i=0;i<ZW_FIELD_COUNT;i++)if(zwFields[i].tab==tab && zw_native_editable(i) && at--==0)return i;return -1;}
static void zw_read(s32 id,char *out,s32 size){out[0]=0;PortZwNative_Read(id,(u32)out,size);}
static void zw_name(s32 id,char *out,s32 size){out[0]=0;PortZwNative_List(id,(u32)out,size,0);}
static void zw_end(void){leaving=1;ColorFade_StartOut(0,0,0,15);}
static void zw_status(void){PortZwNative_Status((u32)notice,sizeof(notice));zw_page(ZW_NOTICE);}
static void zw_backspace(void){s32 n=Ps_Len(text);if(!n)return;n--;while(n>0 && ((u8)text[n]&0xc0)==0x80)n--;text[n]=0;}
static s32 zw_attack_slots(s32 id){return id==ZW_BLAST11 || id==ZW_BLAST12 || id==ZW_BLAST11FX || id==ZW_BLAST12FX?2:3;}
static void zw_attack_label(s32 id,const char *source,s32 technique,char *out,s32 size) {
    char attack[264];s32 slot=zw_attack_slots(id)==2?technique-1:technique+1;
    if(PortZwNative_AttackName((u32)source,slot,(u32)attack,sizeof(attack))) {
        Ps_Copy(out,attack,size);Ps_Cat(out," | ",size);Ps_Cat(out,source,size);
    }else {char suffix[32];Ps_Copy(out,source,size);sprintf(suffix," : %d",technique);Ps_Cat(out,suffix,size);}
}
static void zw_done_text(void){if(PortZwNative_Write(field,(u32)text))zw_page(ZW_EDIT);else zw_info(field>=ZW_BLAST11NAME && field<=ZW_ULTIMATENAME?"Attack names allow up to 31 bytes. Leave blank to use the original name.":"Please enter a name or title before continuing.");}

static void zw_choose(s32 id) {
    s32 i;const char *name;char sourceValue[512],*separator;
    field=id;choice=0;choiceCount=0;parameter=id==ZW_ULTIMATE || id==ZW_ULTIMATEFX?3:1;
    zw_read(field,value,sizeof(value));
    Ps_Copy(sourceValue,value,sizeof(sourceValue));
    if(zwFields[id].kind==ZW_LAYER) {
        layerPart=0;
        for(i=0;i<13;i++)if(!strncmp(value,layerKeys[i],strlen(layerKeys[i])) && value[strlen(layerKeys[i])]==':') {
            layerPart=i;Ps_Copy(sourceValue,value+strlen(layerKeys[i])+1,sizeof(sourceValue));break;
        }
    }
    if(zwFields[id].kind==ZW_PART || zwFields[id].kind==ZW_ATTACK || zwFields[id].kind==ZW_LAYER)choices[choiceCount++]=-2;
    if(!strcmp(value,"none") && choiceCount>1 && choices[1]==-1)choice=1;
    separator=strstr(sourceValue,", costume ");
    if(separator){if(separator[10]>='1' && separator[10]<='4')parameter=separator[10]-'0';*separator=0;}
    if(zwFields[id].kind==ZW_ATTACK) {
        separator=strchr(sourceValue,':');
        if(separator){if(separator[1]>='1' && separator[1]<='0'+zw_attack_slots(id))parameter=separator[1]-'0';*separator=0;}
    }
    for(i=0;(name=PortTbo_WarriorDonor(i))!=NULL && choiceCount<252;i++)
        if(PortZwNative_DonorAllowed(id,(u32)name)) {
            choices[choiceCount]=i;
            if(!strcmp(sourceValue,name))choice=choiceCount;
            choiceCount++;
        }
    if(!choiceCount){zw_info("There are no compatible source fighters in this category.");return;}
    zw_page(ZW_CHOOSE);
}
static void zw_donor_value(void) {
    s32 source=choices[choice];
    value[0]=0;
    if(source==-1)Ps_Copy(value,"none",sizeof(value));
    else if(source>=0) {
        if(zwFields[field].kind==ZW_LAYER){Ps_Copy(value,layerKeys[layerPart],sizeof(value));Ps_Cat(value,":",sizeof(value));Ps_Cat(value,PortTbo_WarriorDonor(source),sizeof(value));}
        else Ps_Copy(value,PortTbo_WarriorDonor(source),sizeof(value));
        if(zwFields[field].kind==ZW_ATTACK) {char suffix[3]={':','0'+parameter,0};Ps_Cat(value,suffix,sizeof(value));}
        else if(parameter>1 && field!=ZW_VOICE && field!=ZW_STYLE && field!=ZW_MOVES) {
            char suffix[32];sprintf(suffix,", costume %d",parameter);Ps_Cat(value,suffix,sizeof(value));
        }
    }
}
static void zw_accept_donor(void) {
    zw_donor_value();
    if(PortZwNative_Write(field,(u32)value))zw_page(ZW_EDIT);else zw_info("That source cannot be used for this setting.");
}
static s32 zw_hex(s32 c){if(c>='0' && c<='9')return c-'0';if(c>='a' && c<='f')return c-'a'+10;if(c>='A' && c<='F')return c-'A'+10;return 15;}
static void zw_edit_field(void) {
    s32 i;field=zw_field(row);
    if(field<0) {
        if(tab==1) {
            if(!PortZwNative_ColourRefresh()){zw_status();return;}
            surfaceDamaged=0;PortZwNative_ColourMode(0);
            if(!PortZwNative_ColourCount()){zw_info("Save your parts first to refresh the surface colour choices.");return;}
            choice=0;zw_page(ZW_SURFACES);return;
        }
        s32 added=PortZwNative_New(warrior);
        if(added<0){zw_status();return;}
        warrior=added;tab=0;row=0;zw_page(ZW_EDIT);return;
    }
    zw_read(field,value,sizeof(value));
    switch(zwFields[field].kind) {
    case ZW_TEXT:Ps_Copy(text,value,sizeof(text));keyCursor=0;zw_page(ZW_NAME_PAD);break;
    case ZW_RGB:
        for(i=0;i<3;i++)rgb[i]=Ps_Len(value)==6?16*zw_hex(value[2*i])+zw_hex(value[2*i+1]):255;
        channel=0;zw_page(ZW_COLOUR);break;
    case ZW_FORM:choice=0;parameter=1;zw_page(ZW_FORMS);break;
    case ZW_NUMBER:
        parameter=100;sscanf(value,"%d",&parameter);if(parameter<50 || parameter>180)parameter=100;
        zw_page(ZW_SCALE);break;
    default:zw_choose(field);break;
    }
}

static void zw_update(void) {
    u32 pressed=gPad[0].gamePressed,repeat=gPad[0].gameRepeat;
    s32 i,side=Ps_Side();
    if(page==ZW_BUILD) {
        s32 result=PortZwNative_Poll();
        if(result){if(result==1){previewState=0;warrior=PortZwNative_Selected();}libraryRow=warrior<PortZwNative_Count()?warrior+1:1;zw_status();}return;
    }
    if(page==ZW_LIBRARY) {
        Ps_Move(&libraryRow,PortZwNative_Count()+2);
        if(pressed&PADG_TRIANGLE) {if(PortZwNative_Dirty())zw_page(ZW_DISCARD);else zw_end();}
        else if((pressed&PADG_CIRCLE) && libraryRow>0 && libraryRow<=PortZwNative_Count()) {
            warrior=libraryRow-1;zw_name(warrior,value,sizeof(value));
            Ps_Copy(notice,"Delete ",sizeof(notice));Ps_Cat(notice,value,sizeof(notice));
            Ps_Cat(notice,"? This removes the character from your roster.",sizeof(notice));
            zw_page(ZW_DELETE);
        }
        else if((pressed&PADG_SQUARE) && libraryRow>0 && libraryRow<=PortZwNative_Count()) {
            warrior=PortZwNative_Duplicate(libraryRow-1);
            if(warrior<0)zw_status();else {libraryRow=warrior+1;tab=0;row=0;zw_page(ZW_EDIT);}
        }
        else if(pressed&PADG_CROSS) {
            if(libraryRow==PortZwNative_Count()+1) {if(PortZwNative_Dirty())zw_page(ZW_DISCARD);else zw_end();}
            else if(libraryRow==0) {
                warrior=PortZwNative_New(-1);
                if(warrior<0)zw_status();else {tab=0;row=0;zw_page(ZW_EDIT);}
            }else {warrior=libraryRow-1;PortZwNative_Select(warrior);tab=0;row=0;zw_page(ZW_EDIT);}
        }
    }else if(page==ZW_EDIT) {
        if(pressed&PADG_R1){tab=(tab+1)%4;row=0;}
        if(pressed&PADG_L1){tab=(tab+3)%4;row=0;}
        Ps_Move(&row,zw_count());
        if(pressed&PADG_TRIANGLE){libraryRow=warrior+1;zw_page(ZW_LIBRARY);}
        else if(pressed&PADG_SELECT) {
            if(PortZwNative_Dirty())zw_info("Save your changes before starting a playtest.");
            else {playtestRequest=PortWarriors_LocalCharacter(warrior);zw_end();}
        }
        else if(pressed&PADG_START){if(PortZwNative_Build())zw_page(ZW_BUILD);else zw_status();}
        else if((pressed&PADG_CIRCLE) && zw_field(row)==ZW_HAIR && zw_native_editable(ZW_EXTRA1)) {
            for(i=ZW_EXTRA1;i<=ZW_EXTRA4;i++){zw_read(i,value,sizeof(value));if(!value[0])break;}
            if(i>ZW_EXTRA4)zw_info("All four extra layers are in use. Edit an existing layer first.");
            else zw_choose(i);
        }
        else if(pressed&PADG_CROSS)zw_edit_field();
    }else if(page==ZW_CHOOSE) {
        Ps_Move(&choice,choiceCount);
        if(zwFields[field].kind==ZW_LAYER) {
            if(side)layerPart=Ps_Wrap(layerPart+side,0,12);
            if(pressed&PADG_L1)parameter=Ps_Wrap(parameter-1,1,4);
            if(pressed&PADG_R1)parameter=Ps_Wrap(parameter+1,1,4);
        }else if(zwFields[field].kind==ZW_ATTACK && side)
            parameter=Ps_Wrap(parameter+side,1,zwFields[field].kind==ZW_ATTACK?zw_attack_slots(field):4);
        if(pressed&PADG_TRIANGLE)zw_page(ZW_EDIT);
        else if(pressed&PADG_CROSS)zw_accept_donor();
    }else if(page==ZW_NAME_PAD) {
        s32 c;
        while((c=PortTbo_TextRead())!=0 && page==ZW_NAME_PAD) {
            s32 n=Ps_Len(text);
            if(c==13)zw_done_text();else if(c==27)zw_page(ZW_EDIT);else if(c==8)zw_backspace();
            else if(c>=32 && c!=127 && n<190){text[n]=(char)c;text[n+1]=0;}
        }
        if(page!=ZW_NAME_PAD)return;
        if(repeat&PADG_RIGHT)keyCursor=(keyCursor+1)%68;
        if(repeat&PADG_LEFT)keyCursor=(keyCursor+67)%68;
        if(repeat&PADG_DOWN)keyCursor=(keyCursor+10)%68;
        if(repeat&PADG_UP)keyCursor=(keyCursor+58)%68;
        if(pressed&PADG_TRIANGLE)zw_page(ZW_EDIT);
        else if(pressed&PADG_START)zw_done_text();
        else if(pressed&PADG_SQUARE)zw_backspace();
        else if(pressed&PADG_CROSS) {
            s32 n=Ps_Len(text);
            if(keyCursor==65)zw_backspace();else if(keyCursor==66)zw_done_text();else if(keyCursor==67)zw_page(ZW_EDIT);
            else if(n<190){text[n]=keyCursor<64?keys[keyCursor]:' ';text[n+1]=0;}
        }
    }else if(page==ZW_COLOUR) {
        Ps_Move(&channel,3);if(side)rgb[channel]=Ps_Wrap(rgb[channel]+side*((gPad[0].gameHeld&PADG_L2)?1:8),0,255);
        if(pressed&PADG_TRIANGLE)zw_page(field<0?ZW_SURFACES:ZW_EDIT);
        else if(pressed&PADG_CROSS){
            if(field<0){if(!PortZwNative_ColourWrite(surface,rgb[0],rgb[1],rgb[2]))zw_status();else zw_page(ZW_SURFACES);}
            else {sprintf(value,"%02x%02x%02x",rgb[0],rgb[1],rgb[2]);PortZwNative_Write(field,(u32)value);zw_page(ZW_EDIT);}
        }
    }else if(page==ZW_SCALE) {
        if(side){parameter+=side*5;if(parameter<50)parameter=50;if(parameter>180)parameter=180;}
        if(pressed&PADG_SQUARE)parameter=100;
        if(pressed&PADG_TRIANGLE)zw_page(ZW_EDIT);
        else if(pressed&PADG_CROSS){sprintf(value,"%d",parameter);if(PortZwNative_Write(field,(u32)value))zw_page(ZW_EDIT);else zw_status();}
    }else if(page==ZW_SURFACES) {
        Ps_Move(&choice,PortZwNative_ColourCount());
        if(pressed&(PADG_L1|PADG_R1)) {
            surfaceDamaged=!surfaceDamaged;PortZwNative_ColourMode(surfaceDamaged);choice=0;
        }
        if(pressed&PADG_TRIANGLE)zw_page(ZW_EDIT);
        else if(pressed&PADG_CROSS){surface=choice;for(i=0;i<3;i++)rgb[i]=PortZwNative_ColourValue(surface,i);field=-1;channel=0;zw_page(ZW_COLOUR);}
        else if(pressed&PADG_SQUARE){if(!PortZwNative_ColourCopy(choice))zw_status();}
        else if(pressed&PADG_CIRCLE){if(!PortZwNative_ColourPaste(choice))zw_status();}
        else if(pressed&PADG_R2){if(!PortZwNative_ColourReset(choice))zw_status();}
    }else if(page==ZW_FORMS) {
        Ps_Move(&choice,PortZwNative_Count()+1);if(side)parameter=Ps_Wrap(parameter+side,0,5);
        if(pressed&PADG_TRIANGLE)zw_page(ZW_EDIT);
        else if(pressed&PADG_CROSS) {
            value[0]=0;
            if(choice>0) {
                char target[192],prefix[16];
                if(choice-1==warrior){zw_info("Choose another form of your character.");return;}
                PortZwNative_List(choice-1,(u32)target,sizeof(target),1);
                sprintf(prefix,"%d,0,",parameter);Ps_Copy(value,prefix,sizeof(value));Ps_Cat(value,target,sizeof(value));
            }
            PortZwNative_Write(field,(u32)value);zw_page(ZW_EDIT);
        }
    }else if(page==ZW_DELETE) {
        if(pressed&PADG_TRIANGLE)zw_page(ZW_LIBRARY);
        else if(pressed&PADG_CROSS) {
            if(PortZwNative_Delete(warrior)) {
                warrior=-1;libraryRow=libraryRow>PortZwNative_Count()?PortZwNative_Count():libraryRow;
                previewStarted=previewState=0;
            }
            zw_status();
        }
    }else if(page==ZW_NOTICE) {
        if(pressed&(PADG_CROSS|PADG_TRIANGLE)){zw_page(warrior>=0 && warrior<PortZwNative_Count()?ZW_EDIT:ZW_LIBRARY);if(libraryRow>PortZwNative_Count()+1)libraryRow=0;}
    }else if(page==ZW_DISCARD) {
        if(pressed&PADG_CROSS){PortZwNative_Open();zw_end();}
        else if(pressed&PADG_TRIANGLE)zw_page(ZW_LIBRARY);
    }
}

static void zw_bar(f32 y,char *label,s32 selected) {
    Ps_Pill(selected?TBO_ZW_BAR_SEL:TBO_ZW_BAR,10,y,307,y+29,0x80);
    Ps_Text(23,y+3,label,1.02f,271,selected?1:2);
}
static void zw_backdrop(void) {
    f32 aspect=(f32)Port_PageAspectMilli()/1000.0f,pic=1376.0f/768.0f;
    f32 u=aspect<pic?aspect/pic:1,v=aspect<pic?1:pic/aspect;
    Ps_SpriteScreen(TBO_ZW_BG,0,0,512,448,.5f-u*.5f,.5f-v*.5f,.5f+u*.5f,.5f+v*.5f,PS_PLAIN,0x80);
    Ps_Icon(TBO_ZW_EMBLEM,33,32,50,0x80);Ps_Align(0);Ps_Heading(61,8,"Z WARRIORS",2.25f,400,0x80);
    Ps_Stand(TBO_ZW_HOST,456,354,174,0x80);
    Ps_Icon(TBO_ZW_PLATFORM,385,297,100,0x80);
}
static void zw_draw(void) {
    s32 i,first,count,preview=-1;char line[512],heading[192];
    Ps_PageSet();Ps_SetShift(0);Ps_Align(0);zw_backdrop();Ps_PromptsClear();
    if(page==ZW_LIBRARY && libraryRow>0 && libraryRow<=PortZwNative_Count())preview=PortWarriors_LocalCharacter(libraryRow-1);
    else if(page==ZW_EDIT || page==ZW_CHOOSE || page==ZW_COLOUR || page==ZW_FORMS || page==ZW_SURFACES || page==ZW_SCALE)preview=PortWarriors_LocalCharacter(warrior);
    if(page==ZW_EDIT || page==ZW_CHOOSE || page==ZW_COLOUR || page==ZW_FORMS || page==ZW_SURFACES || page==ZW_SCALE) {
        // Browsing never starts a rebuild. Cross first applies the choice to
        // the draft; only that assembled draft is then sent to the helper.
        previewStarted=1;previewState=PortZwNative_Preview(-1,0);
        if(previewState==1 || previewState==2)preview=250;
    }else if(page==ZW_NOTICE && previewStarted) {
        preview=previewState==1 || previewState==2?PORT_WARRIOR_FIRST_CHARACTER:PortWarriors_LocalCharacter(warrior);
    }else if(previewStarted) {
        PortZwNative_PreviewStop();previewStarted=0;previewState=0;
    }
    if(!PortGame_IsCustomCharacter(preview))preview=-1;
    if(page==ZW_SURFACES || (page==ZW_COLOUR && field<0)) {
        s32 picked=page==ZW_SURFACES?choice:surface;
        WarriorView_SetVariant(PortZwNative_ColourValue(picked,4),surfaceDamaged);
    }else WarriorView_SetVariant(0,0);
    WarriorView_Draw(preview,383,318,185,!gPortOverlayOpen && (page==ZW_EDIT || page==ZW_LIBRARY));
    Ps_PageSet();Ps_SetShift(0);Ps_Align(0);
    if(page==ZW_LIBRARY) {
        count=PortZwNative_Count()+2;first=libraryRow/8*8;
        for(i=first;i<count && i<first+8;i++) {
            if(i==0)Ps_Copy(line,"+ Create a New Warrior",sizeof(line));
            else if(i==count-1)Ps_Copy(line,"Back",sizeof(line));else zw_name(i-1,line,sizeof(line));
            zw_bar(76+(i-first)*32,line,i==libraryRow);
        }
        if(libraryRow>0 && libraryRow<count-1) {
            zw_name(libraryRow-1,heading,sizeof(heading));Ps_Heading(322,93,heading,1.15f,175,0x80);
        }
        Ps_Prompt(0,"Create / Edit");Ps_Prompt(2,"Back");
        if(libraryRow>0 && libraryRow<=PortZwNative_Count()){Ps_Prompt(1,"Duplicate");Ps_Prompt(4,"Delete");}
        Ps_Copy(line,"Choose a warrior to edit, or create a new one.",sizeof(line));
    }else if(page==ZW_EDIT) {
        static char *tabs[]={"WARRIOR","PARTS","MOVES","FORMS"};
        for(i=0;i<4;i++) {
            f32 x=10+i*76;Ps_Pill(i==tab?TBO_ZW_TAB:TBO_ZW_TAB_OFF,x,67,x+73,91,0x80);
            Ps_Align(1);Ps_Text(x+36.5f,69,tabs[i],.74f,65,i==tab?1:2);Ps_Align(0);
        }
        first=row/8*8;count=zw_count();Ps_Rect(10,99,307,360,Ps_Rgba(77,37,134,112),Ps_Rgba(56,25,96,112));
        for(i=first;i<count && i<first+8;i++) {
            s32 id=zw_field(i);f32 y=104+(i-first)*31;
            if(i==row)Ps_Pill(TBO_ZW_BAR_SEL,13,y,304,y+29,0x80);
            if(id<0){Ps_Text(23,y+4,tab==1?"Surface colours":"+ Add a Form",1,270,i==row?1:2);continue;}
            zw_read(id,value,sizeof(value));
            if(zwFields[id].kind==ZW_ATTACK && value[0]) {
                char source[512],*separator;Ps_Copy(source,value,sizeof(source));separator=strchr(source,':');
                if(separator && separator[1]>='1' && separator[1]<='0'+zw_attack_slots(id)) {
                    s32 technique=separator[1]-'0';*separator=0;zw_attack_label(id,source,technique,value,sizeof(value));
                }
            }
            if(zwFields[id].kind==ZW_NUMBER) {
                if(!value[0])Ps_Copy(value,"100",sizeof(value));
                Ps_Cat(value,"%",sizeof(value));
            }
            Ps_Text(22,y+3,(char *)zwFields[id].label,.76f,112,i==row?1:2);
            Ps_Text(137,y+3,value[0]?value:"Default",.76f,157,2);
        }
        zw_name(warrior,heading,sizeof(heading));Ps_Heading(322,103,heading,1.1f,175,0x80);
        Ps_Text(324,135,previewState==0?"Saved design":previewState==1 || previewState==3?"Draft preview":previewState==-2?"Preview unavailable":"Updating preview...",.67f,170,2);
        Ps_Text(324,153,"R stick: turn | L2 / R2: zoom",.52f,170,2);
        Ps_Prompt(0,"Change");Ps_Prompt(7,"Save all");Ps_Prompt(2,"Library");
        Ps_Copy(line,"L1 / R1: tabs. SELECT: playtest the saved character.",sizeof(line));
    }else if(page==ZW_CHOOSE || page==ZW_FORMS) {
        if(page==ZW_CHOOSE)Ps_Text(324,120,previewState==0 || previewState==1 || previewState==3?"Current design":previewState==-2?"Preview unavailable":"Updating selected design...",.67f,170,2);
        Ps_Heading(16,65,(char *)zwFields[field].label,1.2f,300,0x80);
        count=page==ZW_FORMS?PortZwNative_Count()+1:choiceCount;first=choice/7*7;
        for(i=first;i<count && i<first+7;i++) {
            if(page==ZW_FORMS) {if(!i)Ps_Copy(line,"No transformation",sizeof(line));else zw_name(i-1,line,sizeof(line));}
            else if(choices[i]==-2)Ps_Copy(line,zwFields[field].kind==ZW_LAYER?"None":"Default",sizeof(line));
            else if(choices[i]==-1)Ps_Copy(line,"None",sizeof(line));
            else if(zwFields[field].kind==ZW_ATTACK)zw_attack_label(field,PortTbo_WarriorDonor(choices[i]),parameter,line,sizeof(line));
            else Ps_Copy(line,PortTbo_WarriorDonor(choices[i]),sizeof(line));
            zw_bar(103+(i-first)*32,line,i==choice);
        }
        if(page==ZW_FORMS)sprintf(line,"Transformation cost: %d",parameter);
        else if(zwFields[field].kind==ZW_LAYER) {
            char suffix[64];Ps_Copy(line,"Layer: ",sizeof(line));Ps_Cat(line,layerKeys[layerPart],sizeof(line));
            sprintf(suffix," | L1/R1 costume: %d",parameter);Ps_Cat(line,suffix,sizeof(line));
        }
        else if(zwFields[field].kind==ZW_ATTACK)sprintf(line,"Technique slot: %d",parameter);
        else if(field!=ZW_VOICE && field!=ZW_STYLE && field!=ZW_MOVES)sprintf(line,"Source costume: %d",parameter);
        else if(field==ZW_STYLE)Ps_Copy(line,"Stats donor: health, combat stats and ki rates.",sizeof(line));
        else if(field==ZW_MOVES)Ps_Copy(line,"Punches, kicks and rush combos; default techniques follow this fighter.",sizeof(line));
        else Ps_Copy(line,"Select the fighter whose voice you want.",sizeof(line));
        if(page==ZW_CHOOSE) {
            if(field==ZW_HEADSET)Ps_Cat(line," | Whole head replaces hair, head and face. Cross applies.",sizeof(line));
            else if(field==ZW_HEAD)Ps_Cat(line," | Skull and ears only; Hair and Face stay separate.",sizeof(line));
            else if(field==ZW_EFFECTS)Ps_Copy(line,"Ki-blast and charge effects only; techniques keep their own effects.",sizeof(line));
            else if(field==ZW_ANIM_CHARGE)Ps_Copy(line,"Charge poses only; ki rates, aura and blast effects stay separate.",sizeof(line));
            else if(field==ZW_AURAFROM)Ps_Copy(line,"Aura flames, body light and ki trails; Default follows Melee moves.",sizeof(line));
            else Ps_Cat(line," | Cross applies the choice, then updates the preview.",sizeof(line));
        }
        Ps_Prompt(0,"Choose");Ps_Prompt(3,"Adjust");Ps_Prompt(2,"Cancel");
    }else if(page==ZW_NAME_PAD) {
        Ps_Heading(16,68,(char *)zwFields[field].label,1.2f,440,0x80);
        Ps_Pill(TBO_ZW_BAR_SEL,10,104,502,143,0x80);Ps_Text(22,110,text,1.1f,464,2);
        for(i=0;i<68;i++) {
            f32 x=11+(i%10)*49,y=158+(i/10)*27;char label[12];
            if(i==keyCursor)Ps_Pill(TBO_ZW_TAB,x,y,x+46,y+25,0x80);
            if(i<64){label[0]=keys[i];label[1]=0;}else Ps_Copy(label,i==64?"Space":i==65?"Del":i==66?"Done":"Back",sizeof(label));
            Ps_Align(1);Ps_Text(x+23,y+3,label,.85f,43,i==keyCursor?1:2);Ps_Align(0);
        }
        Ps_Copy(line,"Use your keyboard or choose letters with the controller.",sizeof(line));
        Ps_Prompt(0,"Letter");Ps_Prompt(1,"Delete");Ps_Prompt(7,"Done");Ps_Prompt(2,"Cancel");
    }else if(page==ZW_COLOUR) {
        static char *channels[]={"Red","Green","Blue"};
        Ps_Heading(16,68,field<0?"Surface colour":(char *)zwFields[field].label,1.2f,300,0x80);
        for(i=0;i<3;i++){char digits[16];zw_bar(115+i*43,channels[i],i==channel);sprintf(digits,"%d",rgb[i]);Ps_Text(235,121+i*43,digits,1,60,2);}
        Ps_Rect(340,115,422,203,Ps_Rgba(rgb[0],rgb[1],rgb[2],128),Ps_Rgba(rgb[0],rgb[1],rgb[2],128));
        Ps_Copy(line,"Adjust left or right. Hold L2 for fine colour changes.",sizeof(line));Ps_Prompt(0,"Apply");Ps_Prompt(2,"Cancel");
    }else if(page==ZW_SCALE) {
        Ps_Heading(16,68,(char *)zwFields[field].label,1.2f,300,0x80);
        sprintf(line,"%d%%",parameter);zw_bar(150,line,1);
        Ps_Text(324,120,previewState==0 || previewState==1 || previewState==3?"Current design":previewState==-2?"Preview unavailable":"Updating preview...",.67f,170,2);
        Ps_Prompt(0,"Apply");Ps_Prompt(3,"Adjust");Ps_Prompt(2,"Cancel");
        Ps_Copy(line,"50-180%, in 5% steps. Square restores 100%.",sizeof(line));
    }else if(page==ZW_SURFACES) {
        count=PortZwNative_ColourCount();first=choice/7*7;
        Ps_Heading(16,65,surfaceDamaged?"SURFACE COLOURS: DAMAGED":"SURFACE COLOURS: NORMAL",.95f,300,0x80);
        for(i=first;i<count && i<first+7;i++) {
            s32 r=PortZwNative_ColourValue(i,0),g=PortZwNative_ColourValue(i,1),b=PortZwNative_ColourValue(i,2);f32 y=103+(i-first)*32;
            sprintf(line,"Costume %d - Surface %d",PortZwNative_ColourValue(i,4)+1,PortZwNative_ColourValue(i,3)+1);zw_bar(y,line,i==choice);
            Ps_Rect(252,y+6,294,y+24,Ps_Rgba(r,g,b,128),Ps_Rgba(r,g,b,128));
        }
        Ps_Copy(line,"L1/R1: Normal/Damaged. Square: Copy. Circle: Paste. R2: Reset colour.",sizeof(line));
        Ps_Prompt(0,"Colour");Ps_Prompt(1,"Copy");Ps_Prompt(2,"Back");Ps_Prompt(4,"Paste");
    }else {
        char *message=page==ZW_BUILD?"Building and validating your warriors...":page==ZW_DISCARD?"Discard unsaved creator changes and leave?":notice;
        Ps_Rect(14,112,498,280,Ps_Rgba(77,37,134,120),Ps_Rgba(56,25,96,120));
        Ps_Text(29,137,message,.95f,454,2);
        if(page==ZW_DELETE){Ps_Prompt(0,"Delete");Ps_Prompt(2,"Cancel");}
        else if(page==ZW_DISCARD){Ps_Prompt(0,"Discard");Ps_Prompt(2,"Keep editing");}
        else if(page!=ZW_BUILD)Ps_Prompt(0,"Continue");
        Ps_Copy(line,page==ZW_BUILD?"Your current playable files are kept until the build passes validation.":"Z Warriors",sizeof(line));
    }
    Ps_Panel(TBO_ZW_DIALOG,8,368,504,414,0x80);Ps_Text(22,376,line,.77f,468,2);Ps_PromptsDraw();
}

s32 PortZw_Run(void) {
    s32 beforePage,beforeRow,beforeTab;
    PortZwNative_Open();WarriorView_Open();playtestRequest=-1;previewStarted=previewState=0;page=ZW_LIBRARY;libraryRow=0;warrior=0;tab=row=leaving=0;lastPage=lastRow=lastTab=-1;
    ColorFade_StartIn(0,0,0,15);
    while(1) {
        Job_Run();Gfx_BeginFrame();Pad_Update();Snd_Update();ColorFade_Update();
        if(!leaving && ColorFade_IsInDone() && !gPortOverlayOpen)zw_update();
        Port_MenuPage(52);zw_draw();ColorFade_Draw();Port_MenuPage(0);
        beforePage=page;beforeRow=page==ZW_LIBRARY?libraryRow:page==ZW_EDIT?row:choice;beforeTab=tab;
        if(beforePage!=lastPage || beforeRow!=lastRow || beforeTab!=lastTab) {
            PortZwNative_Trace(beforePage,beforeRow,beforeTab);lastPage=beforePage;lastRow=beforeRow;lastTab=beforeTab;
        }
        File_Stub264D90();Gfx_EndFrame(1);Dma_Flush();
        if(leaving && ColorFade_IsOutDone())break;
    }
    PortTbo_TextMode(0);sceGsSyncPath(0,0);PortZwNative_PreviewStop();WarriorView_Close();
    if(playtestRequest>=0) {
        extern void Battle_ClearWork(void),BattleSetup_Finish(void),TagSession_Leave(void);
        extern void BattleSetup_SetRule(s32,s32,s32,s32,s32,s32,s32);
        extern void BattleSetup_SetSide(s32,s32,s32,s32,s32,s32,s32,void *);
        extern void BattleSetup_SetMember(s32,s32,s32,s32,s32,s32,f32,void *);
        static u8 noItems[16];
        TagSession_Leave();Dma_ResetBuffers();Battle_ClearWork();
        BattleSetup_SetRule(0,6,8,0,1,4,0);
        BattleSetup_SetSide(0,0,0,1,1,1,0,NULL);BattleSetup_SetSide(1,2,1,1,1,1,0,NULL);
        BattleSetup_SetMember(0,0,playtestRequest,0,0,-1,100.0f,noItems);
        BattleSetup_SetMember(1,0,0,0,0,-1,100.0f,noItems);BattleSetup_Finish();
        gPortWarriorPlaytest=1;return 1;
    }
    return 0;
}
#endif
