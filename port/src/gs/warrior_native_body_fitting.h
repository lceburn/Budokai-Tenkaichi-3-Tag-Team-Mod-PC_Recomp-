// Reference-compatible fitting of torso and limb donor geometry.
// Stitch after assembly so attachment boundaries come from the retained body.
#pragma once
#include "warrior_native_seams.h"
#include <set>
namespace WarriorNativeBodyFitting {
using WarriorNativeFitting::Vec3;
struct Segment {Vec3 a,b;};
inline bool segments(const std::string &mesh,const std::vector<WarriorNativeParts::Record> &rows,
                     std::vector<Segment> &out,std::string &error) {
    static constexpr unsigned pairs[][2]={{16,46},{9,10},{10,11},{13,14},{14,15},{46,19},{46,33},{19,20},{20,21},{33,34},{34,35}};
    std::vector<Segment> result;
    for(const auto &pair:pairs) {
        const WarriorNativeParts::Record *a,*b;
        if(!WarriorNativeFitting::anchor(mesh,rows,pair[0],a,error) || !WarriorNativeFitting::anchor(mesh,rows,pair[1],b,error))return false;
        Segment s;
        for(unsigned k=0;k<3;k++) {
            s.a[k]=WarriorNativeFitting::scalar(mesh,a->offset+16+k*4);
            s.b[k]=WarriorNativeFitting::scalar(mesh,b->offset+16+k*4);
            if(!std::isfinite(s.a[k]) || !std::isfinite(s.b[k])){error="Invalid native fitting segment.";return false;}
        }
        result.push_back(s);
    }
    out=std::move(result);error.clear();return true;
}
inline Vec3 radial(const Vec3 &point,const std::vector<Segment> &segments) {
    Vec3 sum{{0,0,0}};float total=0;
    for(const auto &s:segments) {
        Vec3 direction,difference;float numerator=0,denominator=0;
        for(unsigned k=0;k<3;k++) {
            direction[k]=s.b[k]-s.a[k];numerator+=(point[k]-s.a[k])*direction[k];
            denominator+=direction[k]*direction[k];
        }
        float t=denominator>1e-8f?std::max(0.f,std::min(1.f,numerator/denominator)):0.f;
        float distance=0;
        for(unsigned k=0;k<3;k++){difference[k]=point[k]-(s.a[k]+t*direction[k]);distance+=difference[k]*difference[k];}
        float weight=1.f/(distance*distance+1.0000001e-6f);total+=weight;
        for(unsigned k=0;k<3;k++)sum[k]+=weight*difference[k];
    }
    if(total>0)for(auto &v:sum)v/=total;
    return sum;
}
inline bool fit(const std::string &body,const std::string &donor,const std::set<unsigned> &slots,
                std::string &out,std::string &error) {
    if(slots.empty()){error="No native body parts were selected.";return false;}
    for(auto slot:slots)if(slot<3 || slot>10){error="Unsupported native body part group.";return false;}
    std::vector<WarriorNativeParts::Record> rows,base;
    if(!WarriorNativeParts::records(donor,rows,error) || !WarriorNativeParts::records(body,base,error))return false;
    std::vector<Segment> skeleton;if(!segments(donor,rows,skeleton,error))return false;
    std::string result=donor;unsigned count=0;
    for(const auto &r:rows)if(slots.count(unsigned(WarriorNativeParts::slot(r.node)))) {
        const WarriorNativeParts::Record *target;
        if(!WarriorNativeFitting::anchor(body,base,r.node,target,error))return false;
        WarriorNativeFitting::Fit current,parent;
        if(!WarriorNativeFitting::fit(body,donor,r.node,current,error))return false;
        bool uniform=r.node==11 || r.node==15 || (r.node>=21 && r.node<=30) || (r.node>=35 && r.node<=44);
        if(!WarriorNativeSeams::anchors(result,body,r.node,error,uniform?(current.length+current.width)*.5f:1.f,&r))return false;
        count++;if(uniform)continue;
        parent=current;parent.length=1;
        if((current.length!=1 || current.width!=1) && r.parent>=0 && WarriorNativeParts::slot(rows[r.parent].node)>=3) {
            if(!WarriorNativeFitting::fit(body,donor,rows[r.parent].node,parent,error))return false;
        }
        WarriorNativeSeams::Points points;if(!WarriorNativeSeams::read(donor,r,points,error))return false;
        for(auto offset:points.offsets) {
            float w=WarriorNativeFitting::scalar(donor,offset+12),dc=0,dp=0;Vec3 point;
            for(unsigned k=0;k<3;k++) {
                point[k]=WarriorNativeFitting::scalar(donor,offset+k*4);
                dc+=(point[k]-WarriorNativeFitting::scalar(donor,r.offset+16+k*4))*current.axis[k];
                dp+=(point[k]-WarriorNativeFitting::scalar(donor,r.offset+32+k*4))*parent.axis[k];
            }
            auto thickness=radial(point,skeleton);
            for(unsigned k=0;k<3;k++) {
                float delta=WarriorNativeFitting::scalar(body,target->offset+16+k*4)-WarriorNativeFitting::scalar(donor,r.offset+16+k*4);
                float parentDelta=WarriorNativeFitting::scalar(body,target->offset+32+k*4)-WarriorNativeFitting::scalar(donor,r.offset+32+k*4);
                float value=(point[k]+w*delta)+(1-w)*parentDelta;
                value+=((current.length-1.f)*w)*dc*current.axis[k];
                value+=((parent.length-1.f)*(1-w))*dp*parent.axis[k];
                value+=(current.width-1.f)*thickness[k];
                if(!std::isfinite(value)){error="Native body fitting overflow.";return false;}
                WarriorNativeFitting::scalar(result,offset+k*4,value);
            }
        }
    }
    if(!count){error="The native donor has no selected body parts.";return false;}
    out=std::move(result);error.clear();return true;
}
}
