// Independent attack visual donors. Technique rules and animations are unchanged.
#pragma once
#include "warrior_native_sources.h"
#include "warrior_native_effect_colours.h"
namespace WarriorNativeEffects {
inline const char *field(unsigned slot) {
    const char *keys[]={"blast1_1fx","blast1_2fx","blast2_1fx","blast2_2fx","ultimatefx"};
    return slot<5?keys[slot]:"";
}
inline const char *colour_field(unsigned slot) {
    const char *keys[]={"blast1_1colour","blast1_2colour","blast2_1colour","blast2_2colour","ultimatecolour"};return slot<5?keys[slot]:"";
}
inline bool key(const std::string &name) {
    if(name=="effects" || name=="aurafrom" || name=="aura")return true;
    for(unsigned i=0;i<5;i++)if(name==field(i) || (i>=2 && name==colour_field(i)))return true;
    return false;
}
inline bool changed(const WarriorRecipe::Document &before,const WarriorRecipe::Document &edited) {
    for(const char *key:{"effects","aurafrom","aura"})if(before.get(key)!=edited.get(key))return true;
    for(unsigned i=0;i<5;i++)if(before.get(field(i))!=edited.get(field(i)) || (i>=2 && before.get(colour_field(i))!=edited.get(colour_field(i))))return true;
    return false;
}
inline bool colour(const std::string &value,unsigned &rgb,std::string &error) {
    if(value.size()!=6 || value.find_first_not_of("0123456789abcdefABCDEF")!=std::string::npos){error="Choose a six-digit native effect colour.";return false;}
    unsigned hex=std::strtoul(value.c_str(),nullptr,16);rgb=((hex&255)<<16)|(hex&0xff00)|(hex>>16);return true;
}
inline std::string common_source(const WarriorRecipe::Document &recipe) {
    for(const char *key:{"effects","moves","body"})if(!recipe.get(key).empty())return recipe.get(key);
    return {};
}
inline bool load(const std::filesystem::path &root,const std::string &name,WarriorNativePack::Pack &out,std::string &error) {
    WarriorNativeSources::Model model;std::string raw;
    if(!WarriorNativeSources::model(root,name,model,error) || !WarriorNativeSources::asset(root,1432+model.id*10,raw,error) || !WarriorNativePack::decode(raw,out,error))return false;
    if(out.entries.size()<6){error="Incomplete native effect source.";return false;}return true;
}
inline bool source(const WarriorRecipe::Document &recipe,unsigned slot,std::string &name,unsigned &entry,std::string &error) {
    if(slot>=5){error="Invalid native effect slot.";return false;}
    auto value=recipe.get(field(slot));
    if(value.empty()) {
        const char *attacks[]={"blast1_1","blast1_2","blast2_1","blast2_2","ultimate"};
        value=recipe.get(attacks[slot]);
    }
    if(!value.empty()) {
        auto colon=value.rfind(':');unsigned maximum=slot<2?2:3;
        if(colon==std::string::npos || colon+2!=value.size() || value.back()<'1' || unsigned(value.back()-'0')>maximum) {
            error="Invalid native attack effect donor.";return false;
        }
        name=value.substr(0,colon);entry=unsigned(value.back()-'0')+(slot<2?0:2);
    }else {
        name=recipe.get("effects");if(name.empty())name=recipe.get("moves");if(name.empty())name=recipe.get("body");entry=slot+1;
    }
    if(!WarriorRecipe::value_ok(name) || name=="none") {error="Unsupported native attack effect donor.";return false;}
    return true;
}
inline bool update(const std::filesystem::path &root,WarriorNativePack::Pack &target,
                   const WarriorRecipe::Document &before,const WarriorRecipe::Document &edited,std::string &error) {
    auto next=target;
    for(unsigned i=0;i<5;i++) {
        std::string name,oldName;unsigned entry,oldEntry;WarriorNativePack::Pack effects;
        if(!source(edited,i,name,entry,error) || !source(before,i,oldName,oldEntry,error))return false;
        if(name==oldName && entry==oldEntry && (i<2 || before.get(colour_field(i))==edited.get(colour_field(i))))continue;
        if(!load(root,name,effects,error))return false;
        if(target.entries.size()<6 || effects.entries.size()<6 || entry>5) {error="Invalid native effect resource.";return false;}
        next.entries[i]=effects.entries[entry-1];
        if(i>=2 && !edited.get(colour_field(i)).empty()) {
            unsigned rgb;if(!colour(edited.get(colour_field(i)),rgb,error) || !WarriorNativeEffectColours::tint(next.entries[i],rgb,error))return false;
        }
    }
    if(common_source(before)!=common_source(edited) || before.get("aura")!=edited.get("aura")) {
        WarriorNativePack::Pack effects;if(!load(root,common_source(edited),effects,error))return false;
        if(next.entries.size()<6){error="Missing native aura resource.";return false;}next.entries[5]=effects.entries[5];
        if(!edited.get("aura").empty()) {unsigned rgb;if(!colour(edited.get("aura"),rgb,error) || !WarriorNativeEffectColours::common(next.entries[5],rgb,error))return false;}
    }
    target=std::move(next);return true;
}
inline bool assemble(const std::filesystem::path &root,const WarriorRecipe::Document &recipe,
                     WarriorNativePack::Pack &out,std::string &error) {
    std::string base=recipe.get("effects");if(base.empty())base=recipe.get("moves");if(base.empty())base=recipe.get("body");
    WarriorNativePack::Pack result;if(!load(root,base,result,error))return false;
    for(unsigned i=0;i<5;i++) {
        std::string name;unsigned entry;WarriorNativePack::Pack donor;
        if(!source(recipe,i,name,entry,error) || !load(root,name,donor,error))return false;
        result.entries[i]=donor.entries[entry-1];
        if(i>=2 && !recipe.get(colour_field(i)).empty()){unsigned rgb;if(!colour(recipe.get(colour_field(i)),rgb,error) || !WarriorNativeEffectColours::tint(result.entries[i],rgb,error))return false;}
    }
    WarriorNativePack::Pack aura;if(!load(root,common_source(recipe),aura,error))return false;result.entries[5]=aura.entries[5];
    const char *attackKeys[]={"blast2_1","blast2_2","ultimate"};
    for(unsigned target=0;target<3;target++) {
        auto loop=target==2?recipe.get("cam_ultimate_loop"):std::string();
        if(loop.empty())loop=recipe.get(attackKeys[target]);
        if(loop.empty())continue;
        auto colon=loop.rfind(':');
        if(colon==std::string::npos || colon+2!=loop.size() || loop.back()<'1' || loop.back()>'3') {
            error="Invalid native technique loop camera source.";return false;
        }
        unsigned selected=unsigned(loop.back()-'1');WarriorNativePack::Pack donor,common,sourceCommon,cameras,sourceCameras;
        if(!load(root,loop.substr(0,colon),donor,error) ||
           !WarriorNativePack::decode(result.entries[5],common,error) ||
           !WarriorNativePack::decode(donor.entries[5],sourceCommon,error) || common.entries.size()<=7 || sourceCommon.entries.size()<=7 ||
           !WarriorNativePack::decode(common.entries[7],cameras,error) ||
           !WarriorNativePack::decode(sourceCommon.entries[7],sourceCameras,error) || cameras.entries.size()!=4 || sourceCameras.entries.size()!=4) {
            error="Incomplete native technique loop camera pack.";return false;
        }
        cameras.entries[target]=sourceCameras.entries[selected];
        if(!cameras.flags.empty())cameras.flags[target]=sourceCameras.flags.empty()?0:sourceCameras.flags[selected];
        if(target==2 && selected==2) {
            cameras.entries[3]=sourceCameras.entries[3];
            if(!cameras.flags.empty())cameras.flags[3]=sourceCameras.flags.empty()?0:sourceCameras.flags[3];
        }
        if(!WarriorNativePack::encode(cameras,common.entries[7],error) ||
           !WarriorNativePack::encode(common,result.entries[5],error))return false;
    }
    if(!recipe.get("aura").empty()){unsigned rgb;if(!colour(recipe.get("aura"),rgb,error) || !WarriorNativeEffectColours::common(result.entries[5],rgb,error))return false;}
    out=std::move(result);error.clear();return true;
}
}
