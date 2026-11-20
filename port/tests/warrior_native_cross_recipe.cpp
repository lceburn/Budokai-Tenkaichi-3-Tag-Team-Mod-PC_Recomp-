#include "../src/gs/warrior_native_stock.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
int main(int argc,char **argv) {
    assert(argc==4);namespace fs=std::filesystem;
    auto root=fs::u8path(argv[1]),gold=fs::u8path(argv[2]),out=fs::u8path(argv[3]);
    WarriorRecipe::Document recipe;std::string error;
    assert(WarriorRecipe::read(gold/"warrior.txt",recipe.raw));
    assert(!WarriorNativeStock::supports(recipe));setenv("BT3_WARRIOR_NATIVE_HEAD_TEST","1",1);
    assert(WarriorNativeStock::supports(recipe));
    if(!WarriorNativeStock::generate(root,recipe,out,error)){std::cerr<<error<<"\n";return 1;}
    unsigned models=0,resources=0;
    for(const auto &p:fs::recursive_directory_iterator(gold))if(p.is_regular_file()) {
        auto relative=fs::relative(p.path(),gold);std::string a,b;
        assert(WarriorNativeBuild::read_file(p.path(),a,true));
        assert(WarriorNativeBuild::read_file(out/relative,b,true));
        auto name=relative.generic_string();
        if(name.find("_1p")!=std::string::npos || name.find("_2p")!=std::string::npos) {
            WarriorNativePack::Pack reference,native;
            assert(WarriorNativePack::decode(a,reference,error) && WarriorNativePack::decode(b,native,error));
            for(size_t i=17;i<reference.entries.size();i++)if(i!=22) {
                if(reference.entries[i]!=native.entries[i]){std::cerr<<name<<" entry "<<i+1<<" differs\n";return 1;}
            }
            assert(((b.size()+2047)&~size_t(2047))<=0xE6800);models++;
        }else if(name.find("_anm.pak")!=std::string::npos || name.find("_eff.pak")!=std::string::npos ||
                 name.find("_img.cdbt")!=std::string::npos || name.rfind("pzs3us1/",0)==0) {
            if(a!=b){std::cerr<<name<<" differs\n";return 1;}resources++;
        }
    }
    assert(models==4 && resources==5);
    std::cout<<"PASS: gated cross-body recipe generates four model variants with reference technique/voice/scaling entries and five exact supporting resources\n";
}
