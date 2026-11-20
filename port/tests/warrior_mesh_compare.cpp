#include "warrior_mesh_compare.h"
#include <iostream>
int main() {
    using namespace WarriorNativePack;using WarriorNativeFitting::scalar;
    std::string mesh(368,'\0'),error;put(mesh,96,0x60000010);put(mesh,104,0x6c058000);put(mesh,156,3);
    scalar(mesh,240,1);scalar(mesh,288+4,1);
    for(size_t at:{size_t(192),size_t(240),size_t(288)}){scalar(mesh,at+12,1);scalar(mesh,at+24,1);}
    WarriorNativeParts::Record r{0,mesh.size(),0,1,17,0,-1};WarriorNativeSeams::Points a;
    assert(WarriorNativeSeams::read(mesh,r,a,error));
    auto compare=[&](const std::string &changed){WarriorNativeSeams::Points b;assert(WarriorNativeSeams::read(changed,r,b,error));return WarriorMeshCompare::retained(mesh,r,a,changed,r,b);};
    auto cyclic=mesh;cyclic.replace(192,48,mesh,240,48);cyclic.replace(240,48,mesh,288,48);cyclic.replace(288,48,mesh,192,48);
    assert(compare(cyclic));
    auto reversed=mesh;reversed.replace(192,48,mesh,240,48);reversed.replace(240,48,mesh,192,48);assert(!compare(reversed));
    auto moved=mesh;scalar(moved,240,1.1f);assert(!compare(moved));
    auto normal=mesh;scalar(normal,192+24,-1);assert(!compare(normal));
    auto weight=mesh;scalar(weight,192+12,.5f);assert(!compare(weight));
    auto uv=mesh;scalar(uv,192+32,.5f);assert(!compare(uv));
    std::cout<<"PASS equivalent strip ordering; rejects geometry, winding, normal, weight and UV changes\n";
}
