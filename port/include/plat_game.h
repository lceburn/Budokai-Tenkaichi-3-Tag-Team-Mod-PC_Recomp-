#ifndef PORT_GAME_H
#define PORT_GAME_H
#ifdef __cplusplus
extern "C" {
#endif
/* Disc profiles keep native code independent of each disc's resource numbering. */
int PortGame_IsBT4(void);
int PortGame_Region(void); /* 0 USA/legacy, 1 Europe, 2 Japan */
const char *PortGame_DefaultLanguage(void);
int PortGame_VolumeCount(int volume);
int PortGame_CharacterCount(void);
int PortGame_IsCustomCharacter(int chara);
/* Local library IDs and room IDs are different namespaces. */
int PortGame_WarriorCharacter(int index);
int PortGame_WarriorIndex(int chara);
int PortGame_WarriorAuraOverride(const void *model,int size);
int PortGame_TableCharacter(int chara);
int PortGame_UsesBT4Rules(int chara);
/* Basic named form links only; returns 0xFF for unsupported/missing links. */
int PortGame_WarriorFormTarget(int chara,int slot);
int PortGame_WarriorFormField(int chara,int slot,int field);
/* Append connected form groups to a seven-column native team-selection grid.
   Cells are nine 32-bit words; capacity includes the trailing padded row. */
int PortGame_WarriorGrid(void *cells,int count,int capacity);
int PortGame_MenuPortrait(int chara);
int PortGame_CheckPortraitHeader(const void *header,int headerSize,int fileSize);
int PortGame_WarriorPortraitValid(int chara);
int PortGame_CheckWarriorChip(const void *data,int size);
int PortGame_WarriorChip(int chara);
/* Apply an imported pack's optional BT3WARRI motion translation scales to a
   freshly decoded, private animation buffer. Never changes model files. */
int PortGame_AdjustWarriorAnimation(const void *model,int modelSize,int motion,void *animation,int animationSize);
void *PortGame_GaugeTable(void *original);
int PortGame_IsCharacter(int chara);
int PortGame_IsOnlineCharacter(int chara);
/* The standard online picker may display saved-local fighters. These IDs are
   still rejected on the wire and are exported/remapped only after selection. */
void PortGame_LocalPicker(int enabled);
int PortGame_LocalPickerActive(void);
const char *PortGame_CharacterName(int chara);
const char *PortGame_WarriorAttackName(int chara,int slot);
int PortGame_WarriorAuraColour(int chara);
int PortGame_StageCount(void);
int PortGame_MusicFile(int track);
int PortGame_MusicCount(void);
int PortGame_MusicAt(int index);
int PortGame_HasMusic(int track);
int PortGame_StreamFile(int original);
int PortGame_Costumes(int chara);
int PortGame_Model(int chara, int costume, int damaged);
int PortGame_ModelCharacter(int id);
void PortGame_RepairModelBody(void *body, int size, int model);
int PortGame_Animation(int chara);
int PortGame_Combat(int chara);
int PortGame_Voice(int chara, int japanese);
int PortGame_VoiceLine(int chara, int line, int japanese);
int PortGame_Member(int chara, int side);
int PortGame_Partition(int id);
int PortGame_FileIndex(int id);
void *PortGame_Metadata(int chara, int kind);
/* Experimental BT3-layout model imports: off unless BT3_WARRIOR_MODELS=1.
   Selects embedded metadata only after checking the pack's offset table. */
int PortGame_UseEmbeddedModel(const void *data, int size);
int PortGame_EmbeddedBT4(const void *data, int size);
int PortGame_ModelCapacity(void);
int PortGame_WarriorModelsEnabled(void);
void *PortGame_CustomSlot(void *original, int slot);
int PortGame_SaveCustom(void);
void PortGame_ResetAnimations(void);
void *PortGame_AnimationBuffer(void *original);
int PortGame_AnimationCapacity(void);
int PortGame_CheckAnimation(void *data, int size);
#ifdef __cplusplus
}
#endif
#endif
