#ifndef PLAT_WARRIORS_H
#define PLAT_WARRIORS_H
#ifdef __cplusplus
extern "C" {
#endif
/* Resource discovery only. Indices here are NOT game character slot ids. */
enum {
    PORT_WARRIOR_MODEL_1, PORT_WARRIOR_MODEL_2,
    PORT_WARRIOR_DAMAGE_1, PORT_WARRIOR_DAMAGE_2,
    PORT_WARRIOR_ANIMATION, PORT_WARRIOR_EFFECTS,
    PORT_WARRIOR_MEMBER_1, PORT_WARRIOR_MEMBER_2,
    PORT_WARRIOR_PORTRAIT, PORT_WARRIOR_RESOURCES
};
/* Optional native-format roster chip; not required by .warrior_built. */
enum { PORT_WARRIOR_CHIP=PORT_WARRIOR_RESOURCES };
/* Keep existing virtual identities stable as costume families grow. */
enum { PORT_WARRIOR_MODEL_3=10, PORT_WARRIOR_DAMAGE_3,
       PORT_WARRIOR_MODEL_4, PORT_WARRIOR_DAMAGE_4, PORT_WARRIOR_FILE_ROLES };
enum { PORT_WARRIOR_FIRST_CHARACTER=250, PORT_WARRIOR_PLAYABLE_MAX=6,
       PORT_WARRIOR_FILE_BASE=0x40000, PORT_WARRIOR_FILE_STRIDE=16 };
/* Room identities do not occupy the legacy byte-sized local character range.
   Capacity counts base fighters AND every linked transformation. */
enum { PORT_WARRIOR_SESSION_FIRST=512, PORT_WARRIOR_SESSION_MAX=19 };
/* Read-only saved-local resource keys, separate from the active room registry. */
enum { PORT_WARRIOR_LOCAL_KEY=128 };
/* Preserve existing local identities and skip the game's 0xFF "no fighter"
   sentinel. The sixth saved form uses 256; room identities remain separate. */
static inline int PortWarriors_LocalCharacter(int index) {
    return index<0?-1:PORT_WARRIOR_FIRST_CHARACTER+index+(index>=5);
}
static inline int PortWarriors_LocalIndex(int character) {
    return character<PORT_WARRIOR_FIRST_CHARACTER || character==255?-1:
        character-PORT_WARRIOR_FIRST_CHARACTER-(character>255);
}
/* Main-menu/lobby boundary only. Each root must be a validated, relative
   .warrior-sessions/<session>/<form> directory. Activation is all-or-nothing;
   passing count=0 restores the local registry without writing its library. */
int PortWarriors_Session(const char *const *roots,int count);
int PortWarriors_SessionActive(void);
unsigned PortWarriors_SessionEpoch(void);
int PortWarriors_Count(void);
/* Saved-local metadata only, including while a room registry is active. This
   read-only copy never exposes a downloaded fighter as an exportable local. */
typedef struct PortWarriorLocal {
    char name[192],style[192],voice[192],attack[5][32];
    int aura,rules,target[4],field[4][5];
    char file[PORT_WARRIOR_FILE_ROLES][1600];
} PortWarriorLocal;
int PortWarriors_LocalCount(void);
int PortWarriors_LocalCopy(int index,PortWarriorLocal *out);
/* Main-menu authoring only: no live battle may retain custom resource bindings. */
void PortWarriors_Reload(void);
/* Positive owner tag activates a process-local staging path; zero restores. */
int PortWarriors_Preview(int owner);
int PortWarriors_Costumes(int index);
int PortWarriors_ModelRole(int outfit,int damaged);
const char *PortWarriors_Folder(int index);
const char *PortWarriors_Name(int index);
const char *PortWarriors_TemplateName(int index);
const char *PortWarriors_VoiceName(int index);
const char *PortWarriors_AttackName(int index,int slot);
/* Packed 0xBBGGRR, or -1 when no custom aura colour was saved. */
int PortWarriors_AuraColour(int index);
int PortWarriors_UsesBT4Rules(int index);
/* Resolved import index for a named custom form, or -1. No numeric manifest
   field is interpreted as a gameplay rule by the resource registry. */
int PortWarriors_FormTarget(int index,int form);
int PortWarriors_FormField(int index,int form,int field);
void PortWarriors_TraceNode(int character,int node,int active,const void *matrix);
const char *PortWarriors_Error(void);
/* Returns 1 for a resolved virtual resource, 0 for another namespace, -1
   for an invalid/unavailable warrior resource. Never falls back to disc data. */
int PortWarriors_Alias(const char *relative, char *out, unsigned size);
#ifdef __cplusplus
}
#endif
#endif
