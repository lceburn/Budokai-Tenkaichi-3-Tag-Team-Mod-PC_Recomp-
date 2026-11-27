// Transfer the reference's hair-owned auxiliary branches with a whole head.
// Node 111 belongs to the separate extra-parts category, not the hair category.
#pragma once
#include "warrior_native_texture_bindings.h"
#include "warrior_native_mesh_tail.h"
#include "warrior_native_seams.h"
#include <functional>
#include <map>
namespace WarriorNativeHeadChains {
using WarriorNativeParts::Record;
inline bool owned(const std::vector<Record> &rows,size_t index) {
    if(index>=rows.size() || rows[index].node<71)return false;
    size_t child=index;int parent=rows[index].parent;
    while(parent>=0) {
        if(size_t(parent)>=child)return false;
        if(rows[parent].node<=70)
            return rows[child].node!=111 && rows[parent].node>=46 && rows[parent].node<=48;
        child=size_t(parent);parent=rows[parent].parent;
    }
    return false;
}
struct Field {unsigned offset,unit;};
inline bool chains(const std::string &body,const std::string &donor,
                   const std::set<unsigned> &retained,const std::map<unsigned,unsigned> &mapped,
                   std::string &out,std::string &error) {
    if(body.size()<0x350 || donor.size()<0x350) {error="Incomplete native hair chain parameters.";return false;}
    static const Field a[]={{0,1},{0x10,4},{0x50,4},{0x90,4},{0xd0,1},{0xe0,2},
        {0x100,1},{0x110,4},{0x150,4},{0x190,1},{0x1a0,2},{0x1c0,1},{0x1d0,4},
        {0x210,4},{0x250,1},{0x260,4},{0x2a0,4},{0x2e0,1}};
    static const Field b[]={{0x2f0,1},{0x2f8,1},{0x300,4},{0x320,4},{0x340,1},{0x348,1}};
    auto result=body;std::set<unsigned> assigned;
    auto transfer=[&](const Field *fields,size_t count,unsigned slots) {
        const auto start=fields[0].offset;
        for(unsigned i=0;i<slots;i++) {
            unsigned node=static_cast<unsigned char>(body[start+i]);
            if(node && retained.count(node)) {
                if(!assigned.insert(node).second)return false;
            }else for(size_t k=0;k<count;k++)result.replace(fields[k].offset+i*fields[k].unit,fields[k].unit,fields[k].unit,'\0');
        }
        for(unsigned i=0;i<slots;i++) {
            unsigned original=static_cast<unsigned char>(donor[start+i]);auto mapping=mapped.find(original);
            if(!original || mapping==mapped.end())continue;
            unsigned node=mapping->second;if(node>255 || !assigned.insert(node).second)return false;
            unsigned target=slots;
            for(unsigned j=0;j<slots;j++)if(!result[start+j] && static_cast<unsigned char>(body[start+j])==node){target=j;break;}
            if(target==slots)for(unsigned j=0;j<slots;j++)if(!result[start+j]){target=j;break;}
            if(target==slots)return false;
            for(size_t k=0;k<count;k++)result.replace(fields[k].offset+target*fields[k].unit,fields[k].unit,
                donor,fields[k].offset+i*fields[k].unit,fields[k].unit);
            result[start+target]=char(node);
        }
        return true;
    };
    if(!transfer(a,sizeof(a)/sizeof(*a),16) || !transfer(b,sizeof(b)/sizeof(*b),8)) {
        error="Native hair chains conflict or exceed the model's chain slots.";return false;
    }
    out=std::move(result);error.clear();return true;
}
inline bool apply(const WarriorNativePack::Pack &body,const WarriorNativePack::Pack &donor,
                  WarriorNativePack::Pack &out,std::string &error) {
    if(body.entries.size()<26 || donor.entries.size()<26){error="Incomplete native hair sources.";return false;}
    std::vector<Record> base,source;
    if(!WarriorNativeParts::records(body.entries[2],base,error) || !WarriorNativeParts::records(donor.entries[2],source,error))return false;
    std::set<size_t> retainedRows,importedRows;std::set<unsigned> retainedNodes,used;
    std::map<unsigned,size_t> canonical;
    for(size_t i=0;i<base.size();i++)if(!owned(base,i)) {
        retainedRows.insert(i);retainedNodes.insert(base[i].node);used.insert(base[i].node);
        if(base[i].node>=46 && base[i].node<=48)canonical[base[i].node]=i;
    }
    std::map<unsigned,unsigned> mapping;
    for(size_t i=0;i<source.size();i++)if(owned(source,i)) {
        importedRows.insert(i);unsigned node=source[i].node;
        if(mapping.count(node)){error="Ambiguous native hair branch identity.";return false;}
        if(node>161 || used.count(node)) {
            for(node=72;node<=161 && used.count(node);node++){}
            if(node>161){error="The native hair branches exceed available node identities.";return false;}
        }
        mapping[source[i].node]=node;used.insert(node);
    }
    if(importedRows.empty() && retainedRows.size()==base.size()){out=body;error.clear();return true;}
    std::string textures;
    if(!WarriorNativeTextureBindings::merge(body.entries[11],donor.entries[11],donor.entries[2],importedRows,textures,error,&body))return false;
    std::string bodyMesh=body.entries[2],donorMesh=donor.entries[2];
    if(!WarriorNativeTextureBindings::relocate(bodyMesh,body.entries[11],textures,retainedRows,error) ||
       !WarriorNativeTextureBindings::relocate(donorMesh,donor.entries[11],textures,importedRows,error))return false;
    // Head fitting preserves local shape. Translate each auxiliary branch in
    // the fitted head anchor's frame, including both bind endpoints and bounds.
    for(size_t i:importedRows) {
        int root=source[i].parent;
        while(root>=0 && source[root].node>70)root=source[root].parent;
        if(root<0 || !canonical.count(source[root].node)){error="A native hair branch has no fitted head anchor.";return false;}
        const auto &target=base[canonical.at(source[root].node)];const auto &record=source[i];
        WarriorNativeSeams::Points points;
        if(!WarriorNativeSeams::read(donorMesh,record,points,error))return false;
        for(unsigned k=0;k<3;k++) {
            float delta=WarriorNativeFitting::scalar(bodyMesh,target.offset+16+k*4)-
                WarriorNativeFitting::scalar(donor.entries[2],source[root].offset+16+k*4);
            if(!std::isfinite(delta)){error="Invalid native hair attachment translation.";return false;}
            for(size_t offset:points.offsets) {
                float value=WarriorNativeFitting::scalar(donorMesh,offset+k*4)+delta;
                if(!std::isfinite(value)){error="Invalid native hair vertex.";return false;}
                WarriorNativeFitting::scalar(donorMesh,offset+k*4,value);
            }
            for(unsigned field:{16u,32u,64u})if(field<record.size) {
                float value=WarriorNativeFitting::scalar(donorMesh,record.offset+field+k*4)+delta;
                if(!std::isfinite(value)){error="Invalid native hair bind or bound.";return false;}
                WarriorNativeFitting::scalar(donorMesh,record.offset+field+k*4,value);
            }
        }
    }
    struct Item {size_t index;bool imported;int parent;unsigned node,depth;};
    std::vector<Item> items;
    std::function<bool(size_t,int,unsigned)> addDonor,addBase;
    addDonor=[&](size_t index,int parent,unsigned depth) {
        if(depth>=64 || items.size()>=640)return false;
        int at=int(items.size());items.push_back({index,true,parent,mapping.at(source[index].node),depth});
        for(size_t i=index+1;i<source.size();i++)if(source[i].parent==int(index) && importedRows.count(i))
            if(!addDonor(i,at,depth+1))return false;
        return true;
    };
    addBase=[&](size_t index,int parent,unsigned depth) {
        if(depth>=64 || items.size()>=640)return false;
        int at=int(items.size());items.push_back({index,false,parent,base[index].node,depth});
        for(size_t i=index+1;i<base.size();i++)if(base[i].parent==int(index) && retainedRows.count(i))
            if(!addBase(i,at,depth+1))return false;
        if(base[index].node>=46 && base[index].node<=48)
            for(size_t i:importedRows)if(source[i].parent>=0 && source[source[i].parent].node==base[index].node)
                if(!addDonor(i,at,depth+1))return false;
        return true;
    };
    for(size_t i=0;i<base.size();i++)if(base[i].parent<0 && retainedRows.count(i))
        if(!addBase(i,-1,0)){error="The native hair hierarchy exceeds its limits.";return false;}
    if(items.size()!=retainedRows.size()+importedRows.size()){error="A native hair branch became orphaned.";return false;}
    std::string mesh=bodyMesh.substr(0,base.front().offset);
    std::vector<WarriorNativeTextureBindings::Texture> original,combined;
    if(!WarriorNativeTextureBindings::textures(body.entries[11],original,error) ||
       !WarriorNativeTextureBindings::textures(textures,combined,error))return false;
    unsigned normal=0,shades=0;for(const auto &t:combined)if(t.psm==0)shades++;else if(!t.auxiliary)normal++;
    mesh[0x4e]=0;mesh[0x4f]=char(normal);mesh[0x50]=char(normal);mesh[0x51]=char(shades);
    if(!WarriorNativeTextureBindings::fade_header(mesh,bodyMesh,body.entries[11],textures,error))return false;
    for(unsigned field:{0x4cu,0x54u}) {
        unsigned index=static_cast<unsigned char>(bodyMesh[field]);if(field==0x54 && !index)continue;
        if(index>=original.size()){error="Invalid native hair face material index.";return false;}
        auto target=WarriorNativeTextureBindings::destination(original[index],combined);
        if(!target || (field==0x54 && target==combined.data())){error="Missing native hair face material.";return false;}
        mesh[field]=char(target-combined.data());
    }
    for(size_t i=0;i<items.size();i++) {
        const auto &item=items[i];const auto &record=item.imported?source[item.index]:base[item.index];
        const auto &data=item.imported?donorMesh:bodyMesh;auto bytes=data.substr(record.offset,record.size);
        unsigned next=i+1<items.size()?items[i+1].depth:0;
        if(next>item.depth+1){error="Invalid native hair hierarchy traversal.";return false;}
        unsigned pop=item.depth+1-next;bytes[4]=char(pop);bytes[5]=char(pop>>8);
        bytes[6]=char(i+1==items.size());bytes[7]=0;bytes[10]=char(item.node);bytes[11]=char(item.node>>8);
        if(bytes.size()>WarriorNativePack::Limit-mesh.size()){error="The native hair mesh is too large.";return false;}
        mesh+=bytes;
    }
    std::vector<Record> checked;
    if(!WarriorNativeParts::records(mesh,checked,error) || checked.size()!=items.size() ||
       !WarriorNativeMeshTail::append(mesh,body.entries[2],body.entries[11],textures,error))return false;
    auto result=body;
    if(!chains(body.entries[22],donor.entries[22],retainedNodes,mapping,result.entries[22],error))return false;
    result.entries[2]=std::move(mesh);result.entries[11]=std::move(textures);
    for(unsigned i=3;i<11;i++)if(!WarriorNativeTextureBindings::relocate_stream(result.entries[i],body.entries[11],result.entries[11],error))return false;
    out=std::move(result);error.clear();return true;
}
}
