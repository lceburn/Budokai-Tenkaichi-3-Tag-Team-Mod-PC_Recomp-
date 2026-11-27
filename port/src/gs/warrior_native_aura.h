#pragma once
#include "warrior_native_sources.h"
namespace WarriorNativeAura {
inline std::string source(const WarriorRecipe::Document &recipe) {
    for(const char *key:{"aurafrom","moves","body"})if(!recipe.get(key).empty())return recipe.get(key);
    return {};
}
inline bool apply(WarriorNativePack::Pack &model,const WarriorNativePack::Pack &donor,std::string &error,bool explicitSource=true) {
    if(model.entries.size()<18 || donor.entries.size()<18 || model.entries[17].size()!=256 || donor.entries[17].size()!=256) {
        error="Incomplete native aura preset parameters.";return false;
    }
    auto preset=static_cast<unsigned char>(donor.entries[17][3]);
    if(preset>10){error="Unknown native aura preset.";return false;}
    auto next=model;
    size_t index=252;
    if(next.entries.size()==252 && next.entries[25].compare(0,8,"BT3WARRI")==0)index=25;
    else if(next.entries.size()==252) {
        std::string table("BT3WARRI",8);table.resize(16,'\0');
        WarriorNativePack::put(table,8,0x3f800000);next.entries.push_back(table);
        if(!next.flags.empty())next.flags.push_back(0);
    }
    if(next.entries.size()<=index){error="Missing native aura metadata.";return false;}
    auto &table=next.entries[index];
    if(table.size()<16 || (table.compare(0,8,"BT3WARRI") && table.compare(0,8,"BT4WARRI"))) {
        error="Invalid native aura metadata.";return false;
    }
    size_t count=WarriorNativePack::word(table,12);
    if(count>(table.size()-16)/8){error="Truncated native aura metadata.";return false;}
    size_t end=16+count*8;
    if(explicitSource) {
        if(table.size()<end+8)table.resize(end+8,'\0');
        table.replace(end,4,"AURA");WarriorNativePack::put(table,end+4,preset);
    }else if(table.size()>=end+8 && table.compare(end,4,"AURA")==0)table.replace(end,8,8,'\0');
    next.entries[17][3]=char(preset);model=std::move(next);error.clear();return true;
}
}
