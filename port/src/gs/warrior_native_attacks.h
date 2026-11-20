// Independent attack behavior and motion lanes; effects are assembled separately.
#pragma once
#include "warrior_native_sources.h"
#include "warrior_native_attack_layout.h"
#include "warrior_native_motion_groups.h"
namespace WarriorNativeAttacks {
using WarriorNativePack::Pack;
inline const char *field(unsigned slot) {
    const char *keys[]={"blast1_1","blast1_2","blast2_1","blast2_2","ultimate"};
    return slot<5?keys[slot]:"";
}
struct Selection {unsigned slot,sourceSlot;WarriorNativeSources::Model source;Pack motions;float height;};
inline bool key(const std::string &value) {
    for(unsigned i=0;i<5;i++)if(value==field(i))return true;
    return value=="cam_blast2_1" || value=="cam_blast2_2" || value=="cam_ultimate_in" || value=="cam_ultimate_loop";
}
struct CameraSelection {unsigned destination,sourceSlot;WarriorNativeSources::Model source;};
inline bool prepare_cameras(const std::filesystem::path &root,const WarriorRecipe::Document &recipe,std::vector<CameraSelection> &out,std::string &error) {
    const char *keys[]={"cam_blast2_1","cam_blast2_2","cam_ultimate_in"};
    std::vector<CameraSelection> result;
    for(unsigned i=0;i<3;i++)if(!recipe.get(keys[i]).empty()) {
        auto value=recipe.get(keys[i]);auto colon=value.rfind(':');
        if(colon==std::string::npos || colon+2!=value.size() || value.back()<'1' || value.back()>'3') {
            error="Invalid native attack camera source.";return false;
        }
        CameraSelection s;s.destination=i;s.sourceSlot=unsigned(value.back()-'1');
        if(!WarriorNativeSources::model(root,value.substr(0,colon),s.source,error))return false;
        result.push_back(std::move(s));
    }
    out=std::move(result);error.clear();return true;
}
inline bool camera(Pack &target,const CameraSelection &selection,std::string &error) {
    const auto &source=selection.source.pack;
    if(selection.destination>=3 || selection.sourceSlot>=3 || target.entries.size()<28 || source.entries.size()<28 ||
       target.entries[23].size()<0x244 || target.entries[27].size()<0x90 || source.entries[27].size()<0x90 ||
       WarriorNativeSources::bt4_origin(target)!=WarriorNativeSources::bt4_origin(source)) {
        error="Incomplete or incompatible native attack camera.";return false;
    }
    // The reference retains cinematic rush cameras; only ordinary technique
    // cameras use this independently selected opening shot.
    auto kind=static_cast<unsigned char>(target.entries[23][0x138+selection.destination]);
    if(kind==2 || kind==7){error.clear();return true;}
    target.entries[27].replace(selection.destination*0x24,0x24,source.entries[27],selection.sourceSlot*0x24,0x24);
    if(selection.destination==2 && selection.sourceSlot==2)
        target.entries[27].replace(0x6c,0x24,source.entries[27],0x6c,0x24);
    error.clear();return true;
}
inline bool prepare(const std::filesystem::path &root,const WarriorRecipe::Document &recipe,std::vector<Selection> &out,std::string &error) {
    std::vector<Selection> result;
    for(unsigned i=0;i<5;i++)if(!recipe.get(field(i)).empty()) {
        auto value=recipe.get(field(i));auto colon=value.rfind(':');unsigned maximum=i<2?2:3;
        if(colon==std::string::npos || colon+2!=value.size() || value.back()<'1' || unsigned(value.back()-'0')>maximum) {
            error="Invalid native attack source slot.";return false;
        }
        Selection s;s.slot=i;s.sourceSlot=unsigned(value.back()-'1')+(i<2?0:2);std::string raw;
        if(!WarriorNativeSources::model(root,value.substr(0,colon),s.source,error) ||
           !WarriorNativeMotionGroups::height(s.source.pack,s.height,error) ||
           !WarriorNativeSources::asset(root,1431+s.source.id*10,raw,error) ||
           !WarriorNativePack::decode(raw,s.motions,error))return false;
        result.push_back(std::move(s));
    }
    out=std::move(result);error.clear();return true;
}
inline std::vector<WarriorNativeMotionGroups::RangeScale> scale_ranges(const std::vector<Selection> &selections) {
    std::vector<WarriorNativeMotionGroups::RangeScale> result;
    for(const auto &s:selections) {
        result.push_back({s.slot<2?259+s.slot:261+(s.slot-2)*34,s.slot<2?1u:34u,s.height});
        if(s.slot==4 && s.sourceSlot==4)result.push_back({363,1,s.height});
    }
    return result;
}
inline bool label_span(const std::string &script,unsigned slot,size_t &begin,size_t &end,size_t &used) {
    if(script.empty() || script.size()%2 || script.size()>65536 || slot>=5)return false;
    auto word=[&](size_t at){return unsigned(static_cast<unsigned char>(script[at]))|(unsigned(static_cast<unsigned char>(script[at+1]))<<8);};
    used=0;while(used<script.size() && word(used))used+=2;
    unsigned found=0;
    for(size_t start=0;start<used;) {
        size_t finish=start;while(finish<used && word(finish)!=10 && word(finish)!=13)finish+=2;
        size_t at=start;if(at==0 && at<finish && word(at)==0xfeff)at+=2;
        size_t first=at;
        if(finish-at>=6 && word(at)=='!') {
            at+=4;if(finish-at>=8 && word(at)=='&')at+=8;
            if(finish-at>=8 && word(at)=='*' && found++==slot){begin=first;end=finish;return true;}
        }
        start=finish+2;if(start<used && word(finish)==13 && word(start)==10)start+=2;
    }
    return false;
}
inline bool names(Pack &target,const Pack &source,unsigned destination,unsigned selected,std::string &error) {
    if(target.entries.size()<52 || source.entries.size()<52 || destination>=5 || selected>=5) {
        error="Missing native attack label source.";return false;
    }
    auto result=target;bool bt4=WarriorNativeSources::bt4_origin(target);
    for(unsigned i=bt4?44:43;i<(bt4?45:52);i++) {
        if(target.entries[i].empty() || source.entries[i].empty())continue;
        size_t a,b,used,c,d,sourceUsed;
        if(!label_span(target.entries[i],destination,a,b,used) || !label_span(source.entries[i],selected,c,d,sourceUsed)) {
            error="Incomplete native attack label list.";return false;
        }
        auto text=target.entries[i].substr(0,used);
        text.replace(a,b-a,source.entries[i],c,d-c);text.append(2,'\0');
        if(text.size()>65536){error="Native attack labels exceed their script buffer.";return false;}
        text.resize((text.size()+63)&~size_t(63),'\0');result.entries[i]=std::move(text);
    }
    target=std::move(result);error.clear();return true;
}
inline bool parameters(Pack &target,const Pack &source,unsigned destination,unsigned selected,std::string &error) {
    if(destination>=5 || selected>=5 || (destination<2)!=(selected<2) || target.entries.size()<52 || source.entries.size()<52 ||
       (!target.flags.empty() && target.flags.size()!=target.entries.size()) ||
       (!source.flags.empty() && source.flags.size()!=source.entries.size()) ||
       WarriorNativeSources::bt4_origin(target)!=WarriorNativeSources::bt4_origin(source)) {
        error="Incompatible native attack slots or data profiles.";return false;
    }
    unsigned count,slots=destination<2?2:3,index=destination<2?24:23;
    const auto *fields=destination<2?WarriorNativeAttackLayout::skill(count):WarriorNativeAttackLayout::super(count);
    unsigned to=destination<2?destination:destination-2,from=selected<2?selected:selected-2;
    auto result=target;
    for(unsigned i=0;i<count;i++)for(unsigned n=0;n<fields[i].count;n++) {
        size_t a=fields[i].offset+(n*slots+to)*fields[i].unit,b=fields[i].offset+(n*slots+from)*fields[i].unit;
        if(a>result.entries[index].size() || fields[i].unit>result.entries[index].size()-a ||
           b>source.entries[index].size() || fields[i].unit>source.entries[index].size()-b) {
            error="Incomplete native attack parameter lane.";return false;
        }
        result.entries[index].replace(a,fields[i].unit,source.entries[index],b,fields[i].unit);
    }
    if(destination>=2) {
        // B14 adds a partner number for each technique in each of four costumes.
        if(WarriorNativeSources::bt4_origin(target)) {
            if(result.entries[23].size()<0x25c || source.entries[23].size()<0x25c) {
                error="Incomplete BT4 attack partner table.";return false;
            }
            for(unsigned costume=0;costume<4;costume++)
                result.entries[23].replace(0x244+(costume*3+to)*2,2,source.entries[23],0x244+(costume*3+from)*2,2);
        }
        if(result.entries[27].size()<0x90 || source.entries[27].size()<0x90) {
            error="Incomplete native attack camera table.";return false;
        }
        result.entries[27].replace(to*0x24,0x24,source.entries[27],from*0x24,0x24);
        if(to==2 && from==2)result.entries[27].replace(0x6c,0x24,source.entries[27],0x6c,0x24);
        result.entries[40+to]=source.entries[40+from];
        if(!result.flags.empty())result.flags[40+to]=source.flags.empty()?0:source.flags[40+from];
    }
    target=std::move(result);error.clear();return true;
}
inline bool motions(Pack &target,const Pack &source,unsigned destination,unsigned selected,std::string &error) {
    if(destination>=5 || selected>=5 || (destination<2)!=(selected<2) || target.entries.size()!=440 || source.entries.size()!=440) {
        error="Incompatible native attack animation slots.";return false;
    }
    unsigned to=destination<2?259+destination:261+(destination-2)*34;
    unsigned from=selected<2?259+selected:261+(selected-2)*34;
    unsigned count=destination<2?1:34;auto result=target;
    if((!target.flags.empty() && target.flags.size()!=440) || (!source.flags.empty() && source.flags.size()!=440)) {
        error="Incomplete native attack animation flags.";return false;
    }
    for(unsigned i=0;i<count;i++) {
        result.entries[to+i]=source.entries[from+i];
        if(!result.flags.empty())result.flags[to+i]=source.flags.empty()?0:source.flags[from+i];
    }
    if(destination==4 && selected==4) {
        result.entries[363]=source.entries[363];
        if(!result.flags.empty())result.flags[363]=source.flags.empty()?0:source.flags[363];
    }
    target=std::move(result);error.clear();return true;
}
}
