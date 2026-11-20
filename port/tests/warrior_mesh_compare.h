#pragma once
#include "../src/gs/warrior_native_seams.h"
#include <cassert>
#include <set>
// Reference composition can reorder retained strips. Compare their oriented
// triangles, including weights, normals and texture coordinates, not array order.
namespace WarriorMeshCompare {
using Vertex=std::array<uint32_t,12>;
using Triangle=std::array<unsigned,3>;
inline bool equivalent(const Vertex &a,const Vertex &b) {
    for(unsigned k=0;k<12;k++) {
        float x,y;std::memcpy(&x,&a[k],4);std::memcpy(&y,&b[k],4);
        if(std::isfinite(x) && std::isfinite(y)) {if(std::fabs(x-y)>.00001f)return false;}
        else if(a[k]!=b[k])return false;
    }
    return true;
}
inline bool ids(const std::string &mesh,const WarriorNativeSeams::Points &points,
                std::vector<Vertex> &catalogue,std::vector<unsigned> &out,bool add) {
    for(auto at:points.offsets) {
        Vertex v;for(unsigned k=0;k<12;k++)v[k]=WarriorNativePack::word(mesh,at+k*4);
        unsigned index=0;for(;index<catalogue.size();index++)if(equivalent(v,catalogue[index]))break;
        if(index==catalogue.size()) {if(!add)return false;catalogue.push_back(v);}
        out.push_back(index);
    }
    return true;
}
inline std::multiset<Triangle> triangles(const std::string &mesh,const WarriorNativeParts::Record &r,
                                       const std::vector<unsigned> &indices,const std::vector<Vertex> &catalogue) {
    std::multiset<Triangle> result;unsigned cursor=0;
    auto value=[&](unsigned id,unsigned k){float f;auto bits=catalogue[id][k];std::memcpy(&f,&bits,4);return f;};
    for(size_t dma=r.offset+96;dma+16<=r.offset+r.size;) {
        auto tag=WarriorNativePack::word(mesh,dma);size_t bytes=(tag&0xffff)*16,words=bytes/4+2;
        for(size_t j=0;j+23<=words;) {
            auto at=dma+8+j*4;if(WarriorNativePack::word(mesh,at)!=0x6c058000)break;
            unsigned count=WarriorNativePack::word(mesh,at+52)&0x7fff;assert(cursor+count<=indices.size());
            for(unsigned v=2;v<count;v++) {
                Triangle t{{indices[cursor+v-2],indices[cursor+v-1],indices[cursor+v]}};
                if(v&1)std::swap(t[0],t[1]);
                std::array<float,3> a,b,c;
                for(unsigned k=0;k<3;k++){a[k]=value(t[1],k)-value(t[0],k);b[k]=value(t[2],k)-value(t[0],k);}
                c={{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]}};
                if(c[0]*c[0]+c[1]*c[1]+c[2]*c[2]<1e-12f)continue;
                Triangle u{{t[1],t[2],t[0]}},w{{t[2],t[0],t[1]}};result.insert(std::min(t,std::min(u,w)));
            }
            cursor+=count;j+=23+count*12;
        }
        dma+=16+bytes;if((tag&0xf0000000)==0x60000000)break;
    }
    assert(cursor==indices.size());return result;
}
inline bool retained(const std::string &a,const WarriorNativeParts::Record &ar,const WarriorNativeSeams::Points &ap,
                     const std::string &b,const WarriorNativeParts::Record &br,const WarriorNativeSeams::Points &bp) {
    std::vector<Vertex> catalogue;std::vector<unsigned> actual,expected;
    if(!ids(b,bp,catalogue,expected,true) || !ids(a,ap,catalogue,actual,false))return false;
    return triangles(a,ar,actual,catalogue)==triangles(b,br,expected,catalogue);
}
}
