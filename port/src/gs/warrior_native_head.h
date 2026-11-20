// Experimental complete-head assembly, gated by a development-only stock path.
#pragma once
#include "warrior_native_texture_bindings.h"
#include "warrior_native_seams.h"
#include "warrior_native_mesh_tail.h"
#include "warrior_native_head_chains.h"
#include "warrior_native_head_pieces.h"
namespace WarriorNativeHead {
inline bool compose(const WarriorNativePack::Pack &body,const WarriorNativePack::Pack &head,
                    WarriorNativePack::Pack &out,std::string &error,unsigned mask=7,bool deferSeams=false) {
    if(!mask || (mask&~7u)){error="Invalid native head part selection.";return false;}
    if(body.entries.size()<26 || head.entries.size()<26) {error="Incomplete native head source model.";return false;}
    WarriorNativeParts::HeadPlan plan;
    if(!WarriorNativeParts::whole_head_plan(body.entries[2],head.entries[2],plan,error))return false;
    std::set<size_t> donorRows,bodyRows;
    auto selected=[&](const WarriorNativeParts::Choice &c) {
        unsigned node=plan.base[c.base].node;
        return c.imported && (node==48?(mask&2u):(mask&5u));
    };
    for(const auto &c:plan.choices)if(selected(c))donorRows.insert(c.donor);else bodyRows.insert(c.base);
    bool mixedHead=bool(mask&1)!=bool(mask&4);
    if(mixedHead)for(const auto &c:plan.choices)if(plan.base[c.base].node==46 || plan.base[c.base].node==47)bodyRows.insert(c.base);
    std::string combined;
    if(!WarriorNativeTextureBindings::merge(body.entries[11],head.entries[11],head.entries[2],donorRows,combined,error,&body,(mask&2)?&head:nullptr))return false;
    auto bodyMesh=body.entries[2],headMesh=head.entries[2];
    for(unsigned node:{46u,47u})if((mask&5u) && !WarriorNativeSeams::anchors(headMesh,bodyMesh,node,error))return false;
    if((mask&2) && !WarriorNativeFitting::face(headMesh,bodyMesh,error))return false;
    if(!WarriorNativeTextureBindings::relocate(bodyMesh,body.entries[11],combined,bodyRows,error)) {
        error="Native head body binding: "+error;return false;
    }
    if(!WarriorNativeTextureBindings::relocate(headMesh,head.entries[11],combined,donorRows,error)) {
        error="Native head donor binding: "+error;return false;
    }
    std::vector<WarriorNativeTextureBindings::Texture> textures,headTextures;
    const auto &faceOwner=(mask&2)?head:body;
    if(!WarriorNativeTextureBindings::textures(combined,textures,error) ||
       !WarriorNativeTextureBindings::textures(faceOwner.entries[11],headTextures,error))return false;
    size_t first=plan.base.front().offset;std::string mesh=bodyMesh.substr(0,first);
    unsigned normal=0,shades=0;
    for(const auto &t:textures)if(t.psm==0)shades++;else normal++;
    if(normal>255 || shades>255){error="Native head material counts are too large.";return false;}
    mesh[0x4e]=0;mesh[0x4f]=char(normal);
    mesh[0x50]=char(normal);mesh[0x51]=char(shades);
    unsigned face=static_cast<unsigned char>(faceOwner.entries[2][0x4c]);
    if(face>=headTextures.size()){error="The donor face material is invalid.";return false;}
    auto faceTexture=WarriorNativeTextureBindings::destination(headTextures[face],textures);
    if(!faceTexture){error="The donor face material has no native destination.";return false;}
    mesh[0x4c]=char(faceTexture-textures.data());
    unsigned alpha=static_cast<unsigned char>(bodyMesh[0x54]);
    if(alpha) {
        std::vector<WarriorNativeTextureBindings::Texture> original;
        if(!WarriorNativeTextureBindings::textures(body.entries[11],original,error) || alpha>=original.size()) {
            error="Invalid native head alpha exclusion.";return false;
        }
        auto target=WarriorNativeTextureBindings::destination(original[alpha],textures);
        if(!target || target==textures.data()){error="Missing native head alpha exclusion.";return false;}
        mesh[0x54]=char(target-textures.data());
    }
    auto bodyFlags=WarriorNativePack::word(mesh,12),headFlags=WarriorNativePack::word(headMesh,12);
    if(mask&2)WarriorNativePack::put(mesh,12,(bodyFlags&~0xc00000u)|(headFlags&0xc00000u));
    for(const auto &c:plan.choices) {
        bool imported=selected(c);
        const auto &base=plan.base[c.base];const auto &chosen=imported?plan.donor[c.donor]:base;
        const auto &source=imported?headMesh:bodyMesh;
        auto record=source.substr(chosen.offset,chosen.size);
        if(mixedHead && (base.node==46 || base.node==47) &&
           !WarriorNativeHeadPieces::mixed_record(body.entries[2],head.entries[2],bodyMesh,headMesh,
               base,plan.donor[c.donor],mask,record,error))return false;
        // Retain the base parent traversal, including termination. The donor
        // contributes its geometry, bind coordinates, bounds and enabled state.
        WarriorNativePack::put(record,0,unsigned(record.size()));
        WarriorNativePack::put(record,4,WarriorNativePack::word(bodyMesh,base.offset+4));
        if(mesh.size()>WarriorNativePack::Limit-record.size()){error="The native head mesh is too large.";return false;}
        mesh+=record;
    }
    std::vector<WarriorNativeParts::Record> checked;
    if(!WarriorNativeParts::records(mesh,checked,error) || checked.size()!=plan.base.size())return false;
    // The neck attaches to the actual hierarchy parent (often chest node 17),
    // rather than assuming the torso anchor is also its drawing parent.
    auto importedNode=[&](unsigned node) {
        return head.entries[2]!=body.entries[2] &&
            (node==46?(mask&1u):node==47?(mask&1u):node==48?(mask&2u):false);
    };
    for(const auto &r:checked)if(!deferSeams && r.node>=46 && r.node<=48 && r.parent>=0 &&
        bool(importedNode(r.node))!=bool(importedNode(checked[r.parent].node))) {
        if(!WarriorNativeSeams::stitch(mesh,r,checked[r.parent],nullptr,error))return false;
    }
    if(mixedHead)for(const auto &r:checked)if((r.node==46 || r.node==47) && r.size>=96)
        if(!WarriorNativeSeams::bounds(mesh,r,error))return false;
    if(!WarriorNativeMeshTail::append(mesh,body.entries[2],body.entries[11],combined,error))return false;
    auto result=body;result.entries[2]=std::move(mesh);result.entries[11]=std::move(combined);
    // The face donor owns its mouth draws and alternate facial texture sets.
    for(size_t i=3;i<11;i++) {
        auto stream=faceOwner.entries[i];
        if((mask&2) && !WarriorNativeFitting::face_stream(stream,head.entries[2],body.entries[2],error)) {
            error="Native head expression fitting "+std::to_string(i+1)+": "+error;return false;
        }
        if(!WarriorNativeTextureBindings::relocate_stream(stream,faceOwner.entries[11],result.entries[11],error)) {
            error="Native head facial stream "+std::to_string(i+1)+": "+error;return false;
        }
        result.entries[i]=std::move(stream);
    }
    if(mask&2)for(size_t i=12;i<17;i++)result.entries[i]=head.entries[i];
    if(mask&4) {
        WarriorNativePack::Pack withHair;
        if(!WarriorNativeHeadChains::apply(result,head,withHair,error))return false;
        result=std::move(withHair);
    }
    // Chain synthesis and head-auxiliary shading substitutions are still
    // unverified. This output is for isolated inspection, never live publication.
    std::string packed;
    if(!WarriorNativePack::encode(result,packed,error) || ((packed.size()+2047)&~size_t(2047))>WarriorNativePack::ModelLimit) {
        error="The experimental head model exceeds the game buffer.";return false;
    }
    out=std::move(result);return true;
}
}
