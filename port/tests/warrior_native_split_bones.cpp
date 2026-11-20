#include "../src/gs/warrior_native_seams.h"
#include <cassert>
#include <iostream>
int main() {
    using namespace WarriorNativeFitting;
    std::string mesh(0x90+64+96+96,'\0'),error;
    WarriorNativePack::put(mesh,0,0x6c646d70);WarriorNativePack::put(mesh,0x6c,0x90);
    auto record=[&](size_t offset,unsigned size,unsigned node,bool end) {
        WarriorNativePack::put(mesh,offset,size);mesh[offset+6]=end;mesh[offset+10]=char(node);
    };
    record(0x90,64,16,false);record(0xd0,96,17,false);mesh[0xd4]=1;record(0x130,96,17,true);
    for(size_t offset:{size_t(0xd0),size_t(0x130)}) {
        scalar(mesh,offset+20,-1);scalar(mesh,offset+80,1);scalar(mesh,offset+84,1);scalar(mesh,offset+88,1);
    }
    scalar(mesh,0x130+80,2);scalar(mesh,0x130+88,3);
    float width;assert(girth(mesh,17,{{0,1,0}},width,error) && std::fabs(width-std::sqrt(13.f))<1e-6f);
    std::vector<WarriorNativeParts::Record> rows;assert(WarriorNativeParts::records(mesh,rows,error));
    const WarriorNativeParts::Record *ambiguous;assert(!find(rows,17,ambiguous,error));
    assert(anchor(mesh,rows,17,ambiguous,error));
    auto target=mesh.substr(0,0x130);target[0xd6]=1;scalar(target,0xd0+20,-2);
    assert(WarriorNativeSeams::anchors(mesh,target,17,error,1,&rows[2]));
    assert(scalar(mesh,0xd0+20)==-1 && scalar(mesh,0x130+20)==-2);
    auto before=width;assert(!girth(mesh,17,{{0,1,0}},width,error) && width==before);
    assert(!anchor(mesh,rows,17,ambiguous,error));
    std::cout<<"PASS split bounds, explicit source anchor and rejection of conflicting bone coordinates\n";
}
