#include "../src/gs/warrior_native_materials.h"
#include "../src/gs/warrior_native_build.h"
#include <cassert>
#include <iostream>
int main(int argc,char **argv) {
    using namespace WarriorNativeMaterials;
    assert(argc==3 || argc==7);auto base=std::filesystem::u8path(argv[1]),golden=std::filesystem::u8path(argv[2]);
    std::string raw,error;WarriorNativePack::Pack pack;
    assert(WarriorNativeBuild::read_file(base/"Goku_3_1p.pak",raw,true));
    assert(WarriorNativePack::decode(raw,pack,error));std::vector<Material> m;assert(catalogue(pack,m,error) && !m.empty());
    std::vector<Colour> colours={{m[0].key,255,0,128}};
    if(argc==7)colours={{uint32_t(std::stoul(argv[3])),std::stoi(argv[4]),std::stoi(argv[5]),std::stoi(argv[6])}};
    unsigned checked=0;
    for(const auto &p:std::filesystem::directory_iterator(base)) {
        if(!WarriorNativeBuild::model_name(p.path().filename().u8string()))continue;
        std::string expected,actual;assert(WarriorNativeBuild::read_file(p.path(),raw,true));
        assert(WarriorNativePack::decode(raw,pack,error));
        assert(recolour(pack,colours,error));assert(WarriorNativePack::encode(pack,actual,error));
        assert(WarriorNativeBuild::read_file(golden/p.path().filename(),expected,true));
        if(expected!=actual)assert(WarriorRecipe::write(golden/"native-output.pak",actual));
        if(expected!=actual){std::cerr<<"DIFFERENT "<<p.path().filename()<<"\n";unsigned differences=0;for(size_t n=0;n<std::min(expected.size(),actual.size());n++)if(expected[n]!=actual[n]){if(differences++<24)std::cerr<<n<<":"<<int(static_cast<unsigned char>(expected[n]))<<"/"<<int(static_cast<unsigned char>(actual[n]))<<" ";}std::cerr<<" count="<<differences<<"\n";return 1;}checked++;
    }
    assert(checked==6);
    auto original=pack.entries[11];
    assert(!recolour(pack,{{m[0].key,-1,0,0}},error));assert(pack.entries[11]==original);
    assert(!recolour(pack,{{m[0].key,0,0,0},{m[0].key,0,0,0}},error));assert(pack.entries[11]==original);
    assert(!recolour(pack,{{123,0,0,0}},error));assert(pack.entries[11]==original);
    std::cout<<"PASS: native palette recolouring matches all six reference variants; invalid edits preserve models\n";
}
