// Relocate existing VIF material addresses without changing vertex data.
#pragma once
#include "warrior_native_parts.h"
#include <set>
#include <algorithm>
#include <cmath>
#include <cstring>
namespace WarriorNativeTextureBindings {
struct Texture {
    unsigned tbp,cbp,psm;uint64_t format;
    std::string pixels,palette;
    std::string header,pixelUpload,paletteUpload;
};
inline uint64_t wide(const std::string &data,size_t at) {
    return WarriorNativePack::word(data,at)|(uint64_t(WarriorNativePack::word(data,at+4))<<32);
}
inline bool textures(const std::string &data,std::vector<Texture> &out,std::string &error) {
    if(data.size()<8){error="Missing native texture table.";return false;}
    auto count=WarriorNativePack::word(data,0);size_t table=size_t(WarriorNativePack::word(data,4))*4;
    if(!count || count>128 || table<8 || table>data.size() || count*64>data.size()-table) {
        error="Invalid native texture table.";return false;
    }
    std::vector<Texture> result;
    for(unsigned i=0;i<count;i++) {
        size_t e=table+i*64,p=size_t(WarriorNativePack::word(data,e))*4,c=size_t(WarriorNativePack::word(data,e+4))*4;
        auto bytes=WarriorNativePack::word(data,e+8),colours=WarriorNativePack::word(data,e+12);
        if(!p || bytes<96 || p>data.size() || bytes>data.size()-p ||
           (c && (colours<96 || c>data.size() || colours>data.size()-c)) || (!c && colours)) {
            error="A native texture upload is outside its resource.";return false;
        }
        auto tex0=wide(data,e+48);
        Texture t{WarriorNativePack::word(data,e+32),WarriorNativePack::word(data,e+36),unsigned((tex0>>20)&63),
            tex0&~(UINT64_C(0x3fff)|(UINT64_C(0x3fff)<<37)),data.substr(p+96,bytes-96),c?data.substr(c+96,colours-96):std::string(),
            data.substr(e,64),data.substr(p,bytes),c?data.substr(c,colours):std::string()};
        if(t.tbp>0x3fff || t.cbp>0x3fff){error="Native texture addresses exceed GS fields.";return false;}
        result.push_back(std::move(t));
    }
    out=std::move(result);return true;
}
inline const Texture *destination(const Texture &source,const std::vector<Texture> &target) {
    const Texture *found=nullptr;
    for(const auto &t:target)if(t.psm==source.psm && t.format==source.format && t.pixels==source.pixels && t.palette==source.palette) {
        if(found && (found->tbp!=t.tbp || found->cbp!=t.cbp))return nullptr;
        found=&t;
    }
    return found;
}
// Some BT4 costumes bind one upload's pixels with another upload's palette.
// Keep the exact-pair behavior for ordinary models, then resolve that explicit
// cross binding without guessing a substitute texture.
inline bool material(const std::vector<Texture> &source,unsigned tbp,unsigned cbp,Texture &out) {
    const Texture *direct=nullptr,*pixels=nullptr,*palette=nullptr;
    for(const auto &t:source)if(t.psm!=0) {
        auto extent=WarriorNativePack::word(t.header,16);
        if(t.tbp<=0x3fff && extent && extent<=0x4000-t.tbp && tbp>=t.tbp && tbp-t.tbp<extent &&
           (!pixels || t.tbp>=pixels->tbp))pixels=&t;
        if(t.cbp==cbp)palette=&t;
        if(t.tbp==tbp && t.cbp==cbp)direct=&t;
    }
    if(direct){out=*direct;return true;}
    if(!pixels || !palette || palette->palette.empty() ||
       pixels->psm!=palette->psm || pixels->palette.size()!=palette->palette.size() ||
       ((pixels->format>>51)&15)!=((palette->format>>51)&15))return false;
    out=*pixels;out.cbp=cbp;out.palette=palette->palette;out.paletteUpload=palette->paletteUpload;
    WarriorNativePack::put(out.header,12,unsigned(out.paletteUpload.size()));
    WarriorNativePack::put(out.header,36,cbp);return true;
}
inline bool nonrendering_material(const std::string &mesh,size_t at,size_t vertices) {
    if(vertices<3 || at>mesh.size() || mesh.size()-at<88 || vertices>(mesh.size()-at-88)/48)return false;
    for(unsigned k=0;k<4;k++) {
        float value;std::memcpy(&value,mesh.data()+at+88+k*4,4);
        if(!std::isfinite(value))return false;
    }
    for(size_t i=1;i<vertices;i++)if(std::memcmp(mesh.data()+at+88,mesh.data()+at+88+i*48,16))return false;
    return true;
}
inline bool relocate(std::string &mesh,const std::string &source,const std::string &target,
                     const std::set<size_t> &selected,std::string &error) {
    std::vector<WarriorNativeParts::Record> records;std::vector<Texture> from,to;
    if(!WarriorNativeParts::records(mesh,records,error) || !textures(source,from,error) || !textures(target,to,error))return false;
    for(auto index:selected)if(index>=records.size()){error="Invalid native texture record selection.";return false;}
    std::string next=mesh;
    for(auto index:selected) {
        auto r=records[index];size_t end=r.offset+r.size;
        if(r.size<112)continue;
        for(size_t dma=r.offset+96;dma+16<=end;) {
            auto tag=WarriorNativePack::word(mesh,dma);size_t qwc=tag&0xffff,bytes=qwc*16;
            if(bytes>end-dma-16){error="A native material DMA packet is truncated.";return false;}
            size_t words=qwc*4+2,begin=dma+8;
            for(size_t j=0;j+23<=words;) {
                size_t at=begin+j*4;
                if(WarriorNativePack::word(mesh,at)!=0x6c058000)break;
                size_t vertices=WarriorNativePack::word(mesh,at+52)&0x7fff;
                if(vertices>(words-j-23)/12){error="A native material vertex packet is truncated.";return false;}
                auto low=WarriorNativePack::word(mesh,at+20),high=WarriorNativePack::word(mesh,at+24);
                auto shadow=WarriorNativePack::word(mesh,at+40);
                const Texture *normal=nullptr,*shade=nullptr;
                for(const auto &t:from) {
                    if(t.psm!=0 && t.tbp==(low&0x3fff) && t.cbp==((high>>5)&0x3fff))normal=&t;
                    if(t.psm==0 && t.tbp==((shadow>>5)&0x3fff))shade=&t;
                }
                Texture cross;
                if(!normal && material(from,low&0x3fff,(high>>5)&0x3fff,cross))normal=&cross;
                auto normalTo=normal?destination(*normal,to):nullptr,shadeTo=shade?destination(*shade,to):nullptr;
                if(!normalTo || !shadeTo) {
                    // B14 includes attribution bytes in a coincident-vertex
                    // dummy triangle's material fields. Preserve those bytes;
                    // they are not texture addresses used by visible geometry.
                    if(nonrendering_material(mesh,at,vertices)){j+=23+vertices*12;continue;}
                    error="A native material has no unambiguous destination texture.";return false;
                }
                unsigned offset=(low&0x3fff)-normal->tbp;
                if(offset>0x3fff-normalTo->tbp){error="A native texture window exceeds GS addresses.";return false;}
                WarriorNativePack::put(next,at+20,(low&~0x3fffu)|(normalTo->tbp+offset));
                WarriorNativePack::put(next,at+24,(high&~0x7ffe0u)|(normalTo->cbp<<5));
                WarriorNativePack::put(next,at+40,(shadow&~0x7ffe0u)|(shadeTo->tbp<<5));
                j+=23+vertices*12;
            }
            dma+=16+bytes;
            if((tag&0xf0000000)==0x60000000)break;
        }
    }
    mesh=std::move(next);error.clear();return true;
}
inline bool used_textures(const std::string &mesh,const std::vector<Texture> &from,const std::set<size_t> &selected,
                          std::vector<Texture> &used,std::string &error) {
    std::vector<WarriorNativeParts::Record> records;
    if(!WarriorNativeParts::records(mesh,records,error))return false;
    std::set<size_t> result;std::vector<Texture> crossBindings;
    for(auto index:selected) {
        if(index>=records.size()){error="Invalid donor texture record.";return false;}
        auto r=records[index];if(r.size<112)continue;size_t end=r.offset+r.size;
        for(size_t dma=r.offset+96;dma+16<=end;) {
            auto tag=WarriorNativePack::word(mesh,dma);size_t qwc=tag&0xffff,bytes=qwc*16;
            if(bytes>end-dma-16){error="Truncated donor material packet.";return false;}
            for(size_t j=0,words=qwc*4+2;j+23<=words;) {
                auto at=dma+8+j*4;if(WarriorNativePack::word(mesh,at)!=0x6c058000)break;
                size_t vertices=WarriorNativePack::word(mesh,at+52)&0x7fff;
                if(vertices>(words-j-23)/12){error="Truncated donor vertices.";return false;}
                auto low=WarriorNativePack::word(mesh,at+20),high=WarriorNativePack::word(mesh,at+24),shadow=WarriorNativePack::word(mesh,at+40);
                bool normal=false,shade=false;
                for(size_t i=0;i<from.size();i++) {
                    auto &t=from[i];
                    if(t.psm!=0 && t.tbp==(low&0x3fff) && t.cbp==((high>>5)&0x3fff)){result.insert(i);normal=true;}
                    if(t.psm==0 && t.tbp==((shadow>>5)&0x3fff)){result.insert(i);shade=true;}
                }
                if(!normal) {
                    Texture cross;
                    if(material(from,low&0x3fff,(high>>5)&0x3fff,cross)) {
                        normal=true;bool found=false;
                        for(size_t i=0;i<from.size();i++)if(from[i].psm==cross.psm && from[i].format==cross.format &&
                            from[i].pixels==cross.pixels && from[i].palette==cross.palette){result.insert(i);found=true;}
                        if(!found) {
                            for(const auto &t:crossBindings)if(t.psm==cross.psm && t.format==cross.format && t.pixels==cross.pixels && t.palette==cross.palette)found=true;
                            if(!found)crossBindings.push_back(std::move(cross));
                            if(crossBindings.size()>128){error="Too many native palette bindings.";return false;}
                        }
                    }
                }
                if(!normal || !shade) {
                    if(nonrendering_material(mesh,at,vertices)){j+=23+vertices*12;continue;}
                    error="A donor material has no source texture.";return false;
                }
                j+=23+vertices*12;
            }
            dma+=16+bytes;if((tag&0xf0000000)==0x60000000)break;
        }
    }
    std::vector<Texture> all;for(auto i:result)all.push_back(from[i]);
    all.insert(all.end(),crossBindings.begin(),crossBindings.end());used=std::move(all);return true;
}
inline bool relocate_stream(std::string &stream,const std::string &source,const std::string &target,std::string &error) {
    if(stream.empty())return true;
    if(stream.size()>WarriorNativePack::Limit-0xf0){error="The native facial stream is too large.";return false;}
    // Mouth streams use the same DMA/VIF format as mesh record payloads.
    // Wrap one temporarily to share all packet/texture validation rules.
    std::string wrapper(0xf0,'\0');WarriorNativePack::put(wrapper,0,0x6c646d70);
    WarriorNativePack::put(wrapper,0x6c,0x90);WarriorNativePack::put(wrapper,0x90,unsigned(stream.size()+96));
    wrapper[0x96]=1;wrapper+=stream;
    if(!relocate(wrapper,source,target,{0},error))return false;
    stream=wrapper.substr(0xf0);return true;
}
inline bool merge(const std::string &bodyData,const std::string &donorData,const std::string &donorMesh,
                  const std::set<size_t> &selected,std::string &out,std::string &error,
                  const WarriorNativePack::Pack *bodyModel=nullptr,
                  const WarriorNativePack::Pack *donorFace=nullptr) {
    std::vector<Texture> body,donor;
    if(!textures(bodyData,body,error) || !textures(donorData,donor,error))return false;
    std::vector<Texture> used;if(!used_textures(donorMesh,donor,selected,used,error))return false;
    // Stock models can contain identical uploads at different addresses.
    // Canonicalize those before assigning new addresses, so content-based
    // rebinding has one destination for both retained and imported parts.
    std::vector<Texture> combined;
    auto append=[&](const Texture &source) {
        for(const auto &t:combined)if(t.psm==source.psm && t.format==source.format &&
            t.pixels==source.pixels && t.palette==source.palette)return;
        combined.push_back(source);
    };
    for(const auto &t:body)append(t);
    if(bodyModel) {
        if(bodyModel->entries.size()<12){error="Missing native body texture sources.";return false;}
        std::vector<WarriorNativeParts::Record> rows;
        if(!WarriorNativeParts::records(bodyModel->entries[2],rows,error))return false;
        std::set<size_t> all;for(size_t i=0;i<rows.size();i++)all.insert(i);
        std::vector<Texture> needed;
        if(!used_textures(bodyModel->entries[2],body,all,needed,error))return false;
        for(const auto &t:needed)append(t);
    }
    // A facial stream may use an atlas pixel/palette pair that does not occur
    // in the visible head mesh. Retained and imported faces both own bindings.
    for(const auto *model:{bodyModel,donorFace})if(model) {
        if(model->entries.size()<12){error="Missing native facial texture sources.";return false;}
        const auto &source=model==bodyModel?body:donor;
        std::vector<Texture> needed;
        for(unsigned i=3;i<11;i++)if(!model->entries[i].empty()) {
            const auto &stream=model->entries[i];
            if(stream.size()>WarriorNativePack::Limit-0xf0){error="The native facial texture stream is too large.";return false;}
            std::string wrapper(0xf0,'\0');WarriorNativePack::put(wrapper,0,0x6c646d70);
            WarriorNativePack::put(wrapper,0x6c,0x90);WarriorNativePack::put(wrapper,0x90,unsigned(stream.size()+96));
            wrapper[0x96]=1;wrapper+=stream;
            if(!used_textures(wrapper,source,{0},needed,error))return false;
            for(const auto &t:needed)append(t);
        }
    }
    for(const auto &t:used)append(t);
    if(combined.size()>128){error="The combined texture table exceeds the model limit.";return false;}
    // Indexed textures and shading uploads have separate address spaces.
    std::stable_sort(combined.begin(),combined.end(),[](const Texture &a,const Texture &b){return (a.psm!=0)>(b.psm!=0);});
    std::string result(32+combined.size()*64,'\0');result.replace(0,32,bodyData.substr(0,32));
    WarriorNativePack::put(result,0,unsigned(combined.size()));WarriorNativePack::put(result,4,8);
    unsigned normal=0,shade=0,palette=0;
    for(size_t i=0;i<combined.size();i++) {
        auto t=combined[i];size_t e=32+i*64;
        if(t.psm!=0 && t.psm!=19 && t.psm!=20){error="This native texture pixel format is not supported yet.";return false;}
        unsigned &cursor=t.psm==0?shade:normal;auto extent=WarriorNativePack::word(t.header,16);
        if(cursor>0x3fff || !extent || extent>0x4000-cursor || palette>0x3fff) {
            error="The combined texture addresses exceed GS limits.";return false;
        }
        result.replace(e,64,t.header);WarriorNativePack::put(result,e,unsigned(result.size()/4));
        result+=t.pixelUpload;
        if(!t.paletteUpload.empty()) {
            WarriorNativePack::put(result,e+4,unsigned(result.size()/4));result+=t.paletteUpload;
        }else WarriorNativePack::put(result,e+4,0);
        WarriorNativePack::put(result,e+32,cursor);WarriorNativePack::put(result,e+36,t.psm==0?0:palette++);cursor+=extent;
        if(result.size()>WarriorNativePack::Limit){error="The combined texture resource is too large.";return false;}
    }
    std::vector<Texture> checked;if(!textures(result,checked,error))return false;
    out=std::move(result);return true;
}
}
