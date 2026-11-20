// Read technique labels from the donor's own English skill-list script.
#pragma once
#include "warrior_native_pack.h"
#include <array>
namespace WarriorNativeAttackNames {
inline bool extract(const WarriorNativePack::Pack &model,std::array<std::string,5> &out,std::string &error) {
    if(model.entries.size()<45){error="Missing native technique names.";return false;}
    const auto &script=model.entries[44];
    if(script.empty() || script.size()%2 || script.size()>65536){error="Invalid native technique name script.";return false;}
    auto word=[&](size_t i){return unsigned(static_cast<unsigned char>(script[i]))|(unsigned(static_cast<unsigned char>(script[i+1]))<<8);};
    std::array<std::string,5> result;unsigned count=0;
    for(size_t start=0;start<script.size() && count<5;) {
        size_t end=start;while(end<script.size() && word(end)!=0 && word(end)!=10 && word(end)!=13)end+=2;
        size_t at=start;if(at==0 && at<end && word(at)==0xfeff)at+=2;
        if(end-at>=6 && word(at)=='!') {
            at+=4;
            if(end-at>=8 && word(at)=='&')at+=8;
            if(end-at>=8 && word(at)=='*') {
                at+=8;std::string name;
                while(at<end) {
                    unsigned c=word(at);at+=2;
                    if(c>=0xd800 && c<=0xdbff) {
                        if(at>=end || word(at)<0xdc00 || word(at)>0xdfff){error="Invalid native technique name encoding.";return false;}
                        c=0x10000+((c-0xd800)<<10)+(word(at)-0xdc00);at+=2;
                    }else if(c>=0xdc00 && c<=0xdfff){error="Invalid native technique name encoding.";return false;}
                    if(c<32 || c==127){error="Invalid native technique name character.";return false;}
                    if(c<0x80)name+=char(c);
                    else if(c<0x800){name+=char(0xc0|(c>>6));name+=char(0x80|(c&63));}
                    else if(c<0x10000){name+=char(0xe0|(c>>12));name+=char(0x80|((c>>6)&63));name+=char(0x80|(c&63));}
                    else {name+=char(0xf0|(c>>18));name+=char(0x80|((c>>12)&63));name+=char(0x80|((c>>6)&63));name+=char(0x80|(c&63));}
                    if(name.size()>256){error="Native technique name is too long.";return false;}
                }
                while(!name.empty() && name.back()==' ')name.pop_back();
                result[count++]=std::move(name);
            }
        }
        if(end==script.size() || word(end)==0)break;
        start=end+2;if(start<script.size() && word(end)==13 && word(start)==10)start+=2;
    }
    if(count!=5){error="Incomplete native technique name list.";return false;}
    out=std::move(result);error.clear();return true;
}
}
