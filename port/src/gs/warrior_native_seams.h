// Reference-compatible part boundary discovery for native seam handling.
#pragma once
#include "warrior_native_fitting.h"
#include <algorithm>
#include <map>
namespace WarriorNativeSeams {
using WarriorNativeFitting::Vec3;
struct Points {
    std::vector<Vec3> positions;
    std::vector<size_t> offsets;
    std::vector<unsigned> indices;
    std::vector<bool> boundary;
};
inline bool read(const std::string &mesh,const WarriorNativeParts::Record &record,
                 Points &out,std::string &error) {
    if(record.offset>mesh.size() || record.size>mesh.size()-record.offset || record.size<64) {
        error="Invalid native seam record extent.";return false;
    }
    Points result;std::map<std::pair<unsigned,unsigned>,unsigned> edges;
    const size_t end=record.offset+record.size;
    if(record.size>64 && record.size<96){error="Truncated native seam bounds.";return false;}
    for(size_t dma=record.offset+96;dma+16<=end;) {
        auto tag=WarriorNativePack::word(mesh,dma);size_t bytes=(tag&0xffff)*16;
        if(bytes>end-dma-16){error="Truncated native seam DMA.";return false;}
        const size_t words=bytes/4+2;
        for(size_t j=0;j+23<=words;) {
            size_t at=dma+8+j*4;
            if(WarriorNativePack::word(mesh,at)!=0x6c058000)break;
            size_t count=WarriorNativePack::word(mesh,at+52)&0x7fff;
            if(count>(words-j-23)/12){error="Truncated native seam vertices.";return false;}
            if(result.offsets.size()+count>4096){error="Too many native seam vertices.";return false;}
            std::vector<unsigned> strip;
            for(size_t v=0;v<count;v++) {
                size_t offset=at+88+v*48;Vec3 point;
                for(unsigned k=0;k<3;k++) {
                    point[k]=WarriorNativeFitting::scalar(mesh,offset+k*4);
                    if(!std::isfinite(point[k])){error="Invalid native seam vertex.";return false;}
                }
                unsigned index=0;
                for(;index<result.positions.size();index++) {
                    bool equal=true;
                    for(unsigned k=0;k<3;k++)equal&=std::fabs(point[k]-result.positions[index][k])<.002f;
                    if(equal)break;
                }
                if(index==result.positions.size())result.positions.push_back(point);
                strip.push_back(index);result.indices.push_back(index);result.offsets.push_back(offset);
            }
            for(size_t v=2;v<strip.size();v++) {
                unsigned a=strip[v-2],b=strip[v-1],c=strip[v];
                if(a==b || b==c || a==c)continue;
                for(auto edge:{std::minmax(a,b),std::minmax(b,c),std::minmax(a,c)})edges[edge]++;
            }
            j+=23+count*12;
        }
        dma+=16+bytes;if((tag&0xf0000000)==0x60000000)break;
    }
    result.boundary.assign(result.positions.size(),false);
    for(const auto &edge:edges)if(edge.second==1) {
        result.boundary[edge.first.first]=true;result.boundary[edge.first.second]=true;
    }
    out=std::move(result);error.clear();return true;
}
inline bool anchors(std::string &mesh,const std::string &body,unsigned node,std::string &error,float uniform=1.f,
                    const WarriorNativeParts::Record *sourceHint=nullptr) {
    if(!std::isfinite(uniform) || uniform<=0 || uniform>16.f){error="Invalid native part fitting scale.";return false;}
    std::vector<WarriorNativeParts::Record> rows,baseRows;
    if(!WarriorNativeParts::records(mesh,rows,error) || !WarriorNativeParts::records(body,baseRows,error))return false;
    const WarriorNativeParts::Record *source=nullptr,*target;
    if(sourceHint) {
        for(const auto &r:rows)if(r.node==node && r.offset==sourceHint->offset && r.size==sourceHint->size)source=&r;
        if(!source){error="Invalid native source anchor record.";return false;}
    }else if(!WarriorNativeFitting::find(rows,node,source,error))return false;
    if(!WarriorNativeFitting::anchor(body,baseRows,node,target,error))return false;
    bool same=true;
    for(unsigned k=0;k<12;k++) {
        float a=WarriorNativeFitting::scalar(mesh,source->offset+16+k*4);
        float b=WarriorNativeFitting::scalar(body,target->offset+16+k*4);
        if(!std::isfinite(a) || !std::isfinite(b)){error="Invalid native part bind coordinates.";return false;}
        same&=a==b;
    }
    if(same && uniform==1.f){error.clear();return true;}
    Points points;if(!read(mesh,*source,points,error))return false;
    std::string result=mesh;
    for(size_t offset:points.offsets) {
        float weight=WarriorNativeFitting::scalar(mesh,offset+12);
        if(!std::isfinite(weight) || weight<0 || weight>1){error="Invalid native part blend weight.";return false;}
        for(unsigned k=0;k<3;k++) {
            float delta=WarriorNativeFitting::scalar(body,target->offset+16+k*4)-WarriorNativeFitting::scalar(mesh,source->offset+16+k*4);
            float parentDelta=WarriorNativeFitting::scalar(body,target->offset+32+k*4)-WarriorNativeFitting::scalar(mesh,source->offset+32+k*4);
            float value=(WarriorNativeFitting::scalar(mesh,offset+k*4)+weight*delta)+(1-weight)*parentDelta;
            if(uniform!=1.f) {
                float child=WarriorNativeFitting::scalar(mesh,source->offset+16+k*4);
                float parent=WarriorNativeFitting::scalar(mesh,source->offset+32+k*4);
                value+=(uniform-1.f)*(WarriorNativeFitting::scalar(mesh,offset+k*4)-(weight*child+(1-weight)*parent));
            }
            if(!std::isfinite(value)){error="Native part fitting overflow.";return false;}
            WarriorNativeFitting::scalar(result,offset+k*4,value);
        }
    }
    result.replace(source->offset+16,48,body.substr(target->offset+16,48));
    mesh=std::move(result);error.clear();return true;
}
inline float distance_squared(const Vec3 &a,const Vec3 &b) {
    float sum=0;for(unsigned k=0;k<3;k++){float d=a[k]-b[k];sum+=d*d;}return sum;
}
inline bool bounds(std::string &mesh,const WarriorNativeParts::Record &record,std::string &error) {
    Points points;if(!read(mesh,record,points,error))return false;
    if(points.positions.empty()){error.clear();return true;}
    Vec3 low=points.positions.front(),high=low;
    for(const auto &p:points.positions)for(unsigned k=0;k<3;k++) {
        low[k]=std::min(low[k],p[k]);high[k]=std::max(high[k],p[k]);
    }
    std::string result=mesh;
    for(unsigned k=0;k<3;k++) {
        float center=(low[k]+high[k])*.5f,extent=(high[k]-low[k])*.5f;
        if(!std::isfinite(center) || !std::isfinite(extent)){error="Native part bound overflow.";return false;}
        WarriorNativeFitting::scalar(result,record.offset+64+k*4,center);
        WarriorNativeFitting::scalar(result,record.offset+80+k*4,extent);
    }
    mesh=std::move(result);error.clear();return true;
}
inline bool stitch(std::string &mesh,const WarriorNativeParts::Record &part,
                   const WarriorNativeParts::Record &parent,
                   const WarriorNativeParts::Record *sibling,std::string &error) {
    Points a,b,s;
    if(!read(mesh,part,a,error) || !read(mesh,parent,b,error) ||
       (sibling && !read(mesh,*sibling,s,error)))return false;
    std::vector<Vec3> deltas(a.positions.size(),Vec3{{0,0,0}});
    std::vector<bool> matched(a.positions.size(),false);bool any=false;
    for(size_t i=0;i<a.positions.size();i++)if(a.boundary[i]) {
        bool shared=false;
        for(const auto &p:s.positions)if(distance_squared(p,a.positions[i])<.01f){shared=true;break;}
        if(shared)continue;
        float best=.16f;
        for(size_t j=0;j<b.positions.size();j++)if(b.boundary[j]) {
            float distance=distance_squared(a.positions[i],b.positions[j]);
            if(distance<best) {
                best=distance;matched[i]=true;
                for(unsigned k=0;k<3;k++)deltas[i][k]=b.positions[j][k]-a.positions[i][k];
            }
        }
        any|=matched[i];
    }
    if(!any){error.clear();return true;}
    for(size_t i=0;i<a.positions.size();i++)if(!matched[i]) {
        float nearest=.5f,total=0;Vec3 sum{{0,0,0}};
        for(size_t j=0;j<a.positions.size();j++)if(matched[j]) {
            float distance=std::sqrt(distance_squared(a.positions[i],a.positions[j]));
            if(distance>=.5f)continue;
            float weight=1.f/(distance*distance+.0004f);
            for(unsigned k=0;k<3;k++)sum[k]+=weight*deltas[j][k];
            total+=weight;nearest=std::min(nearest,distance);
        }
        if(total>0) {
            float falloff=1.f-(nearest+nearest);falloff*=falloff;
            for(unsigned k=0;k<3;k++)deltas[i][k]=(sum[k]/total)*falloff;
        }
    }
    std::string result=mesh;
    for(size_t v=0;v<a.offsets.size();v++)for(unsigned k=0;k<3;k++) {
        size_t offset=a.offsets[v]+k*4;
        float value=WarriorNativeFitting::scalar(mesh,offset)+deltas[a.indices[v]][k];
        if(!std::isfinite(value)){error="Native seam adjustment overflow.";return false;}
        WarriorNativeFitting::scalar(result,offset,value);
    }
    mesh=std::move(result);error.clear();return true;
}
}
