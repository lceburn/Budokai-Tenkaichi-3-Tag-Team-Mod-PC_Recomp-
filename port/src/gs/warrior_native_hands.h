// Native hand/finger fitting using the reference's existing attachment rules.
#pragma once
#include "warrior_native_seams.h"
#include "warrior_native_assembly.h"
namespace WarriorNativeHands {
inline bool fit(const std::string &body,const std::string &donor,std::string &out,std::string &error) {
    std::vector<WarriorNativeParts::Record> rows;
    if(!WarriorNativeParts::records(donor,rows,error))return false;
    std::string result=donor;std::set<unsigned> nodes;
    for(const auto &r:rows)if(WarriorNativeParts::slot(r.node)==7) {
        WarriorNativeFitting::Fit fitted;
        if(!WarriorNativeFitting::fit(body,donor,r.node,fitted,error))return false;
        bool uniform=(r.node>=21 && r.node<=30)||(r.node>=35 && r.node<=44);
        if(!uniform) {
            WarriorNativeSeams::Points points;
            if(!WarriorNativeSeams::read(donor,r,points,error))return false;
            if(!points.offsets.empty()){error="This hand endpoint needs additional native fitting support.";return false;}
        }
        if(!WarriorNativeSeams::anchors(result,body,r.node,error,uniform?(fitted.length+fitted.width)*.5f:1.f,&r))return false;
        nodes.insert(r.node);
    }
    if(nodes.size()!=22){error="The native hand hierarchy is incomplete.";return false;}
    // Adjacent fingers still need stitching after attachment, even though
    // their geometry came from the same donor.
    for(const auto &r:rows)if(WarriorNativeParts::slot(r.node)==7 && r.parent>=0 &&
        WarriorNativeParts::slot(rows[r.parent].node)==7)
        if(!WarriorNativeSeams::stitch(result,r,rows[r.parent],nullptr,error))return false;
    for(const auto &r:rows)if(WarriorNativeParts::slot(r.node)==7)
        if(!WarriorNativeSeams::bounds(result,r,error))return false;
    out=std::move(result);error.clear();return true;
}
inline bool compose(const WarriorNativePack::Pack &body,const WarriorNativePack::Pack &donor,
                    WarriorNativePack::Pack &out,std::string &error) {
    if(body.entries.size()<26 || donor.entries.size()<26){error="Incomplete native hand source model.";return false;}
    std::string mesh;if(!fit(body.entries[2],donor.entries[2],mesh,error))return false;
    std::set<unsigned> nodes;for(unsigned i=21;i<=31;i++)nodes.insert(i);
    for(unsigned i=35;i<=45;i++)nodes.insert(i);
    return WarriorNativeAssembly::replace(body,donor,mesh,nodes,out,error);
}
}
