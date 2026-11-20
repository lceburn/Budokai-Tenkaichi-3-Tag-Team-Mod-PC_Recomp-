// Verified face-anchor fitting. Neck/torso stitching is a separate operation.
#pragma once
#include "warrior_native_parts.h"
#include <cmath>
#include <cstring>
#include <utility>
namespace WarriorNativeFitting {
using Vec3=std::array<float,3>;
struct Bone {Vec3 origin,axis;float length;};
inline float scalar(const std::string &data,size_t at) {
    auto bits=WarriorNativePack::word(data,at);float value;std::memcpy(&value,&bits,4);return value;
}
inline void scalar(std::string &data,size_t at,float value) {
    uint32_t bits;std::memcpy(&bits,&value,4);WarriorNativePack::put(data,at,bits);
}
inline bool find(const std::vector<WarriorNativeParts::Record> &rows,unsigned node,
                 const WarriorNativeParts::Record *&out,std::string &error) {
    const WarriorNativeParts::Record *found=nullptr;
    for(const auto &r:rows)if(r.node==node){if(found){error="Ambiguous native fitting bone.";return false;}found=&r;}
    if(!found){error="Missing native fitting bone.";return false;}
    out=found;return true;
}
// Several draw records can share one logical skeletal anchor. Resolve that
// anchor only when every bind record is identical; geometry selection remains
// separate and must not silently discard split draws.
inline bool anchor(const std::string &mesh,const std::vector<WarriorNativeParts::Record> &rows,unsigned node,
                   const WarriorNativeParts::Record *&out,std::string &error) {
    const WarriorNativeParts::Record *found=nullptr;
    for(const auto &r:rows)if(r.node==node) {
        if(r.offset>mesh.size() || mesh.size()-r.offset<64 ||
           (found && mesh.compare(found->offset+16,48,mesh,r.offset+16,48)!=0)) {
            error="Conflicting native fitting anchors.";return false;
        }
        if(!found)found=&r;
    }
    if(!found){error="Missing native fitting anchor.";return false;}
    out=found;return true;
}
inline bool bone(const std::string &mesh,unsigned parent,unsigned child,Bone &out,std::string &error) {
    std::vector<WarriorNativeParts::Record> rows;
    if(!WarriorNativeParts::records(mesh,rows,error))return false;
    const WarriorNativeParts::Record *a,*b;
    if(!anchor(mesh,rows,parent,a,error) || !anchor(mesh,rows,child,b,error))return false;
    Bone result;float squared=0;
    for(unsigned k=0;k<3;k++) {
        result.origin[k]=scalar(mesh,a->offset+16+k*4);
        float target=scalar(mesh,b->offset+16+k*4),delta=target-result.origin[k];
        if(!std::isfinite(result.origin[k]) || !std::isfinite(target) || !std::isfinite(delta)) {
            error="Invalid native fitting bone coordinates.";return false;
        }
        result.axis[k]=delta;squared+=delta*delta;
    }
    result.length=std::sqrt(squared);
    if(!std::isfinite(result.length)){error="Native fitting bone length overflow.";return false;}
    if(result.length>.0001f)for(auto &v:result.axis)v/=result.length;
    else result.axis={{0,1,0}};
    out=result;return true;
}
inline bool girth(const std::string &mesh,unsigned node,const Vec3 &axis,float &out,std::string &error) {
    std::vector<WarriorNativeParts::Record> rows;
    if(!WarriorNativeParts::records(mesh,rows,error))return false;
    float norm=0;
    for(float v:axis){if(!std::isfinite(v)){error="Invalid native fitting axis.";return false;}norm+=v*v;}
    if(std::fabs(norm-1.f)>.001f){error="The native fitting axis is not normalized.";return false;}
    // BT4 may split a drawing bone into several records. Its widest component
    // supplies the fitting bound; do not mistake those records for two anchors.
    float result=0;bool found=false;const WarriorNativeParts::Record *anchor=nullptr;
    for(const auto &record:rows)if(record.node==node) {
        if(anchor && mesh.substr(anchor->offset+16,48)!=mesh.substr(record.offset+16,48)) {
            error="Ambiguous native split-bone coordinates.";return false;
        }
        anchor=&record;
        found=true;if(record.size<=64)continue;
        if(record.size<96){error="Truncated native fitting bounds.";return false;}
        float sum=0;
        for(unsigned k=0;k<3;k++) {
            float extent=scalar(mesh,record.offset+80+k*4);
            if(!std::isfinite(extent) || extent<0){error="Invalid native fitting extent.";return false;}
            sum+=(extent*extent)*std::max(0.f,1.f-axis[k]*axis[k]);
        }
        float width=std::sqrt(sum);
        if(!std::isfinite(width)){error="Native fitting width overflow.";return false;}
        result=std::max(result,width);
    }
    if(!found){error="Missing native fitting bone.";return false;}
    out=result;return true;
}
struct Fit {float length=1,width=1;Vec3 axis{{0,1,0}},origin{{0,0,0}};};
// These pairs describe the limb whose longitudinal scale controls a part.
// Finger segments share the final hand bone; head parts use the torso.
inline std::pair<unsigned,unsigned> fitting_bones(unsigned node) {
    static constexpr unsigned parent[]={9,10,10,16,13,14,14,16,16,16,19,20,20,16,16,16,16,16,16,16,16,16,16,16,33,34,34};
    static constexpr unsigned child[]={10,11,11,46,14,15,15,46,46,46,20,21,21,46,46,46,46,46,46,46,46,46,46,46,34,35,35};
    if(node>=22 && node<31)return {20,21};
    if(node>=36 && node<45)return {34,35};
    if(node>=9 && node<=35)return {parent[node-9],child[node-9]};
    return {16,46};
}
inline float length_ratio(float target,float source) {
    return std::max(0x1.333332p-1f,std::min(1.5f,target/source));
}
inline bool fit(const std::string &body,const std::string &donor,unsigned node,Fit &out,std::string &error) {
    auto pair=fitting_bones(node);Bone target,source;
    if(!bone(body,pair.first,pair.second,target,error) ||
       !bone(donor,pair.first,pair.second,source,error))return false;
    Fit result;result.origin=source.origin;
    if(pair.first==16)result.origin[0]=0;
    if(target.length<.001f || source.length<.001f){out=result;error.clear();return true;}
    result.axis=source.axis;result.length=result.width=length_ratio(target.length,source.length);
    Bone torso,baseTorso;
    if(!bone(donor,16,46,torso,error) || !bone(body,16,46,baseTorso,error))return false;
    float baseWidth=0,sourceWidth=0;
    std::vector<WarriorNativeParts::Record> baseRows,sourceRows;
    if(!WarriorNativeParts::records(body,baseRows,error) || !WarriorNativeParts::records(donor,sourceRows,error))return false;
    for(unsigned part:{8u,12u,16u,17u,18u,32u}) {
        // An absent optional torso component contributes zero, as in the reference.
        for(const auto &r:baseRows)if(r.node==part) {
            float width;if(!girth(body,part,torso.axis,width,error))return false;
            baseWidth=std::max(baseWidth,width);break;
        }
        for(const auto &r:sourceRows)if(r.node==part) {
            float width;if(!girth(donor,part,torso.axis,width,error))return false;
            sourceWidth=std::max(sourceWidth,width);break;
        }
    }
    float cap=(baseTorso.length>.001f && torso.length>.001f)?length_ratio(baseTorso.length,torso.length):1.f;
    result.width=cap;
    if(baseWidth>.001f && sourceWidth>.001f)
        result.width=std::max(0x1.199998p-1f,std::min(cap,(baseWidth/sourceWidth)*1.12f));
    out=result;error.clear();return true;
}
inline bool face(std::string &mesh,const std::string &body,std::string &error) {
    std::vector<WarriorNativeParts::Record> source,target;
    if(!WarriorNativeParts::records(mesh,source,error) || !WarriorNativeParts::records(body,target,error))return false;
    const WarriorNativeParts::Record *from=nullptr,*to=nullptr;
    for(const auto &r:source)if(r.node==48){if(from){error="Ambiguous source face anchor.";return false;}from=&r;}
    for(const auto &r:target)if(r.node==48){if(to){error="Ambiguous target face anchor.";return false;}to=&r;}
    if(!from || !to){error="Missing native face anchor.";return false;}
    for(unsigned k=0;k<12;k++)if(!std::isfinite(scalar(mesh,from->offset+16+k*4)) ||
       !std::isfinite(scalar(body,to->offset+16+k*4))) {error="Invalid native face bind data.";return false;}
    float a[3],b[3];bool same=true;
    for(unsigned k=0;k<3;k++) {
        a[k]=scalar(mesh,from->offset+16+k*4);b[k]=scalar(body,to->offset+16+k*4);
        if(!std::isfinite(a[k]) || !std::isfinite(b[k])){error="Invalid native face anchor coordinates.";return false;}
        same&=a[k]==b[k];
    }
    std::string next=mesh;size_t end=from->offset+from->size;
    for(size_t dma=from->offset+96;dma+16<=end;) {
        auto tag=WarriorNativePack::word(mesh,dma);size_t qwc=tag&0xffff,bytes=qwc*16;
        if(bytes>end-dma-16){error="Truncated native face DMA.";return false;}
        for(size_t j=0,words=qwc*4+2;j+23<=words;) {
            size_t at=dma+8+j*4;if(WarriorNativePack::word(mesh,at)!=0x6c058000)break;
            size_t vertices=WarriorNativePack::word(mesh,at+52)&0x7fff;
            if(vertices>(words-j-23)/12){error="Truncated native face vertex data.";return false;}
            for(size_t v=0;v<vertices;v++)for(unsigned k=0;k<3;k++) {
                size_t position=at+88+v*48+k*4;float original=scalar(mesh,position);
                float fitted=same?original:(original-a[k])+b[k];
                if(!std::isfinite(original) || !std::isfinite(fitted)){error="Invalid native face vertex.";return false;}
                scalar(next,position,fitted);
            }
            j+=23+vertices*12;
        }
        dma+=16+bytes;if((tag&0xf0000000)==0x60000000)break;
    }
    // Preserve the body's bind coordinates and translate the drawing bound's
    // center. Extents and nonposition vertex attributes remain unchanged.
    next.replace(from->offset+16,48,body.substr(to->offset+16,48));
    if(from->size>=96)for(unsigned k=0;k<3;k++) {
        auto center=scalar(mesh,from->offset+64+k*4);float moved=same?center:(center-a[k])+b[k];
        if(!std::isfinite(center) || !std::isfinite(moved)){error="Invalid native face bound.";return false;}
        scalar(next,from->offset+64+k*4,moved);
    }
    mesh=std::move(next);error.clear();return true;
}
// Expression draw packets carry the same absolute model-space vertices as
// the resting face. Fit them with the donor face anchor as well, so switching
// mouth/expression shapes does not jump back to the donor body's position.
inline bool face_stream(std::string &stream,const std::string &sourceMesh,
                        const std::string &targetMesh,std::string &error) {
    if(stream.empty())return true;
    if(stream.size()>WarriorNativePack::Limit-0xf0){error="The native expression stream is too large.";return false;}
    std::vector<WarriorNativeParts::Record> rows;
    if(!WarriorNativeParts::records(sourceMesh,rows,error))return false;
    const WarriorNativeParts::Record *anchor=nullptr;
    if(!find(rows,48,anchor,error))return false;
    std::string wrapper(0xf0,'\0');
    WarriorNativePack::put(wrapper,0,0x6c646d70);
    WarriorNativePack::put(wrapper,0x6c,0x90);
    WarriorNativePack::put(wrapper,0x90,unsigned(stream.size()+96));
    wrapper[0x96]=1;wrapper[0x9a]=48;
    wrapper.replace(0xa0,48,sourceMesh.substr(anchor->offset+16,48));
    wrapper+=stream;
    if(!face(wrapper,targetMesh,error))return false;
    stream=wrapper.substr(0xf0);error.clear();return true;
}

}
