/* Bounds checks before downloaded generated resources reach the game readers.
   Reuse the creator's readers; a matching content digest alone is not a layout
   check. This does not accept authoring code or executable/script filenames. */
#pragma once
#include "warrior_native_pack.h"
#include "warrior_native_texture_bindings.h"
namespace WarriorNetResources {
inline bool check(int role,const unsigned char *data,unsigned size) {
    if(!size)return true;
    if(!data)return false;
    if(role==PORT_WARRIOR_PORTRAIT)return size>=8 &&
        PortGame_CheckPortraitHeader(data,8,(int)size);
    if(role==PORT_WARRIOR_CHIP)return PortGame_CheckWarriorChip(data,(int)size);
    std::string bytes(reinterpret_cast<const char *>(data),size),error;
    if(role==PORT_WARRIOR_MEMBER_1 || role==PORT_WARRIOR_MEMBER_2) {
        std::vector<WarriorNativeTextureBindings::Texture> textures;
        return size<=1024*1024 && WarriorNativeTextureBindings::textures(bytes,textures,error);
    }
    WarriorNativePack::Pack pack;
    if(!WarriorNativePack::decode(bytes,pack,error))return false;
    if(role==PORT_WARRIOR_ANIMATION)return pack.entries.size()==440 && pack.entries[0].size()>=148;
    /* Reference-created libraries use six sections; native BT4 generation can
       add a seventh. The optional native section must not exclude old saves. */
    if(role==PORT_WARRIOR_EFFECTS)return pack.entries.size()>=6;
    if((pack.entries.size()!=252 && pack.entries.size()!=253) || pack.entries[0].size()<256)return false;
    if(pack.entries.size()==253 && (pack.entries[252].size()<16 ||
       (pack.entries[252].compare(0,8,"BT3WARRI") && pack.entries[252].compare(0,8,"BT4WARRI"))))return false;
    std::vector<WarriorNativeParts::Record> records;
    if(!WarriorNativeParts::records(pack.entries[2],records,error))return false;
    std::vector<WarriorNativeTextureBindings::Texture> textures;
    if(!WarriorNativeTextureBindings::textures(pack.entries[11],textures,error))return false;
    for(auto const &entry:pack.entries)if(entry.size()>=4 && WarriorNativePack::word(entry,0)==0x6c646d70)
        if(!WarriorNativeParts::records(entry,records,error))return false;
    return true;
}
}
