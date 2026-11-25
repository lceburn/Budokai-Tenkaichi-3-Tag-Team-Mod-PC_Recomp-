// Read stock idle taunts from the player's prepared disc data; no game assets
// are embedded in the executable. Decoder bounds apply before animation use.
#pragma once
#include "warrior_native_sources.h"
namespace WarriorStockTaunts {
inline bool decode(const std::string &src,std::string &out) {
    if(src.size()<8)return false;
    size_t total=WarriorNativePack::word(src,0),packed=WarriorNativePack::word(src,4);
    if(total<148 || total>0x10000 || !packed || packed>src.size()-8)return false;
    size_t at=8,end=8+packed;std::string result;result.reserve(total);
    uint64_t steps=0,budget=total*512u+1048576u;
    auto take=[&](unsigned &n){if(at>=end)return false;n=(unsigned char)src[at++];return true;};
    while(at<end) {
        unsigned char left[256],right[256]={},stack[512];unsigned sp=0,c=0,count;
        for(unsigned i=0;i<256;i++)left[i]=(unsigned char)i;
        if(!take(count))return false;
        for(;;) {
            if(count>127){c+=count-127;count=0;}
            if(c>256)return false;if(c==256)break;
            for(unsigned i=0;i<=count;i++,c++) {
                unsigned a,b;if(c>=256 || !take(a))return false;left[c]=(unsigned char)a;
                if(c!=a){if(!take(b))return false;right[c]=(unsigned char)b;}
            }
            if(c==256)break;if(!take(count))return false;
        }
        unsigned hi,lo;if(!take(hi)||!take(lo))return false;
        size_t stop=at+(hi<<8)+lo;if(stop>end)return false;
        while(sp || at<stop) {
            if(++steps>budget)return false;
            unsigned ch=sp?stack[--sp]:(unsigned char)src[at++];
            if(ch==left[ch]) {if(result.size()==total)return false;result.push_back((char)ch);}
            else {if(sp>510)return false;stack[sp++]=right[ch];stack[sp++]=left[ch];}
        }
    }
    if(result.size()!=total || !((unsigned char)result[2]|((unsigned char)result[3]<<8)))return false;
    out=std::move(result);return true;
}
inline bool load(const std::filesystem::path &root,int donor,std::string &out,std::string &error) {
    if(donor<0 || donor>=250){error="Unknown stock taunt.";return false;}
    std::string source;WarriorNativePack::Pack motions;
    if(!WarriorNativeSources::asset(root,1431+donor*10,source,error) ||
       !WarriorNativePack::decode(source,motions,error))return false;
    if(motions.entries.size()!=440 || !decode(motions.entries[0x185],out)) {
        error="The stock idle taunt is unavailable or invalid.";return false;
    }
    return true;
}
}
