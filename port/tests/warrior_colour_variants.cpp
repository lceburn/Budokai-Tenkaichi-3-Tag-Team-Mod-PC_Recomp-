#include "../src/gs/warrior_native_stock.h"
#include <cassert>
#include <iostream>
int main(int argc,char **argv) {
    using namespace WarriorNativeStock;using WarriorNativeMaterials::Material;
    assert(argc==4);auto root=fs::u8path(argv[1]),output=fs::u8path(argv[3]);
    Document recipe;std::string error;assert(WarriorRecipe::read(fs::u8path(argv[2]),recipe.raw));
    std::vector<Material> materials;
    if(!generate(root,recipe,{},error,{},"",&materials)){std::cerr<<error<<"\n";return 1;}
    unsigned normalCount=0,damageCount=0,changedIdentity=0;const Material *chosen=nullptr;
    for(const auto &m:materials) {
        if(!m.damaged){normalCount++;continue;}damageCount++;
        if(m.normalKey && m.normalKey!=m.key){changedIdentity++;if(!chosen)chosen=&m;}
    }
    assert(normalCount && damageCount);
    // Prefer the formerly missed changed-texture identity, not just exact matches.
    if(!chosen)for(const auto &m:materials)if(m.damaged && m.normalKey){chosen=&m;break;}
    assert(chosen);
    if(!generate(root,recipe,output/"base",error)){std::cerr<<error<<"\n";return 1;}
    recipe.raw+="colour="+std::to_string(chosen->normalKey)+",29,79,139\n";
    if(!generate(root,recipe,output/"painted",error)){std::cerr<<error<<"\n";return 1;}
    std::string defaults,paintedDefaults;
    assert(WarriorRecipe::read(output/"base/native-colours.tsv",defaults));
    assert(WarriorRecipe::read(output/"painted/native-colours.tsv",paintedDefaults));
    assert(defaults==paintedDefaults); // original donor defaults survive a save
    unsigned recolouredDamage=0,models=0;
    for(const auto &file:fs::directory_iterator(output/"base")) {
        if(!WarriorNativeBuild::model_name(file.path().filename().u8string()))continue;
        std::string a,b;WarriorNativePack::Pack before,after;
        assert(WarriorNativeBuild::read_file(file.path(),a,true));
        assert(WarriorNativeBuild::read_file(output/"painted"/file.path().filename(),b,true));
        assert(WarriorNativePack::decode(a,before,error) && WarriorNativePack::decode(b,after,error));
        assert(before.entries.size()==after.entries.size());
        for(size_t i=0;i<before.entries.size();i++)if(i<2 || (i>16))assert(before.entries[i]==after.entries[i]);
        // Independent shading can change a material's shading address, but
        // must preserve all vertex data and other mesh/facial-stream fields.
        for(unsigned i=2;i<=10;i++) {
            auto geometry=after.entries[i];const auto &original=before.entries[i];assert(geometry.size()==original.size());
            for(size_t at=0;at+88<=original.size();at+=4)if(WarriorNativePack::word(original,at)==0x6c058000)
                WarriorNativePack::put(geometry,at+40,WarriorNativePack::word(original,at+40));
            if(i==2)for(unsigned field:{0x4cu,0x4du,0x51u,0x52u})geometry[field]=original[field];
            assert(geometry==original);
        }
        if(file.path().filename().u8string().find("_dmg")!=std::string::npos && before.entries[11]!=after.entries[11])recolouredDamage++;
        models++;
    }
    assert(models>=2 && recolouredDamage);
    recipe.raw+="damagecolour="+std::to_string(chosen->key)+",199,169,119\n";
    if(!generate(root,recipe,output/"override",error)){std::cerr<<error<<"\n";return 1;}
    for(const auto &file:fs::directory_iterator(output/"painted"))if(WarriorNativeBuild::model_name(file.path().filename().u8string()) && file.path().filename().u8string().find("_dmg")==std::string::npos) {
        std::string a,b;assert(WarriorNativeBuild::read_file(file.path(),a,true));
        assert(WarriorNativeBuild::read_file(output/"override"/file.path().filename(),b,true));assert(a==b);
    }
    std::cout<<"PASS: models="<<models<<" normal_materials="<<normalCount<<" damaged_materials="<<damageCount
             <<" mapped_changed_identities="<<changedIdentity<<" recoloured_damaged_models="<<recolouredDamage
             <<"; stable geometry, original defaults and isolated damaged overrides\n";
}
