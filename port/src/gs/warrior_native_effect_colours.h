// Aura/technique colour traversal. Preserve geometry, timing and alpha data.
#pragma once
#include "warrior_native_pack.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
namespace WarriorNativeEffectColours {
using Rgb=std::array<float,3>;
struct Hsv {float h,s,v;};
inline float scalar(const std::string &data,size_t at){auto bits=WarriorNativePack::word(data,at);float value;std::memcpy(&value,&bits,4);return value;}
inline void put(std::string &data,size_t at,float value){uint32_t bits;std::memcpy(&bits,&value,4);WarriorNativePack::put(data,at,bits);}
inline Hsv hsv(const Rgb &c) {
    float high=std::max({c[0],c[1],c[2]}),low=std::min({c[0],c[1],c[2]}),d=high-low;
    Hsv out{0,high>0?d/high:0,high};
    if(d>0) {out.h=high==c[0]?(c[1]-c[2])/d+(c[1]<c[2]?6.f:0.f):high==c[1]?(c[2]-c[0])/d+2.f:(c[0]-c[1])/d+4.f;}
    return out;
}
inline Rgb rgb(const Hsv &c) {
    float chroma=c.v*c.s,x=chroma*(1.f-std::fabs(std::fmod(c.h,2.f)-1.f)),m=c.v-chroma;
    unsigned sector=unsigned(c.h)%6;Rgb out;
    switch(sector){case 0:out={chroma,x,0};break;case 1:out={x,chroma,0};break;case 2:out={0,chroma,x};break;case 3:out={0,x,chroma};break;case 4:out={x,0,chroma};break;default:out={chroma,0,x};break;}
    for(auto &v:out)v+=m;return out;
}
// One effect resource contains a bounded pack of colour ramps, texture uploads
// and V000 vertex draws. Visit only the reference's recognized colour fields.
inline bool visit(std::string &data,const std::function<void(Rgb&)> &apply,std::string &error) {
    WarriorNativePack::Pack pack;if(!WarriorNativePack::decode(data,pack,error))return false;
    for(size_t i=0;i<pack.entries.size();i++) {
        size_t start=WarriorNativePack::word(data,4+i*4)&~3u,end=WarriorNativePack::word(data,8+i*4)&~3u,size=end-start;
        if(pack.flags[i] || !size)continue;
        if(size==0xc0 || size==0x140) {
            bool valid=false;
            for(size_t at=start;at<end;at+=4){float f=scalar(data,at);if(!std::isfinite(f) || (f!=0 && (f<1 || f>255.5f))){valid=false;break;}valid|=f!=0;}
            if(!valid)continue;
            for(size_t at=start;at<end;at+=16) {
                Rgb c{scalar(data,at)/255.f,scalar(data,at+4)/255.f,scalar(data,at+8)/255.f};apply(c);
                for(unsigned k=0;k<3;k++){float f=c[k]*255.f;if(f>0 && f<1)f=1;put(data,at+k*4,f);}
            }
        }else if(size>=32 && data.compare(start,4,"V000")==0) {
            size_t at=WarriorNativePack::word(data,start+8);
            while(at>=32 && at<=size && size-at>=64) {
                if(scalar(data,start+at+12)==1.f && scalar(data,start+at+28)==1.f && scalar(data,start+at+44)==1.f) {
                    size_t p=start+at+48;Rgb c;for(unsigned k=0;k<3;k++)c[k]=static_cast<unsigned char>(data[p+k])/255.f;apply(c);
                    for(unsigned k=0;k<3;k++)data[p+k]=char(std::clamp(int(c[k]*255.f+.5f),0,255));
                }
                at+=64;
            }
        }else if(size>=96) {
            unsigned n=WarriorNativePack::word(data,start);
            if(!n || n>64 || WarriorNativePack::word(data,start+4)!=8 || size_t(n)*64+32>size)continue;
            for(unsigned t=0;t<n;t++) {
                size_t row=start+32+t*64,px=size_t(WarriorNativePack::word(data,row))*4,pal=size_t(WarriorNativePack::word(data,row+4))*4;
                size_t pxSize=WarriorNativePack::word(data,row+8),palSize=WarriorNativePack::word(data,row+12);
                if(px>size || pxSize>size-px || pal>size || palSize>size-pal || palSize<160)continue;
                unsigned count=palSize>=1120?256:16;
                if(size_t(count)*4+96>palSize)continue;
                for(unsigned p=0;p<count;p++) {
                    size_t at=start+pal+96+p*4;Rgb c;for(unsigned k=0;k<3;k++)c[k]=static_cast<unsigned char>(data[at+k])/255.f;apply(c);
                    for(unsigned k=0;k<3;k++)data[at+k]=char(std::clamp(int(c[k]*255.f+.5f),0,255));
                }
            }
        }
    }
    error.clear();return true;
}
inline bool tint(std::string &data,unsigned colour,std::string &error) {
    auto original=data;float sx=0,sy=0;
    if(colour>0xffffff){error="Invalid native effect colour.";return false;}
    auto target=hsv({float(colour&255)/255.f,float((colour>>8)&255)/255.f,float((colour>>16)&255)/255.f});
    if(!visit(data,[&](Rgb &c){auto h=hsv(c);if(h.s>=.29999998f && h.v>=.19999999f){float w=h.s*h.v,angle=h.h*1.0471976f;sx+=w*std::cos(angle);sy+=w*std::sin(angle);}},error)){data=std::move(original);return false;}
    float base=-1;if(sx*sx+sy*sy>1e-6f){base=std::atan2(sy,sx)/1.0471976f;if(base<0)base+=6.f;}
    if(!visit(data,[&](Rgb &c){
        auto h=hsv(c);if(h.s<.12f || h.v<.04f)return;
        bool inverse=false;
        if(base>=0){float gap=std::fabs(h.h-base);if(gap>3)gap=6-gap;if(gap>2){for(auto &v:c)v=1-v;h=hsv(c);inverse=true;}}
        h.h=target.h;h.s*=target.s<.5f?target.s*2:1;h.v*=target.v<.5f?target.v*2:1;
        c=rgb(h);if(inverse)for(auto &v:c)v=1-v;
    },error)){data=std::move(original);return false;}
    return true;
}
inline bool common(std::string &data,unsigned colour,std::string &error) {
    WarriorNativePack::Pack pack;if(!WarriorNativePack::decode(data,pack,error))return false;
    auto result=data;
    for(size_t i=0;i<pack.entries.size();i++) {
        if(i==7)continue; // Loop-camera tracks are not colourable effects.
        if(pack.flags[i] || pack.entries[i].empty())continue;
        WarriorNativePack::Pack nested;std::string ignored;
        if(!WarriorNativePack::decode(pack.entries[i],nested,ignored))continue;
        auto piece=pack.entries[i];if(!tint(piece,colour,error))return false;
        result.replace(WarriorNativePack::word(data,4+i*4)&~3u,piece.size(),piece);
    }
    data=std::move(result);error.clear();return true;
}
}
