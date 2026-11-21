#ifndef WARRIOR_NET_PACK_H
#define WARRIOR_NET_PACK_H
#ifdef __cplusplus
extern "C" {
#endif
/* Wire packages contain generated resources and bounded playback metadata,
   never authoring recipes, arbitrary filenames, scripts, or executables. */
enum { WNP_MAX_BYTES=128*1024*1024, WNP_ROOT_SIZE=192 };
/* Export only the selected local fighter and its reachable transformations.
   Unused saved characters do not consume transfer or room capacity. base receives
   the selected form's package index; selecting another form in the same family
   produces the same bytes and must reuse the existing room slots. */
int WarriorNetPack_Export(int selected,unsigned char **bytes,unsigned *size,unsigned *base);
/* Export the preserved saved-local library even after room registration. */
int WarriorNetPack_ExportLocal(int selected,unsigned char **bytes,unsigned *size,unsigned *base);
void WarriorNetPack_Free(void *bytes);
int WarriorNetPack_Check(const void *bytes,unsigned size);
int WarriorNetPack_CheckSelection(const void *bytes,unsigned size,unsigned form,unsigned costume);
/* Process-owned room directories only. Removal refuses live registrations,
   foreign paths, and linked/reparse-point destinations or descendants. */
int WarriorNetPack_CreateRoom(char prefix[WNP_ROOT_SIZE]);
int WarriorNetPack_RemoveRoom(const char *prefix);
/* prefix is .warrior-sessions/<locally generated room token>. Fresh owned
   directories only; existing files and the permanent library are untouched.
   Returns the number of forms, or zero; roots receives count relative paths. */
int WarriorNetPack_Install(const void *bytes,unsigned size,const char *prefix,
                          int bundle,char roots[][WNP_ROOT_SIZE],int capacity);
#ifdef __cplusplus
}
#endif
#endif
