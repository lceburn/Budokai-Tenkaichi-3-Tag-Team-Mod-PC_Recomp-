#include "../src/gs/warrior_native_texture_bindings.h"
#include <cassert>
#include <iostream>
int main() {
    using namespace WarriorNativeTextureBindings;
    Texture a{10,7,20,UINT64_C(20)<<20,"pixels-a","palette-a",std::string(64,'\0'),"upload-a",std::string(192,'a')};
    Texture b{11,8,20,UINT64_C(20)<<20,"pixels-b","palette-b",std::string(64,'\0'),"upload-b",std::string(192,'b')};
    std::vector<Texture> source={a,b};Texture result;
    WarriorNativePack::put(source[0].header,16,4);WarriorNativePack::put(source[1].header,16,1);
    assert(material(source,10,7,result) && result.pixels==a.pixels && result.palette==a.palette);
    assert(material(source,10,8,result) && result.tbp==10 && result.cbp==8 &&
           result.pixels==a.pixels && result.palette==b.palette && result.paletteUpload==b.paletteUpload);
    assert(source[0].palette==a.palette && source[1].pixels==b.pixels);
    assert(material(source,12,8,result) && result.tbp==10 && result.cbp==8 && result.pixels==a.pixels);
    auto prior=result;assert(!material(source,10,99,result) && result.palette==prior.palette);
    auto incompatible=source;incompatible[1].psm=19;assert(!material(incompatible,10,8,result));
    auto duplicate=prior;duplicate.tbp=20;
    assert(!destination(prior,{prior,duplicate}));
    std::string dummy(88+3*48,'\0');assert(nonrendering_material(dummy,0,3));
    float x=1;std::memcpy(dummy.data()+88+48,&x,4);assert(!nonrendering_material(dummy,0,3));
    assert(!nonrendering_material(dummy,1000,3) && !nonrendering_material(dummy,0,4));
    dummy.assign(88+3*48,'\0');std::memcpy(dummy.data()+88+48+12,&x,4);
    assert(!nonrendering_material(dummy,0,3)); // identical positions, different skin weights
    std::cout<<"PASS independent pixel/palette binding, unchanged sources and strict invisible-packet detection\n";
}
