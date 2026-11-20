#pragma once
#include "warrior_native_sources.h"
namespace WarriorNativeAura {
inline std::string source(const WarriorRecipe::Document &recipe) {
    for(const char *key:{"aurafrom","moves","body"})if(!recipe.get(key).empty())return recipe.get(key);
    return {};
}
inline bool apply(WarriorNativePack::Pack &model,const WarriorNativePack::Pack &donor,std::string &error) {
    if(model.entries.size()<18 || donor.entries.size()<18 || model.entries[17].size()!=256 || donor.entries[17].size()!=256) {
        error="Incomplete native aura preset parameters.";return false;
    }
    auto preset=static_cast<unsigned char>(donor.entries[17][3]);
    if(preset>10){error="Unknown native aura preset.";return false;}
    model.entries[17][3]=char(preset);error.clear();return true;
}
}
