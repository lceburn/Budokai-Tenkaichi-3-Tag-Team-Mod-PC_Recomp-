#include "../src/gs/warrior_native_effects.h"
#include <cassert>
#include <iostream>
int main(int argc,char **argv) {
    using namespace WarriorNativeEffects;namespace fs=std::filesystem;
    assert(argc==4);auto root=fs::u8path(argv[1]),base=fs::u8path(argv[2]),golden=fs::u8path(argv[3]);
    WarriorRecipe::Document before,edited;std::string raw,expected,actual,error;
    assert(WarriorRecipe::read(base/"warrior.txt",before.raw));assert(WarriorRecipe::read(golden/"warrior.txt",edited.raw));
    assert(WarriorNativeBuild::nonvisual_only(before,edited));
    assert(WarriorNativeBuild::read_file(base/"Goku_3_eff.pak",raw,true));WarriorNativePack::Pack pack;
    assert(WarriorNativePack::decode(raw,pack,error));assert(update(root,pack,before,edited,error));
    assert(WarriorNativePack::encode(pack,actual,error));assert(WarriorNativeBuild::read_file(golden/"Goku_3_eff.pak",expected,true));assert(actual==expected);
    assert(update(root,pack,edited,before,error));assert(WarriorNativePack::encode(pack,actual,error));assert(actual==raw);
    for(const auto &value:{"Vegeta (Scouter):0","Vegeta (Scouter):4","Vegeta (Scouter):12","Missing:1"}) {
        auto invalid=before;invalid.raw+="blast2_1fx="+std::string(value)+"\n";
        assert(!update(root,pack,before,invalid,error));assert(WarriorNativePack::encode(pack,actual,error) && actual==raw);
    }
    std::cout<<"PASS: native effect swap and override removal match reference; invalid sources preserve resources\n";
}
