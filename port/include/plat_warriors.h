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
enum { PORT_WARRIOR_FIRST_CHARACTER=250, PORT_WARRIOR_PLAYABLE_MAX=4,
       PORT_WARRIOR_FILE_BASE=0x40000, PORT_WARRIOR_FILE_STRIDE=16 };
int PortWarriors_Count(void);
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
