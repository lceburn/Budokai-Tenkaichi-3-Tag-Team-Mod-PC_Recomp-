// Bounded native reader for the part hierarchy inside a pmdl mesh.
#pragma once
#include "warrior_native_pack.h"
#include <array>
namespace WarriorNativeParts {
struct Record {
    size_t offset,size;unsigned pop,end,node,depth;int parent;
};
inline unsigned half(const std::string &data,size_t at) {
    return static_cast<unsigned char>(data[at])|(unsigned(static_cast<unsigned char>(data[at+1]))<<8);
}
inline int slot(unsigned node) {
    switch(node) {
    case 8:case 9:case 12:case 13:return 8;
    case 10:case 14:return 9;
    case 11:case 15:return 10;
    case 16:return 4;
    case 17:case 18:case 32:return 3;
    case 19:case 33:return 5;
    case 20:case 34:return 6;
    case 46:case 47:return 1;
    case 48:return 2;
    default:return (node>=21 && node<=31) || (node>=35 && node<=45)?7:-1;
    }
}
inline bool records(const std::string &mesh,std::vector<Record> &out,std::string &error) {
    if(mesh.size()<0x90 || mesh.size()>WarriorNativePack::Limit || WarriorNativePack::word(mesh,0)!=0x6c646d70) {
        error="Invalid native part mesh header.";return false;
    }
    size_t at=WarriorNativePack::word(mesh,0x6c);unsigned depth=0;
    std::array<int,64> parents;parents.fill(-1);std::vector<Record> result;
    for(unsigned n=0;n<640;n++) {
        if(at<0x70 || at>mesh.size() || mesh.size()-at<64 || depth>=parents.size()) {
            error="A native mesh record leaves its hierarchy.";return false;
        }
        size_t size=WarriorNativePack::word(mesh,at);unsigned pop=half(mesh,at+4),end=half(mesh,at+6);
        if(size<64 || size>mesh.size()-at || (!end && pop>depth+1)) {
            error="Invalid native part record extent or parent depth.";return false;
        }
        result.push_back({at,size,pop,end,half(mesh,at+10),depth,depth?parents[depth-1]:-1});
        parents[depth]=int(n);
        if(end){out=std::move(result);error.clear();return true;}
        depth=depth+1-pop;at+=size;
    }
    error="The native part hierarchy has no bounded end record.";return false;
}
struct Choice {size_t base,donor;bool imported;};
// Auxiliary chains belong to their first canonical ancestor. Root-attached
// clothing follows the waist; head chains and the special hair node 111 do
// not follow body replacements (wp_compose's inherited-chain rule).
inline int attachment_slot(const std::vector<Record> &rows,size_t index) {
    if(index>=rows.size() || rows[index].node<71)return -1;
    size_t child=index;int parent=rows[index].parent;
    while(parent>=0) {
        if(size_t(parent)>=child)return -1;
        if(rows[parent].node<=70) {
            if(rows[child].node==111 || (rows[parent].node>=46 && rows[parent].node<=48))return -1;
            return rows[parent].node<8?4:slot(rows[parent].node);
        }
        child=size_t(parent);parent=rows[parent].parent;
    }
    return -1;
}
struct HeadPlan {std::vector<Record> base,donor;std::vector<Choice> choices;};
// Plan the three head anchor records without changing bytes or publishing a
// partially assembled model. Geometry fitting and texture rebinding follow.
inline bool whole_head_plan(const std::string &body,const std::string &head,HeadPlan &out,std::string &error) {
    HeadPlan plan;
    if(!records(body,plan.base,error) || !records(head,plan.donor,error))return false;
    std::array<int,3> bodyHead{{-1,-1,-1}},donorHead{{-1,-1,-1}};
    auto find=[&](const std::vector<Record> &rows,std::array<int,3> &indices) {
        for(size_t i=0;i<rows.size();i++)if(rows[i].node>=46 && rows[i].node<=48) {
            auto node=rows[i].node-46;
            if(indices[node]>=0)return false;
            indices[node]=int(i);
        }
        return indices[0]>=0 && indices[1]>=0 && indices[2]>=0;
    };
    if(!find(plan.base,bodyHead) || !find(plan.donor,donorHead)) {
        error="This native whole-head source has missing or ambiguous head anchors.";return false;
    }
    // Preserve the body hierarchy. Auxiliary expression records remain from
    // the body until their individual reference assembly rules are applied.
    for(size_t i=0;i<plan.base.size();i++) {
        const auto &record=plan.base[i];bool import=record.node>=46 && record.node<=48;
        auto donorIndex=import?size_t(donorHead[record.node-46]):size_t(0);
        plan.choices.push_back({i,donorIndex,import});
    }
    out=std::move(plan);error.clear();return true;
}
}
