// Reference animation categories, separate from technique and camera tracks.
#pragma once
#include "warrior_native_sources.h"
#include "warrior_native_parts.h"
#include <cmath>
#include <cstring>
namespace WarriorNativeMotionGroups {
inline const char *key(unsigned group) {
    const char *keys[]={"anim_move","anim_charge","anim_melee","anim_ki","anim_grab","anim_guard","anim_intro","anim_victory","anim_taunt","stance"};
    return group<10?keys[group]:"";
}
inline int group(const std::string &key) {for(unsigned i=0;i<10;i++)if(key==WarriorNativeMotionGroups::key(i))return int(i);return -1;}
inline const unsigned *ranges(unsigned group) {
    static const unsigned table[10][8]={{14,24,40,12,396,1,0,0},{52,3,0,0,0,0,0,0},{55,60,141,7,0,0,0,0},{122,19,0,0,0,0,0,0},{148,10,390,5,408,1,0,0},{236,18,0,0,0,0,0,0},{385,2,0,0,0,0,0,0},{387,2,0,0,0,0,0,0},{389,2,0,0,0,0,0,0},{0,2,7,1,38,2,395,1}};
    return group<10?table[group]:nullptr;
}
inline bool height(const WarriorNativePack::Pack &model,float &out,std::string &error) {
    std::vector<WarriorNativeParts::Record> rows;
    if(model.entries.size()<3 || !WarriorNativeParts::records(model.entries[2],rows,error))return false;
    for(const auto &r:rows)if(r.node==3) {
        auto bits=WarriorNativePack::word(model.entries[2],r.offset+20);float value;std::memcpy(&value,&bits,4);
        if(!std::isfinite(value) || value>=-.5f){error="Invalid native motion donor height.";return false;}
        out=value;return true;
    }
    error="Missing native motion donor height.";return false;
}
struct Selection {unsigned group;float height;WarriorNativePack::Pack motions;std::string standing;};
struct RangeScale {unsigned first,count;float height;};
inline bool scale_standing(std::string &data,float ratio,std::string &error) {
    // B14's stripped model families can omit this optional standing stream;
    // their normal animation pack supplies the stance instead. Preserve null.
    if(data.empty() && std::isfinite(ratio) && ratio>0 && ratio<=16){error.clear();return true;}
    if(data.size()<148 || data.size()>WarriorNativePack::Limit || !std::isfinite(ratio) || !(ratio>0 && ratio<=16)) {
        error="Invalid native standing animation.";return false;
    }
    size_t header=148+size_t(static_cast<unsigned char>(data[1]))*16;std::set<size_t> tracks;
    auto half=[&](size_t at){return unsigned(static_cast<unsigned char>(data[at]))|(unsigned(static_cast<unsigned char>(data[at+1]))<<8);};
    for(unsigned node=0;node<71;node++) {
        size_t offset=size_t(half(6+node*2))*4;if(!offset)continue;
        if(offset<header || offset>data.size() || data.size()-offset<4){error="Invalid native standing track offset.";return false;}
        if(half(offset)&1)continue;
        unsigned count=half(offset+2);
        if(!count || count>(data.size()-offset-4)/24){error="Invalid native standing translation keys.";return false;}
        tracks.insert(offset);
        for(unsigned n=0;n<count;n++)for(unsigned k=0;k<3;k++) {
            auto bits=WarriorNativePack::word(data,offset+4+n*24+k*4);float value;std::memcpy(&value,&bits,4);
            if(!std::isfinite(value) || !std::isfinite(value*ratio)){error="Invalid native standing translation.";return false;}
        }
    }
    auto result=data;
    for(auto offset:tracks)for(unsigned n=0;n<half(offset+2);n++)for(unsigned k=0;k<3;k++) {
        size_t at=offset+4+n*24+k*4;auto bits=WarriorNativePack::word(data,at);float value;std::memcpy(&value,&bits,4);value*=ratio;std::memcpy(&bits,&value,4);WarriorNativePack::put(result,at,bits);
    }
    data=std::move(result);error.clear();return true;
}
inline bool prepare(const std::filesystem::path &root,const WarriorRecipe::Document &recipe,std::vector<Selection> &out,std::string &error) {
    std::vector<Selection> result;
    for(unsigned i=0;i<10;i++)if(!recipe.get(key(i)).empty()) {
        if(i==2){error="Native melee overrides still need semantic compatibility support.";return false;}
        WarriorNativeSources::Model model;Selection s;s.group=i;std::string raw;
        if(!WarriorNativeSources::model(root,recipe.get(key(i)),model,error) || !height(model.pack,s.height,error) ||
           !WarriorNativeSources::asset(root,1431+model.id*10,raw,error) || !WarriorNativePack::decode(raw,s.motions,error))return false;
        if(i==9){if(model.pack.entries.size()<36){error="Missing native standing stream.";return false;}s.standing=model.pack.entries[35];}
        result.push_back(std::move(s));
    }
    out=std::move(result);error.clear();return true;
}
inline bool apply(WarriorNativePack::Pack &motions,const std::vector<Selection> &selections,std::string &error) {
    auto result=motions;
    for(const auto &s:selections) {
        if(s.group>=10 || s.group==2 || result.entries.size()!=s.motions.entries.size() || result.entries.size()<414) {
            error="Incompatible native motion category layout.";return false;
        }
        auto r=ranges(s.group);
        for(unsigned j=0;j<8 && r[j+1];j+=2)if(!WarriorNativePack::copy_entries(result,s.motions,r[j]+1,r[j]+r[j+1],error))return false;
    }
    motions=std::move(result);error.clear();return true;
}
inline bool scales(WarriorNativePack::Pack &model,float bodyHeight,float movesHeight,float outfitHeight,
                   const std::vector<Selection> &selections,std::string &error,bool sameBodyMoves=false,
                   const std::vector<RangeScale> &attackScales={}) {
    if(model.entries.size()<26 || !std::isfinite(bodyHeight) || !std::isfinite(movesHeight) || !std::isfinite(outfitHeight) || bodyHeight>=-.5f || movesHeight>=-.5f || outfitHeight>=-.5f) {
        error="Invalid native animation scaling input.";return false;
    }
    bool bt4=model.entries.size()==253 && model.entries[252].compare(0,8,"BT4WARRI")==0;
    float fallback=sameBodyMoves?1.f:outfitHeight/movesHeight;unsigned count=0;std::string table(bt4?"BT4WARRI":"BT3WARRI",8);table.resize(16);
    auto putFloat=[&](size_t at,float value){uint32_t bits;std::memcpy(&bits,&value,4);WarriorNativePack::put(table,at,bits);};
    if(!(fallback>0 && fallback<=16)){error="Invalid native default animation scale.";return false;}putFloat(8,fallback);
    auto append=[&](unsigned group,float donorHeight) {
        float ratio=outfitHeight/donorHeight;
        if(!std::isfinite(ratio) || !(ratio>0 && ratio<=16))return false;
        auto r=ranges(group);for(unsigned i=0;i<8 && r[i+1];i+=2) {
            size_t at=table.size();table.resize(at+8);WarriorNativePack::put(table,at,r[i]|(r[i+1]<<16));putFloat(at+4,ratio);count++;
        }
        return true;
    };
    float stanceHeight=bodyHeight;unsigned last=0;bool have=false;
    for(const auto &s:selections) {
        if(s.group>=10 || s.group==2 || (have && s.group<=last)){error="Ambiguous native animation category order.";return false;}
        last=s.group;have=true;if(s.group==9){stanceHeight=s.height;continue;}
        if(!append(s.group,s.height)){error="Invalid native category animation scale.";return false;}
    }
    unsigned end=0;
    for(const auto &s:attackScales) {
        float ratio=outfitHeight/s.height;
        if(s.first<end || s.first>=440 || !s.count || s.count>440-s.first ||
           !std::isfinite(s.height) || s.height>=-.5f || !std::isfinite(ratio) || !(ratio>0 && ratio<=16)) {
            error="Invalid native attack animation scale.";return false;
        }
        end=s.first+s.count;size_t at=table.size();table.resize(at+8);
        WarriorNativePack::put(table,at,s.first|(s.count<<16));putFloat(at+4,ratio);count++;
    }
    if(!append(9,stanceHeight)){error="Invalid native stance animation scale.";return false;}
    WarriorNativePack::put(table,12,count);std::string standing;bool replaced=false;
    for(const auto &s:selections)if(s.group==9) {
        if(model.entries.size()<36){error="Missing native standing-stream target.";return false;}
        standing=s.standing;if(!scale_standing(standing,outfitHeight/s.height,error))return false;replaced=true;
    }
    // B14 uses entry 26 for original fighter data. Keep that data intact and
    // put our scaling extension after the 252 embedded records instead.
    if(!bt4 && (model.entries[25].empty() || model.entries[25].compare(0,8,"BT3WARRI")==0))
        model.entries[25]=std::move(table);
    else {
        if(model.entries.size()!=252 && !(model.entries.size()==253 &&
           (model.entries[252].compare(0,8,"BT3WARRI")==0 || bt4))) {
            error="Unsupported native animation extension layout.";return false;
        }
        if(model.entries.size()==252) {
            model.entries.push_back(std::move(table));
            if(!model.flags.empty())model.flags.push_back(0);
        }else model.entries[252]=std::move(table);
    }
    if(replaced)model.entries[35]=std::move(standing);
    error.clear();return true;
}
}
