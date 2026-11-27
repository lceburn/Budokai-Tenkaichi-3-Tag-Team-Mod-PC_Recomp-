#include "../src/gs/warrior_native_regenerate.h"
#include <cassert>
#include <iostream>
using namespace WarriorNativeRegenerate;
int main(int argc,char **argv) {
    assert(argc==2);auto root=fs::absolute(fs::u8path(argv[1]));
    auto source=root/"mods/A",stage=root/"staged";
    fs::create_directories(source/"pzs3us1");fs::create_directories(stage/"pzs3us1");
    const char *a[]={"Old_1p.pak","Old_1p_dmg.pak","Old_anm.pak","Old_eff.pak","Old_img.cdbt","pzs3us1/00013.bin","pzs3us1/00014.bin"};
    const char *b[]={"New_1p.pak","New_1p_dmg.pak","New_anm.pak","New_eff.pak","New_img.cdbt","pzs3us1/00065.bin","pzs3us1/00066.bin"};
    std::string oldManifest="#warrior 47\n",newManifest="#warrior 47\n";
    for(unsigned i=0;i<7;i++) {
        oldManifest+=std::string(a[i])+"\n";newManifest+=std::string(b[i])+"\n";
        assert(WarriorRecipe::write(source/a[i],"old"));
        assert(WarriorRecipe::write(stage/b[i],i<4?"new":"old"));
    }
    assert(WarriorRecipe::write(source/".warrior_built",oldManifest));
    assert(WarriorRecipe::write(stage/".warrior_built",newManifest));
    for(auto name:{"warrior.txt","mod.txt","native-colours.tsv"}) {
        assert(WarriorRecipe::write(source/name,"old\n"));assert(WarriorRecipe::write(stage/name,"new\n"));
    }
    std::vector<FileEdit> files;std::string error;
    assert(plan(source,stage,files,error) && files.size()==8);
    for(unsigned i=0;i<4;i++)assert(files[i].path==source/a[i]);
    assert(publish(root,files,error));
    std::string raw;assert(read_file(source/"Old_anm.pak",raw,true) && raw=="new");
    auto refreshed=oldManifest;refreshed.replace(0,12,"#warrior 49\n");
    assert(read_file(source/".warrior_built",raw) && raw==refreshed);
    assert(!fs::exists(source/"New_anm.pak"));
    // Portraits publish under the original routed names, including both members.
    for(unsigned i=4;i<7;i++)assert(WarriorRecipe::write(stage/b[i],std::string("portrait\0bytes",14)));
    files.clear();assert(plan(source,stage,files,error) && files.size()==3);
    for(const auto &file:files)assert(file.portrait && !file.model && !file.resource);
    assert(publish(root,files,error));
    for(unsigned i=4;i<7;i++)assert(read_file(source/a[i],raw,true) && raw==std::string("portrait\0bytes",14));
    assert(!fs::exists(source/b[4]) && !fs::exists(source/b[5]));
    // A nested member filename is not a general-purpose binary write capability.
    assert(WarriorRecipe::write(source/"pzs3us1/evil.bin","old"));
    assert(!publish(root,{{source/"pzs3us1/evil.bin","old","new",false,false,true}},error));
    assert(!publish(root,{{source/"warrior.txt","new\n","binary",false,false,true}},error));
    files.clear();
    assert(WarriorRecipe::write(stage/".warrior_built",newManifest+"../escape\n"));
    assert(!plan(source,stage,files,error) && files.empty());
    assert(role("pzs3us1/00013.bin")==12 && role("pzs3us1/../x.bin")==-1);
    assert(role("Old_1p.pak")!=role("New_1p_dmg.pak"));
    std::cout<<"PASS: donor and portrait edits retain routed filenames; binary members are bounded to their warrior folder\n";
}
