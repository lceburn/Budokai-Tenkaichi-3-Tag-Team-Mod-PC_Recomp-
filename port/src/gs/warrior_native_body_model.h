#pragma once
#include "warrior_native_body_fitting.h"
#include "warrior_native_assembly.h"
namespace WarriorNativeBodyModel {
struct Selection {const WarriorNativePack::Pack *donor;std::set<unsigned> slots;uint64_t sourceKey=0;};
// Fit every donor against the original body. Join boundaries only after all
// selected records are present, so neighboring changes meet each other.
inline bool compose(const WarriorNativePack::Pack &body,const std::vector<Selection> &selections,
                    WarriorNativePack::Pack &out,std::string &error) {
    if(body.entries.size()<26 || selections.empty()){error="Incomplete native body part selection.";return false;}
    auto result=body;std::set<unsigned> usedSlots,nodes;
    std::vector<WarriorNativeParts::Record> rows;
    if(!WarriorNativeParts::records(body.entries[2],rows,error))return false;
    std::map<unsigned,uint64_t> origins;
    for(const auto &r:rows)origins.emplace(r.node,0);
    for(const auto &selection:selections) {
        if(!selection.donor || selection.donor->entries.size()<26 || selection.slots.empty()) {
            error="Incomplete native body part source.";return false;
        }
        for(auto slot:selection.slots)if(!usedSlots.insert(slot).second) {
            error="Conflicting native body part selections.";return false;
        }
        std::string fitted;
        if(!WarriorNativeBodyFitting::fit(body.entries[2],selection.donor->entries[2],selection.slots,fitted,error))return false;
        std::set<unsigned> selected;
        uint64_t origin=selection.donor->entries[2]==body.entries[2]?0:
            selection.sourceKey?selection.sourceKey:uint64_t(reinterpret_cast<uintptr_t>(selection.donor));
        for(const auto &r:rows)if(selection.slots.count(unsigned(WarriorNativeParts::slot(r.node)))) {
            selected.insert(r.node);WarriorNativeFitting::Fit fit;
            if(!WarriorNativeFitting::fit(body.entries[2],selection.donor->entries[2],r.node,fit,error))return false;
            // The reference assigns an individual join identity once either
            // fitting scale differs from one by more than one percent.
            origins[r.node]=(std::fabs(fit.length-1.f)>.01f || std::fabs(fit.width-1.f)>.01f)?
                (UINT64_C(1)<<63)|r.node:origin;
        }
        WarriorNativePack::Pack assembled;
        std::vector<WarriorNativeParts::Record> currentRows;
        if(!WarriorNativeParts::records(result.entries[2],currentRows,error))return false;
        std::set<unsigned> discard;
        for(size_t i=0;i<currentRows.size();i++) {
            int owner=WarriorNativeParts::attachment_slot(currentRows,i);
            if(owner>=0 && selection.slots.count(unsigned(owner)))discard.insert(currentRows[i].node);
        }
        if(!WarriorNativeAssembly::replace(result,*selection.donor,fitted,selected,assembled,error,false,&discard))return false;
        nodes.insert(selected.begin(),selected.end());result=std::move(assembled);
    }
    if(!WarriorNativeAssembly::stitch(result.entries[2],nodes,error,&origins))return false;
    out=std::move(result);error.clear();return true;
}
inline bool compose(const WarriorNativePack::Pack &body,const WarriorNativePack::Pack &donor,
                    const std::set<unsigned> &slots,WarriorNativePack::Pack &out,std::string &error) {
    return compose(body,std::vector<Selection>{{&donor,slots}},out,error);
}
}
