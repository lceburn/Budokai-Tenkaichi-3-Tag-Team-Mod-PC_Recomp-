// Assemble selected fitted donor records while retaining the base hierarchy.
#pragma once
#include "warrior_native_texture_bindings.h"
#include "warrior_native_seams.h"
#include "warrior_native_mesh_tail.h"
#include <map>
namespace WarriorNativeAssembly {
inline bool stitch(std::string &mesh,const std::set<unsigned> &nodes,std::string &error,
                   const std::map<unsigned,uint64_t> *origins=nullptr) {
    std::vector<WarriorNativeParts::Record> checked;
    if(!WarriorNativeParts::records(mesh,checked,error))return false;
    // A retained child can meet a replaced parent too. With provenance, visit
    // every record and let source identity decide which boundaries need joining.
    for(const auto &r:checked)if(origins || nodes.count(r.node)) {
        const WarriorNativeParts::Record *parent=r.parent>=0?&checked[r.parent]:nullptr,*sibling=nullptr;
        if(r.node==8 || r.node==12) {
            if(!WarriorNativeFitting::find(checked,16,parent,error))return false;
            const WarriorNativeParts::Record *other;
            if(!WarriorNativeFitting::find(checked,r.node==8?12:8,other,error))return false;
            if(other->size>64)sibling=other;
        }
        while(parent && parent->size<=64)parent=parent->parent>=0?&checked[parent->parent]:nullptr;
        bool sameSource=false;
        if(parent && origins) {
            auto a=origins->find(r.node),b=origins->find(parent->node);
            sameSource=a!=origins->end() && b!=origins->end() && a->second==b->second;
        }
        if(parent && !sameSource && !WarriorNativeSeams::stitch(mesh,r,*parent,sibling,error))return false;
        if(!WarriorNativeSeams::bounds(mesh,r,error))return false;
    }
    return true;
}
inline bool replace(const WarriorNativePack::Pack &body,const WarriorNativePack::Pack &donor,
                    const std::string &fitted,const std::set<unsigned> &nodes,
                    WarriorNativePack::Pack &out,std::string &error,bool stitchParts=false,
                    const std::set<unsigned> *discard=nullptr) {
    if(body.entries.size()<26 || donor.entries.size()<26 || nodes.empty()) {
        error="Incomplete native part assembly input.";return false;
    }
    std::vector<WarriorNativeParts::Record> base,source;
    if(!WarriorNativeParts::records(body.entries[2],base,error) ||
       !WarriorNativeParts::records(fitted,source,error))return false;
    std::set<size_t> retained,imported;std::map<unsigned,size_t> choices,baseChoices;std::set<unsigned> found;
    for(size_t i=0;i<source.size();i++)if(nodes.count(source[i].node)) {
        auto placed=choices.emplace(source[i].node,i);
        if(!placed.second) {
            const auto &prior=source[placed.first->second];
            if(prior.size!=64 || source[i].size!=64 ||
               fitted.substr(prior.offset+16,48)!=fitted.substr(source[i].offset+16,48)) {
                error="Ambiguous native donor part.";return false;
            }
        }
    }
    if(choices.size()!=nodes.size()){error="The native donor is missing selected parts.";return false;}
    for(size_t i=0;i<base.size();i++) {
        if(discard && discard->count(base[i].node))continue;
        if(!nodes.count(base[i].node)){retained.insert(i);continue;}
        if(!found.insert(base[i].node).second) {
            const auto &prior=base[baseChoices.at(base[i].node)];
            if(prior.size!=64 || base[i].size!=64 ||
               body.entries[2].compare(prior.offset+16,48,body.entries[2],base[i].offset+16,48)!=0) {
                error="Ambiguous native base part.";return false;
            }
        }else baseChoices.emplace(base[i].node,i);
        imported.insert(choices.at(base[i].node));
    }
    if(found!=nodes){error="The native base is missing selected parts.";return false;}
    std::string textures,bodyMesh=body.entries[2],donorMesh=fitted;
    if(!WarriorNativeTextureBindings::merge(body.entries[11],donor.entries[11],fitted,imported,textures,error,&body) ||
       !WarriorNativeTextureBindings::relocate(bodyMesh,body.entries[11],textures,retained,error) ||
       !WarriorNativeTextureBindings::relocate(donorMesh,donor.entries[11],textures,imported,error))return false;
    std::vector<WarriorNativeTextureBindings::Texture> original,combined;
    if(!WarriorNativeTextureBindings::textures(body.entries[11],original,error) ||
       !WarriorNativeTextureBindings::textures(textures,combined,error))return false;
    std::string mesh=bodyMesh.substr(0,base.front().offset);unsigned normal=0,shades=0;
    for(const auto &t:combined)if(t.psm==0)shades++;else if(!t.auxiliary)normal++;
    mesh[0x4e]=0;mesh[0x4f]=char(normal);mesh[0x50]=char(normal);mesh[0x51]=char(shades);
    if(!WarriorNativeTextureBindings::fade_header(mesh,bodyMesh,body.entries[11],textures,error))return false;
    unsigned face=static_cast<unsigned char>(bodyMesh[0x4c]);
    if(face>=original.size()){error="Invalid native base face material.";return false;}
    auto faceTo=WarriorNativeTextureBindings::destination(original[face],combined);
    if(!faceTo){error="Missing native base face material.";return false;}
    mesh[0x4c]=char(faceTo-combined.data());
    unsigned skip=static_cast<unsigned char>(bodyMesh[0x54]);
    if(skip) {
        if(skip>=original.size()){error="Invalid native base alpha exclusion.";return false;}
        auto to=WarriorNativeTextureBindings::destination(original[skip],combined);
        if(!to || to==combined.data()){error="Unsupported native alpha exclusion remapping.";return false;}
        mesh[0x54]=char(to-combined.data());
    }
    std::vector<size_t> kept;std::vector<unsigned> depths(base.size());
    for(size_t i=0;i<base.size();i++)if(!discard || !discard->count(base[i].node)) {
        int parent=base[i].parent;
        while(parent>=0 && discard && discard->count(base[parent].node))parent=base[parent].parent;
        depths[i]=parent<0?0:depths[parent]+1;kept.push_back(i);
    }
    if(kept.empty()){error="Native part selection removes the whole hierarchy.";return false;}
    for(size_t k=0;k<kept.size();k++) {
        const auto &r=base[kept[k]];
        bool selected=nodes.count(r.node)!=0;const auto &chosen=selected?source[choices.at(r.node)]:r;
        const auto &data=selected?donorMesh:bodyMesh;auto record=data.substr(chosen.offset,chosen.size);
        WarriorNativePack::put(record,4,WarriorNativePack::word(bodyMesh,r.offset+4));
        if(discard && !discard->empty()) {
            unsigned next=k+1<kept.size()?depths[kept[k+1]]:0;
            if(next>depths[kept[k]]+1){error="Invalid native attachment hierarchy.";return false;}
            unsigned pop=depths[kept[k]]+1-next;
            record[4]=char(pop);record[5]=char(pop>>8);
            record[6]=char(k+1==kept.size());record[7]=0;
        }
        if(record.size()>WarriorNativePack::Limit-mesh.size()){error="Native assembled mesh is too large.";return false;}
        mesh+=record;
    }
    std::vector<WarriorNativeParts::Record> checked;
    if(!WarriorNativeParts::records(mesh,checked,error) || checked.size()!=kept.size())return false;
    if(stitchParts && !stitch(mesh,nodes,error))return false;
    if(!WarriorNativeMeshTail::append(mesh,body.entries[2],body.entries[11],textures,error))return false;
    auto result=body;result.entries[2]=std::move(mesh);result.entries[11]=std::move(textures);
    // Retained facial draws must follow the newly assigned texture addresses.
    for(size_t i=3;i<11;i++)if(!WarriorNativeTextureBindings::relocate_stream(result.entries[i],body.entries[11],result.entries[11],error))return false;
    std::string packed;
    if(!WarriorNativePack::encode(result,packed,error) || ((packed.size()+2047)&~size_t(2047))>WarriorNativePack::ModelLimit) {
        error="The native assembled model exceeds the game buffer.";return false;
    }
    out=std::move(result);error.clear();return true;
}
}
