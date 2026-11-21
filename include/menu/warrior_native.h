#ifndef MENU_WARRIOR_NATIVE_H
#define MENU_WARRIOR_NATIVE_H
#include "types.h"
/* Host recipe/build boundary. Addresses are explicit game-window addresses. */
s32 PortZwNative_Open(void);
s32 PortZwNative_Count(void);
s32 PortZwNative_Select(s32 index);
s32 PortZwNative_List(s32 index,u32 out,s32 capacity,s32 folder);
s32 PortZwNative_Read(s32 field,u32 out,s32 capacity);
s32 PortZwNative_Write(s32 field,u32 value);
s32 PortZwNative_Status(u32 out,s32 capacity);
s32 PortZwNative_Dirty(void);
s32 PortZwNative_New(s32 copyForm);
s32 PortZwNative_Duplicate(s32 source);
s32 PortZwNative_DonorAllowed(s32 field,u32 name);
s32 PortZwNative_DonorName(s32 index,u32 out,s32 capacity);
s32 PortZwNative_AttackName(u32 donor,s32 slot,u32 out,s32 capacity);
s32 PortZwNative_ColourCount(void);
s32 PortZwNative_ColourValue(s32 index,s32 channel);
s32 PortZwNative_ColourWrite(s32 index,s32 red,s32 green,s32 blue);
s32 PortZwNative_Build(void);
s32 PortZwNative_Poll(void);
/* 3: metadata-only draft, render this warrior's unchanged saved model.
   1/2 use the temporary preview slot; 0 uses the saved design. */
s32 PortZwNative_Preview(s32 field,u32 highlighted);
void PortZwNative_PreviewStop(void);
void PortZwNative_Trace(s32 page,s32 row,s32 tab);
const char *PortTbo_WarriorDonor(s32 index);
#endif
