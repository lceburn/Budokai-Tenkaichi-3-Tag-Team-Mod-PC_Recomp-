#include "../src/gs/warrior_native_effect_colours.h"
#include "../src/gs/warrior_native_build.h"
#include <cassert>
#include <iostream>
#include <set>
int main(int argc,char **argv) {
    std::cout<<std::unitbuf;assert(argc==6 || argc==7);std::string raw,gold,error;assert(WarriorNativeBuild::read_file(argv[1],raw,true));assert(WarriorNativeBuild::read_file(argv[2],gold,true));
    WarriorNativePack::Pack a,b;assert(WarriorNativePack::decode(raw,a,error) && WarriorNativePack::decode(gold,b,error));
    unsigned slot=std::atoi(argv[3]),hex=std::strtoul(argv[4],nullptr,16),colour=((hex&255)<<16)|(hex&0xff00)|(hex>>16);
    auto before=a;assert(slot<a.entries.size());
    bool ok=std::string(argv[5])=="common"?WarriorNativeEffectColours::common(a.entries[slot],colour,error):WarriorNativeEffectColours::tint(a.entries[slot],colour,error);
    if(!ok){std::cerr<<error<<"\n";return 1;}assert(a.entries[slot].size()==b.entries[slot].size());
    std::set<size_t> floatColours;
    auto collect=[&](const std::string &piece,size_t base) {
        WarriorNativePack::Pack p;if(!WarriorNativePack::decode(piece,p,error))return;
        for(size_t i=0;i<p.entries.size();i++)if(!p.flags[i] && (p.entries[i].size()==192 || p.entries[i].size()==320)) {
            bool valid=false;for(size_t j=0;j<p.entries[i].size();j+=4){float f=WarriorNativeEffectColours::scalar(p.entries[i],j);if(!std::isfinite(f) || (f!=0 && (f<1 || f>255.5f))){valid=false;break;}valid|=f!=0;}
            if(!valid)continue;size_t start=base+(WarriorNativePack::word(piece,4+i*4)&~3u);
            for(size_t j=0;j<p.entries[i].size();j+=16)for(unsigned k=0;k<3;k++)floatColours.insert(start+j+k*4);
        }
    };
    if(std::string(argv[5])=="common") {
        WarriorNativePack::Pack p;assert(WarriorNativePack::decode(before.entries[slot],p,error));
        for(size_t i=0;i<p.entries.size();i++)if(!p.flags[i])collect(p.entries[i],WarriorNativePack::word(before.entries[slot],4+i*4)&~3u);
    }else collect(before.entries[slot],0);
    size_t changed=0,mismatch=0,nonFloatMismatch=0;int maxDelta=0;float maxFloatDelta=0;
    for(size_t i=0;i<a.entries[slot].size();i++){changed+=a.entries[slot][i]!=before.entries[slot][i];if(a.entries[slot][i]!=b.entries[slot][i]){mismatch++;maxDelta=std::max(maxDelta,std::abs(int(static_cast<unsigned char>(a.entries[slot][i]))-int(static_cast<unsigned char>(b.entries[slot][i]))));if(mismatch<4)std::cerr<<"mismatch "<<i<<" native="<<unsigned(static_cast<unsigned char>(a.entries[slot][i]))<<" ref="<<unsigned(static_cast<unsigned char>(b.entries[slot][i]))<<"\n";}}
    std::cout<<"changed="<<changed<<" mismatch="<<mismatch<<" maximum_byte_delta="<<maxDelta<<"\n";
    if(argc==7)assert(WarriorRecipe::write(argv[6],a.entries[slot]));
    for(size_t i=0;i<a.entries[slot].size();i+=4)if(a.entries[slot].substr(i,4)!=b.entries[slot].substr(i,4)) {
        if(floatColours.count(i)){float x=WarriorNativeEffectColours::scalar(a.entries[slot],i),y=WarriorNativeEffectColours::scalar(b.entries[slot],i);assert(std::isfinite(x)&&std::isfinite(y));maxFloatDelta=std::max(maxFloatDelta,std::fabs(x-y));}
        else nonFloatMismatch++;
    }
    std::cout<<"non_float_mismatches="<<nonFloatMismatch<<" float_colour_max_delta="<<maxFloatDelta<<"\n";
    assert(changed && !nonFloatMismatch && maxFloatDelta<.0001f);
    std::string broken="bad",saved=broken;assert(!WarriorNativeEffectColours::common(broken,colour,error) && broken==saved);
    std::cout<<"PASS native effect colour matches complete reference resource and bad input is preserved\n";
}
