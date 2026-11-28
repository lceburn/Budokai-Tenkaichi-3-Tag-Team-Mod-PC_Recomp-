#include "../src/gs/warrior_native_effects.h"
#include <cassert>
#include <iostream>
using namespace WarriorNativePack;
static Pack read_pack(const std::filesystem::path &p){
    std::string raw,error;Pack pack;
    assert(WarriorNativeBuild::read_file(p,raw,true) && decode(raw,pack,error));return pack;
}
int main(int argc,char **argv){
    assert(argc>=4);namespace fs=std::filesystem;auto root=fs::u8path(argv[1]);
    auto base=fs::u8path(argv[2]);WarriorRecipe::Document before;
    assert(WarriorRecipe::read(base/"warrior.txt",before.raw));
    auto original=read_pack(base/"Goku_3_eff.pak");
    for(int i=3;i<argc;i++){
        auto golden=fs::u8path(argv[i]);WarriorRecipe::Document edited;std::string error;
        assert(WarriorRecipe::read(golden/"warrior.txt",edited.raw));
        auto expected=read_pack(golden/"Goku_3_eff.pak");Pack actual;
        assert(WarriorNativeEffects::assemble(root,edited,actual,error));
        if(actual.entries!=expected.entries){
            for(unsigned j=0;j<actual.entries.size();j++)if(actual.entries[j]!=expected.entries[j])std::cerr<<golden<<" entry "<<j<<" differs\n";
            return 1;
        }
        auto updated=original;assert(WarriorNativeEffects::update(root,updated,before,edited,error));
        assert(updated.entries==actual.entries && updated.flags==actual.flags);
        assert(WarriorNativeEffects::update(root,updated,edited,before,error));
        assert(updated.entries==original.entries && updated.flags==original.flags);
    }
    std::cout<<"PASS "<<argc-3<<" controlled reference effect cases; fresh build, edit and reset agree\n";
}
