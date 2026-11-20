// Read material identities and displayed colours directly from model uploads.
#pragma once
#include "warrior_native_pack.h"
#include <algorithm>
#include <map>
#include <set>
#include <cmath>

namespace WarriorNativeMaterials {
struct Material {uint32_t key;int texture,r=255,g=255,b=255;};
static const int block[4][8]={{0,1,4,5,16,17,20,21},{2,3,6,7,18,19,22,23},{8,9,12,13,24,25,28,29},{10,11,14,15,26,27,30,31}};
static const int col[8][8]={{0,1,4,5,8,9,12,13},{2,3,6,7,10,11,14,15},{16,17,20,21,24,25,28,29},{18,19,22,23,26,27,30,31},{32,33,36,37,40,41,44,45},{34,35,38,39,42,43,46,47},{48,49,52,53,56,57,60,61},{50,51,54,55,58,59,62,63}};
static const int col8[4][16]={{0,4,16,20,32,36,48,52,2,6,18,22,34,38,50,54},{8,12,24,28,40,44,56,60,10,14,26,30,42,46,58,62},{33,37,49,53,1,5,17,21,35,39,51,55,3,7,19,23},{41,45,57,61,9,13,25,29,43,47,59,63,11,15,27,31}};
static const int block16[8][4]={{0,2,8,10},{1,3,9,11},{4,6,12,14},{5,7,13,15},{16,18,24,26},{17,19,25,27},{20,22,28,30},{21,23,29,31}};
static const int col4[4][32]={
 {0,8,32,40,64,72,96,104,2,10,34,42,66,74,98,106,4,12,36,44,68,76,100,108,6,14,38,46,70,78,102,110},
 {16,24,48,56,80,88,112,120,18,26,50,58,82,90,114,122,20,28,52,60,84,92,116,124,22,30,54,62,86,94,118,126},
 {65,73,97,105,1,9,33,41,67,75,99,107,3,11,35,43,69,77,101,109,5,13,37,45,71,79,103,111,7,15,39,47},
 {81,89,113,121,17,25,49,57,83,91,115,123,19,27,51,59,85,93,117,125,21,29,53,61,87,95,119,127,23,31,55,63}};
inline uint64_t wide(const std::string &data,size_t at) {
    return WarriorNativePack::word(data,at)|uint64_t(WarriorNativePack::word(data,at+4))<<32;
}
inline uint32_t identity(const std::string &data,size_t start,size_t end) {
    uint32_t h=0x811c9dc5;
    for(size_t i=start;i<end;i++)h=(h^static_cast<unsigned char>(data[i]))*0x1000193u;
    auto folded=(h&0xffffff)^(h>>24);return 0x40000010u|((folded<<5)&0x1fffffe0u);
}
inline bool dominant(const std::string &data,size_t e,Material &result) {
    auto tex0=wide(data,e+48);unsigned w=1u<<((tex0>>26)&15),h=1u<<((tex0>>30)&15),psm=(tex0>>20)&63;
    if(w>512 || h>512 || (psm!=19 && psm!=20) || WarriorNativePack::word(data,e+24)>>8)return false;
    size_t pixels=size_t(WarriorNativePack::word(data,e))*4+96,palette=size_t(WarriorNativePack::word(data,e+4))*4+96;
    size_t pixelEnd=pixels-96+WarriorNativePack::word(data,e+8),paletteEnd=palette-96+WarriorNativePack::word(data,e+12);
    if(pixels>pixelEnd || palette>paletteEnd || pixelEnd>data.size() || paletteEnd>data.size())return false;
    auto upload=wide(data,pixels-48);unsigned uw=upload&4095,uh=(upload>>32)&4095;
    unsigned dbw=WarriorNativePack::word(data,e+24)&63,tbw=(tex0>>14)&63;
    if(!uw || !uh || uw>512 || uh>512 || size_t(uw)*uh*4>pixelEnd-pixels)return false;
    std::vector<int> memory(512*512*4,-1);
    for(unsigned y=0;y<uh;y++)for(unsigned x=0;x<uw;x++) {
        size_t a=((((y&~31u)*dbw+((x>>1)&~31u)+block[(y>>3)&3][(x>>3)&7])<<6)+col[y&7][x&7])*4;
        if(a+4>memory.size())return false;
        for(unsigned k=0;k<4;k++)memory[a+k]=int((y*uw+x)*4+k);
    }
    std::map<uint32_t,size_t> lookup;std::vector<std::pair<uint32_t,unsigned>> counts;
    for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++) {
        unsigned c=(y&15)>>2,sx=x^(c&1?4:0),shift=0;size_t address;
        if(psm==19)address=((((y>>1)&~31u)*(tbw>>1)+((x>>2)&~31u)+block[(y>>4)&3][(x>>4)&7])<<8)+col8[y&3][sx&15]+c*64;
        else {
            auto a=((((y>>2)&~31u)*(tbw>>1)+((x>>2)&~31u)+block16[(y>>4)&7][(x>>5)&3])<<9)+col4[y&3][sx&31]+c*128;
            address=a>>1;shift=(a&1)*4;
        }
        if(address>=memory.size() || memory[address]<0 || pixels+size_t(memory[address])>=pixelEnd)return false;
        unsigned index=(static_cast<unsigned char>(data[pixels+memory[address]])>>shift)&(psm==19?255:15);
        if(psm==19)index=(index&0xe7)|((index&8)<<1)|((index&16)>>1);
        size_t colour=palette+index*4;if(colour+4>paletteEnd)return false;
        auto p=reinterpret_cast<const unsigned char *>(data.data()+colour);
        if(p[3]<=4 || std::max({p[0],p[1],p[2]})<=20)continue;
        uint32_t rgb=uint32_t(p[0])<<16|uint32_t(p[1])<<8|p[2];
        auto found=lookup.find(rgb);
        if(found==lookup.end()){lookup[rgb]=counts.size();counts.push_back({rgb,1});}
        else counts[found->second].second++;
    }
    unsigned maximum=0;uint32_t winner=0xffffff;
    for(auto &count:counts)if(count.second>maximum){maximum=count.second;winner=count.first;}
    result.r=winner>>16;result.g=(winner>>8)&255;result.b=winner&255;return true;
}
inline bool catalogue(const WarriorNativePack::Pack &model,std::vector<Material> &result,std::string &error) {
    if(model.entries.size()<12){error="The model has no texture resource.";return false;}
    const auto &data=model.entries[11];
    if(data.size()<8){error="The model texture resource is truncated.";return false;}
    auto count=WarriorNativePack::word(data,0);size_t table=size_t(WarriorNativePack::word(data,4))*4;
    if(!count || count>128 || table<8 || table>data.size() || count*64>data.size()-table) {
        error="Invalid model texture table.";return false;
    }
    std::vector<Material> found;std::set<uint32_t> seen;
    for(unsigned i=0;i<count;i++) {
        size_t e=table+i*64;auto tex0=wide(data,e+48);unsigned psm=(tex0>>20)&63;
        size_t pixels=size_t(WarriorNativePack::word(data,e))*4,palette=size_t(WarriorNativePack::word(data,e+4))*4;
        auto bytes=WarriorNativePack::word(data,e+8),palBytes=WarriorNativePack::word(data,e+12);
        if((psm!=19 && psm!=20) || !pixels || !palette)continue;
        if(bytes<96 || pixels>data.size() || bytes>data.size()-pixels || palette>data.size() || palBytes>data.size()-palette) {
            error="A texture upload leaves the model resource.";return false;
        }
        auto key=identity(data,pixels+96,pixels+bytes);if(!seen.insert(key).second)continue;
        Material m{key,int(i)};dominant(data,e,m);found.push_back(m);
    }
    result=std::move(found);return true;
}
inline std::string table(const std::vector<Material> &materials) {
    std::string text="# BT4 material colours v1\n";
    for(const auto &m:materials)text+=std::to_string(m.key)+"\t"+std::to_string(m.texture)+"\t"+std::to_string(m.r)+"\t"+std::to_string(m.g)+"\t"+std::to_string(m.b)+"\n";
    return text;
}
struct Colour {uint32_t key;int r,g,b;};
struct Hsv {float h,s,v;};
inline Hsv hsv(int r,int g,int b) {
    float red=r/255.f,green=g/255.f,blue=b/255.f;
    float high=std::max({red,green,blue}),low=std::min({red,green,blue}),delta=high-low;
    Hsv result{0,high>0?delta/high:0,high};
    if(delta>0) {
        result.h=(high==red?(green-blue)/delta:high==green?(blue-red)/delta+2.f:(red-green)/delta+4.f)/6.f;
        if(result.h<0)result.h+=1.f;
    }
    return result;
}
inline void turn(const Hsv &from,const Hsv &to,char *pixel) {
    auto c=hsv(static_cast<unsigned char>(pixel[0]),static_cast<unsigned char>(pixel[1]),static_cast<unsigned char>(pixel[2]));
    if(from.s<.04f){c.h=to.h;c.s=to.s;}
    else if(c.s>=.04f){c.h+=to.h-from.h;c.s*=to.s/from.s;}
    if(c.v>from.v) {
        if(from.v<.98f)c.v=to.v+(c.v-from.v)*(1.f-to.v)/(1.f-from.v);
    }else if(from.v>.02f)c.v=c.v*to.v/from.v;
    c.h-=std::floor(c.h);c.s=std::max(0.f,std::min(1.f,c.s));c.v=std::max(0.f,std::min(1.f,c.v));
    float h=c.h*6.f,f=h-int(h),p=c.v*(1.f-c.s),q=c.v*(1.f-c.s*f),t=c.v*(1.f-c.s*(1.f-f));
    float rgb[3];
    switch(int(h)%6) {
    case 0:rgb[0]=c.v;rgb[1]=t;rgb[2]=p;break;
    case 1:rgb[0]=q;rgb[1]=c.v;rgb[2]=p;break;
    case 2:rgb[0]=p;rgb[1]=c.v;rgb[2]=t;break;
    case 3:rgb[0]=p;rgb[1]=q;rgb[2]=c.v;break;
    case 4:rgb[0]=t;rgb[1]=p;rgb[2]=c.v;break;
    default:rgb[0]=c.v;rgb[1]=p;rgb[2]=q;break;
    }
    for(unsigned k=0;k<3;k++)pixel[k]=char(int(rgb[k]*255.f+.5f));
}
inline std::vector<size_t> palettes(const std::string &data) {
    std::vector<size_t> found;
    if(data.size()<32)return found;
    auto count=WarriorNativePack::word(data,0);
    for(size_t i=0;i<count && found.size()<64 && i<(data.size()-32)/64;i++) {
        size_t e=32+i*64,p=size_t(WarriorNativePack::word(data,e+4))*4;
        if(WarriorNativePack::word(data,e+12) && ((wide(data,e+48)>>20)&63)==20 &&
           p<=data.size() && data.size()-p>=160)found.push_back(p+96);
    }
    return found;
}
inline void follow_colours(std::string &target,const std::string &before,const std::string &after) {
    auto sources=palettes(before),destinations=palettes(target);
    for(auto dest:destinations)for(unsigned n=0;n<16;n++) {
        size_t at=dest+n*4,nearest=0;int best=301;
        for(auto source:sources)for(unsigned j=0;j<16;j++) {
            size_t p=source+j*4;int distance=0;
            for(unsigned k=0;k<3;k++) {
                int delta=int(static_cast<unsigned char>(target[at+k]))-int(static_cast<unsigned char>(before[p+k]));distance+=delta*delta;
            }
            if(distance<best){best=distance;nearest=p;}
        }
        if(best==301)continue;
        for(unsigned k=0;k<3;k++) {
            int value=int(static_cast<unsigned char>(target[at+k]))+int(static_cast<unsigned char>(after[nearest+k]))-int(static_cast<unsigned char>(before[nearest+k]));
            target[at+k]=char(std::max(0,std::min(255,value)));
        }
    }
}
inline bool recolour(WarriorNativePack::Pack &model,const std::vector<Colour> &colours,std::string &error) {
    if(colours.empty())return true;
    std::vector<Material> checked;
    if(!catalogue(model,checked,error))return false;
    auto data=model.entries[11];auto count=WarriorNativePack::word(data,0);
    size_t start=size_t(WarriorNativePack::word(data,4))*4;
    if(model.entries.size()<3 || model.entries[2].size()<0x52){error="Missing material bindings.";return false;}
    const auto &mesh=model.entries[2];unsigned normal=static_cast<unsigned char>(mesh[0x4f]),shades=static_cast<unsigned char>(mesh[0x51]);
    if(normal>count || shades>16 || shades>count-normal){error="Invalid material binding counts.";return false;}
    std::vector<std::set<unsigned>> shadeFor(count);
    // VIF material blocks contain the base TEX0 and shading CLUT addresses.
    // Require a complete block and bounded vertex payload before reading it.
    for(size_t at=0;at+88<=mesh.size();at+=4)if(WarriorNativePack::word(mesh,at)==0x6c058000) {
        size_t vertices=WarriorNativePack::word(mesh,at+52)&0x7fff;
        if(vertices>(mesh.size()-at-88)/48)continue;
        auto tbp=WarriorNativePack::word(mesh,at+20)&0x3fff;
        auto cbp=(WarriorNativePack::word(mesh,at+24)>>5)&0x3fff;
        auto shadow=(WarriorNativePack::word(mesh,at+40)>>5)&0x3fff;
        for(unsigned i=0;i<normal;i++) {
            auto e=start+i*64;
            if(WarriorNativePack::word(data,e+32)!=tbp || WarriorNativePack::word(data,e+36)!=cbp)continue;
            for(unsigned j=normal;j<normal+shades;j++)if(WarriorNativePack::word(data,start+j*64+32)==shadow)shadeFor[i].insert(j);
        }
    }
    std::set<uint32_t> keys;
    for(const auto &c:colours) {
        if((c.key&0xe000001f)!=0x40000010 || !keys.insert(c.key).second ||
           c.r<0 || c.r>255 || c.g<0 || c.g>255 || c.b<0 || c.b>255) {
            error="Invalid native material colour.";return false;
        }
        std::set<size_t> painted;
        for(unsigned i=0;i<count;i++) {
            size_t e=start+i*64;unsigned psm=(wide(data,e+48)>>20)&63;
            size_t pixels=size_t(WarriorNativePack::word(data,e))*4,palette=size_t(WarriorNativePack::word(data,e+4))*4;
            auto bytes=WarriorNativePack::word(data,e+8),palBytes=WarriorNativePack::word(data,e+12);
            if((psm!=19 && psm!=20) || !pixels || !palette || identity(data,pixels+96,pixels+bytes)!=c.key)continue;
            if(palBytes<160 || palette>data.size() || palBytes>data.size()-palette) {
                error="The material palette is truncated.";return false;
            }
            if(!painted.insert(palette).second)continue;
            Material m{c.key,int(i)};
            if(!dominant(data,e,m)){error="Could not read the native material's base colour.";return false;}
            auto from=hsv(m.r,m.g,m.b),to=hsv(c.r,c.g,c.b);
            for(unsigned n=0;n<16;n++)turn(from,to,data.data()+palette+96+n*4);
            for(auto shadow:shadeFor[i]) {
                size_t s=start+shadow*64,p=size_t(WarriorNativePack::word(data,s))*4;
                auto length=WarriorNativePack::word(data,s+8);
                if(length<1120 || p>data.size() || length>data.size()-p) {
                    error="A shading upload leaves the material resource.";return false;
                }
                for(unsigned n=0;n<256;n++) {
                    char *pixel=data.data()+p+96+n*4;
                    if(!(pixel[0] || pixel[1] || pixel[2]))continue;
                    char difference[3];int old[]={m.r,m.g,m.b},next[]={c.r,c.g,c.b};
                    for(unsigned k=0;k<3;k++)difference[k]=char(std::max(0,old[k]-int(static_cast<unsigned char>(pixel[k]))));
                    turn(from,to,difference);
                    for(unsigned k=0;k<3;k++)pixel[k]=char(std::max(0,next[k]-int(static_cast<unsigned char>(difference[k]))));
                }
            }
        }
    }
    // Keep facial-expression texture palettes consistent with the base face.
    for(size_t i=12;i<17 && i<model.entries.size();i++)follow_colours(model.entries[i],model.entries[11],data);
    model.entries[11]=std::move(data);return true;
}
}
