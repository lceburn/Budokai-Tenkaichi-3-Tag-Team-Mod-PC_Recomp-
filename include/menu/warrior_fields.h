#ifndef MENU_WARRIOR_FIELDS_H
#define MENU_WARRIOR_FIELDS_H
/* Shared schema of the native authoring controls and their validated recipe keys. */
enum { ZW_TEXT, ZW_DONOR, ZW_PART, ZW_ATTACK, ZW_RGB, ZW_NUMBER, ZW_FORM, ZW_LAYER };
#define ZW_FIELD_LIST(X) \
 X(NAME,0,"name","Name",ZW_TEXT) \
 X(TITLE,0,"title","Title",ZW_TEXT) \
 X(BODY,0,"body","Body",ZW_DONOR) \
 X(VOICE,0,"voice","Voice",ZW_DONOR) \
 X(STYLE,0,"style","Fighting style",ZW_DONOR) \
 X(STANCE,0,"stance","Stance",ZW_PART) \
 X(PORTRAIT,0,"portrait","Portrait",ZW_PART) \
 X(LOOK2,0,"look2","Costume 2",ZW_PART) \
 X(LOOK3,0,"look3","Costume 3",ZW_PART) \
 X(LOOK4,0,"look4","Costume 4",ZW_PART) \
 X(HEADSET,1,"headset","Whole head",ZW_PART) \
 X(BODYSET,1,"bodyset","Whole outfit",ZW_PART) \
 X(HAIR,1,"hair","Hair",ZW_PART) \
 X(HEAD,1,"head","Head",ZW_PART) \
 X(FACE,1,"face","Face",ZW_PART) \
 X(CHEST,1,"chest","Chest",ZW_PART) \
 X(WAIST,1,"waist","Waist",ZW_PART) \
 X(UPPERARMS,1,"upperarms","Upper arms",ZW_PART) \
 X(FOREARMS,1,"forearms","Forearms",ZW_PART) \
 X(HANDS,1,"hands","Hands",ZW_PART) \
 X(HIPS,1,"hips","Hips",ZW_PART) \
 X(SHINS,1,"shins","Shins",ZW_PART) \
 X(FEET,1,"feet","Feet",ZW_PART) \
 X(ACCESSORIES,1,"accessories","Accessories",ZW_PART) \
 X(EXTRA1,1,"extra1","Extra layer 1",ZW_LAYER) \
 X(EXTRA2,1,"extra2","Extra layer 2",ZW_LAYER) \
 X(EXTRA3,1,"extra3","Extra layer 3",ZW_LAYER) \
 X(EXTRA4,1,"extra4","Extra layer 4",ZW_LAYER) \
 X(HEADSIZE,1,"headsize","Head size",ZW_NUMBER) \
 X(HAIRSIZE,1,"hairsize","Hair size",ZW_NUMBER) \
 X(HANDSIZE,1,"handsize","Hand size",ZW_NUMBER) \
 X(FOOTSIZE,1,"footsize","Foot size",ZW_NUMBER) \
 X(ARMBUILD,1,"armbuild","Arm build",ZW_NUMBER) \
 X(TORSOBUILD,1,"torsobuild","Torso build",ZW_NUMBER) \
 X(LEGBUILD,1,"legbuild","Leg build",ZW_NUMBER) \
 X(MOVES,2,"moves","Move set",ZW_DONOR) \
 X(EFFECTS,2,"effects","Effects",ZW_PART) \
 X(FINISHERS,2,"finishers","Finishers",ZW_PART) \
 X(BLAST11,2,"blast1_1","Blast 1 - first",ZW_ATTACK) \
 X(BLAST12,2,"blast1_2","Blast 1 - second",ZW_ATTACK) \
 X(BLAST21,2,"blast2_1","Blast 2 - first",ZW_ATTACK) \
 X(BLAST22,2,"blast2_2","Blast 2 - second",ZW_ATTACK) \
 X(ULTIMATE,2,"ultimate","Ultimate",ZW_ATTACK) \
 X(AURAFROM,2,"aurafrom","Aura source",ZW_PART) \
 X(AURA,2,"aura","Aura colour",ZW_RGB) \
 X(ANIM_MOVE,2,"anim_move","Movement",ZW_PART) \
 X(ANIM_CHARGE,2,"anim_charge","Charging",ZW_PART) \
 X(ANIM_MELEE,2,"anim_melee","Melee animations",ZW_PART) \
 X(ANIM_KI,2,"anim_ki","Ki animations",ZW_PART) \
 X(ANIM_GRAB,2,"anim_grab","Grab animations",ZW_PART) \
 X(ANIM_GUARD,2,"anim_guard","Guard animations",ZW_PART) \
 X(ANIM_INTRO,2,"anim_intro","Intro animation",ZW_PART) \
 X(ANIM_VICTORY,2,"anim_victory","Victory animation",ZW_PART) \
 X(ANIM_TAUNT,2,"anim_taunt","Taunt animation",ZW_PART) \
 X(CAM_B21,2,"cam_blast2_1","Blast 2 - first camera",ZW_ATTACK) \
 X(CAM_B22,2,"cam_blast2_2","Blast 2 - second camera",ZW_ATTACK) \
 X(CAM_ULT_IN,2,"cam_ultimate_in","Ultimate intro camera",ZW_ATTACK) \
 X(CAM_ULT_LOOP,2,"cam_ultimate_loop","Ultimate loop camera",ZW_ATTACK) \
 X(BLAST11NAME,2,"blast1_1name","B1 (1) name",ZW_TEXT) \
 X(BLAST12NAME,2,"blast1_2name","B1 (2) name",ZW_TEXT) \
 X(BLAST21NAME,2,"blast2_1name","B2 (1) name",ZW_TEXT) \
 X(BLAST22NAME,2,"blast2_2name","B2 (2) name",ZW_TEXT) \
 X(ULTIMATENAME,2,"ultimatename","Ultimate name",ZW_TEXT) \
 X(BLAST11COLOUR,2,"blast1_1colour","B1 (1) glow",ZW_RGB) \
 X(BLAST12COLOUR,2,"blast1_2colour","B1 (2) glow",ZW_RGB) \
 X(BLAST21COLOUR,2,"blast2_1colour","B2 (1) colour",ZW_RGB) \
 X(BLAST22COLOUR,2,"blast2_2colour","B2 (2) colour",ZW_RGB) \
 X(ULTIMATECOLOUR,2,"ultimatecolour","Ultimate colour",ZW_RGB) \
 X(BLAST11FX,2,"blast1_1fx","B1 (1) look",ZW_ATTACK) \
 X(BLAST12FX,2,"blast1_2fx","B1 (2) look",ZW_ATTACK) \
 X(BLAST21FX,2,"blast2_1fx","B2 (1) effects",ZW_ATTACK) \
 X(BLAST22FX,2,"blast2_2fx","B2 (2) effects",ZW_ATTACK) \
 X(ULTIMATEFX,2,"ultimatefx","Ultimate effects",ZW_ATTACK) \
 X(FORM1,3,"form1","Transformation 1",ZW_FORM) \
 X(FORM2,3,"form2","Transformation 2",ZW_FORM) \
 X(FORM3,3,"form3","Transformation 3",ZW_FORM) \
 X(FORM4,3,"form4","Transformation 4",ZW_FORM)
#define ZW_ENUM(id,tab,key,label,kind) ZW_##id,
enum { ZW_FIELD_LIST(ZW_ENUM) ZW_FIELD_COUNT };
#undef ZW_ENUM
typedef struct ZwField {int tab;const char *key,*label;int kind;} ZwField;
#define ZW_ROW(id,tab,key,label,kind) {tab,key,label,kind},
static const ZwField zwFields[ZW_FIELD_COUNT]={ZW_FIELD_LIST(ZW_ROW)};
/* Expose only the controls implemented by the shipped native builder. */
static int zw_native_editable(int field) {
    if(field<0 || field>=ZW_FIELD_COUNT)return 0;
    if(zwFields[field].kind==ZW_LAYER || zwFields[field].kind==ZW_NUMBER)return 0;
    switch(field) {
    case ZW_PORTRAIT:case ZW_LOOK2:case ZW_LOOK3:case ZW_LOOK4:
    case ZW_BODYSET:case ZW_ACCESSORIES:case ZW_FINISHERS:case ZW_ANIM_MELEE:
    case ZW_BLAST11COLOUR:case ZW_BLAST12COLOUR:return 0;
    default:return 1;
    }
}
#undef ZW_ROW
#endif
