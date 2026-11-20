// Preserve the base model's trailing CPU-skinned section after record resizing.
#pragma once
#include "warrior_native_texture_bindings.h"
namespace WarriorNativeMeshTail {
inline bool texture(uint64_t value,const std::vector<WarriorNativeTextureBindings::Texture> &source,
                    const std::vector<WarriorNativeTextureBindings::Texture> &target,uint64_t &out,std::string &error,bool shade=false) {
    unsigned tbp=value&0x3fff,cbp=(value>>37)&0x3fff,psm=(value>>20)&63;
    const WarriorNativeTextureBindings::Texture *from=nullptr;
    for(const auto &t:source) {
        if(shade ? (t.psm==0 && t.tbp==cbp) : (t.psm==psm && t.tbp==tbp && t.cbp==cbp))from=&t;
    }
    WarriorNativeTextureBindings::Texture cross;
    if(!from && !shade && WarriorNativeTextureBindings::material(source,tbp,cbp,cross) && cross.psm==psm)from=&cross;
    auto to=from?WarriorNativeTextureBindings::destination(*from,target):nullptr;
    if(!to){error="Missing native CPU seam material.";return false;}
    // Shading samples a fixed indexed texture with a relocated RGBA ramp as
    // its CLUT, the same arrangement as the second VIF material descriptor.
    if(shade)out=(value&~(UINT64_C(0x3fff)<<37))|(uint64_t(to->tbp)<<37);
    else {
        unsigned offset=tbp-from->tbp;
        if(offset>0x3fff-to->tbp){error="The native CPU texture window exceeds GS addresses.";return false;}
        out=(value&~(UINT64_C(0x3fff)|(UINT64_C(0x3fff)<<37)))|(to->tbp+offset)|(uint64_t(to->cbp)<<37);
    }
    return true;
}
inline bool append(std::string &mesh,const std::string &original,const std::string &sourceTextures,
                   const std::string &targetTextures,std::string &error) {
    std::vector<WarriorNativeParts::Record> oldRows,newRows;
    if(!WarriorNativeParts::records(original,oldRows,error) || !WarriorNativeParts::records(mesh,newRows,error))return false;
    size_t oldEnd=oldRows.back().offset+oldRows.back().size,newEnd=newRows.back().offset+newRows.back().size;
    if(newEnd!=mesh.size() || original.size()-oldEnd>WarriorNativePack::Limit-mesh.size()) {
        error="Invalid native mesh tail extent.";return false;
    }
    size_t seam=WarriorNativePack::word(original,0x70);
    auto fits=[&](size_t offset,size_t count,size_t stride) {
        return offset>=oldEnd && offset<=original.size() && count<=(original.size()-offset)/stride;
    };
    unsigned vertices=0,triangles=0;size_t vertexOffset=0,triangleOffset=0;
    if(seam) {
        if(!fits(seam,1,16)){error="The native CPU seam section is outside its model.";return false;}
        vertices=WarriorNativePack::word(original,seam);triangles=WarriorNativePack::word(original,seam+4);
        vertexOffset=WarriorNativePack::word(original,seam+8);triangleOffset=WarriorNativePack::word(original,seam+12);
        if(vertices>4096 || triangles>8192 || !fits(vertexOffset,vertices,112) || !fits(triangleOffset,triangles,32)) {
            error="Invalid native CPU seam arrays.";return false;
        }
        for(unsigned i=0;i<vertices;i++)if(WarriorNativePack::word(original,vertexOffset+i*112+28)>65535) {
            error="Invalid native CPU seam node.";return false;
        }
        for(unsigned i=0;i<triangles;i++)for(unsigned k=0;k<3;k++)
            if(WarriorNativePack::word(original,triangleOffset+i*32+k*4)>=vertices) {
                error="Invalid native CPU seam triangle.";return false;
            }
    }
    std::string result=mesh;result+=original.substr(oldEnd);
    if(seam) {
        size_t nextSeam=newEnd+(seam-oldEnd),nextVertices=newEnd+(vertexOffset-oldEnd),nextTriangles=newEnd+(triangleOffset-oldEnd);
        WarriorNativePack::put(result,0x70,unsigned(nextSeam));
        WarriorNativePack::put(result,nextSeam+8,unsigned(nextVertices));
        WarriorNativePack::put(result,nextSeam+12,unsigned(nextTriangles));
        if(triangles) {
            std::vector<WarriorNativeTextureBindings::Texture> from,to;
            if(!WarriorNativeTextureBindings::textures(sourceTextures,from,error) ||
               !WarriorNativeTextureBindings::textures(targetTextures,to,error))return false;
            // The renderer uses the first triangle's two material descriptors.
            for(unsigned field:{16u,24u}) {
                uint64_t relocated;
                if(!texture(WarriorNativeTextureBindings::wide(original,triangleOffset+field),from,to,relocated,error,field==24))return false;
                WarriorNativePack::put(result,nextTriangles+field,uint32_t(relocated));
                WarriorNativePack::put(result,nextTriangles+field+4,uint32_t(relocated>>32));
            }
        }
    }else WarriorNativePack::put(result,0x70,0);
    mesh=std::move(result);error.clear();return true;
}
}
