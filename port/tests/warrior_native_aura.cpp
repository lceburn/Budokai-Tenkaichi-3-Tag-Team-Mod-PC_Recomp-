#include "../src/gs/warrior_native_stock.h"
#include <cassert>
#include <iostream>
int main(int argc,char **argv) {
    assert(argc==7 || argc==8);namespace fs=std::filesystem;auto root=fs::u8path(argv[1]),base=fs::u8path(argv[2]),gold=fs::u8path(argv[3]);
    WarriorRecipe::Document before,edited;std::string raw,expected,error;assert(WarriorRecipe::read(base/"warrior.txt",before.raw));assert(WarriorRecipe::read(gold/"warrior.txt",edited.raw));
    assert(WarriorNativeBuild::nonvisual_only(before,edited));assert(WarriorNativeEffects::changed(before,edited));assert(WarriorNativeStock::supports(edited));
    assert(WarriorNativeBuild::read_file(base/"Goku_3_eff.pak",raw,true));assert(WarriorNativeBuild::read_file(gold/"Goku_3_eff.pak",expected,true));
    WarriorNativePack::Pack original,reference,updated,assembled;assert(WarriorNativePack::decode(raw,original,error) && WarriorNativePack::decode(expected,reference,error));
    if(argc==8){std::string resource;assert(WarriorNativeBuild::read_file(argv[7],resource,true));reference.entries[std::atoi(argv[4])]=resource;}
    updated=original;
    if(!WarriorNativeEffects::update(root,updated,before,edited,error)){std::cerr<<error<<"\n";return 1;}
    if(updated.entries!=reference.entries){for(size_t i=0;i<updated.entries.size();i++)if(updated.entries[i]!=reference.entries[i])std::cerr<<"entry "<<i<<" native="<<updated.entries[i].size()<<" ref="<<reference.entries[i].size()<<"\n";return 2;}
    assert(WarriorNativeEffects::assemble(root,edited,assembled,error) && assembled.entries==reference.entries);
    auto output=fs::u8path(argv[5]);assert(!fs::exists(output));assert(WarriorNativeStock::generate(root,edited,output,error));
    std::string stock;WarriorNativePack::Pack generated;assert(WarriorNativeBuild::read_file(output/"Goku_3_eff.pak",stock,true) && WarriorNativePack::decode(stock,generated,error));assert(generated.entries==reference.entries);
    for(unsigned outfit=1;outfit<=3;outfit++)for(const char *damage:{"","_dmg"}) {
        auto file="Goku_3_"+std::to_string(outfit)+"p"+damage+".pak";std::string actual,wanted;
        assert(WarriorNativeBuild::read_file(output/file,actual,true) && WarriorNativeBuild::read_file(gold/file,wanted,true));assert(actual==wanted);
    }
    assert(WarriorNativeEffects::update(root,updated,edited,before,error));assert(updated.entries==original.entries);
    auto broken=before;broken.raw+="aura=notRGB\n";assert(!WarriorNativeEffects::update(root,updated,before,broken,error) && updated.entries==original.entries);
    broken=before;broken.raw+="aurafrom=Missing donor\n";WarriorNativeSources::Model donor;assert(!WarriorNativeSources::model(root,WarriorNativeAura::source(broken),donor,error) && updated.entries==original.entries);
    std::cout<<"PASS: native aura/effect edit, complete stock generation, source reset and invalid-edit preservation\n";
}
