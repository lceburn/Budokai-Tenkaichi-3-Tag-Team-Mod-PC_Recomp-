#include "../src/gs/warrior_native_texture_bindings.h"
#include "../src/gs/warrior_native_build.h"
#include <cassert>
#include <iostream>
static WarriorNativePack::Pack load(const std::filesystem::path &p) {
    std::string raw,error;WarriorNativePack::Pack pack;assert(WarriorNativeBuild::read_file(p,raw,true));assert(WarriorNativePack::decode(raw,pack,error));return pack;
}
int main(int argc,char **argv) {
    using namespace WarriorNativeTextureBindings;assert(argc==3);auto donor=load(std::filesystem::u8path(argv[1])),reference=load(std::filesystem::u8path(argv[2]));
    unsigned matched=0;std::string error;
    for(size_t i=3;i<8;i++) {
        auto stream=donor.entries[i];assert(!stream.empty());
        if(!relocate_stream(stream,donor.entries[11],reference.entries[11],error)) {std::cerr<<"entry "<<i+1<<": "<<error<<"\n";return 1;}
        if(stream!=reference.entries[i]) {std::cerr<<"DIFFERENT facial stream "<<i+1<<"\n";return 1;}matched++;
    }
    std::string invalid(16,'\0');WarriorNativePack::put(invalid,0,0x6000ffff);auto before=invalid;
    assert(!relocate_stream(invalid,donor.entries[11],reference.entries[11],error) && invalid==before);
    std::cout<<"PASS: "<<matched<<" facial drawing streams match reference after native texture relocation; truncated DMA preserves input\n";
}
