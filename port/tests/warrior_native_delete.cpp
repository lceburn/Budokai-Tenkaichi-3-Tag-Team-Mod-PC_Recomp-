#include "../src/gs/warrior_native_delete.h"
#include <cassert>
#include <iostream>

int main(int argc,char **argv) {
    using namespace WarriorRecipe;
    assert(argc==2);fs::path root=fs::absolute(argv[1]);
    assert(!fs::exists(root));fs::create_directories(root/"mods/Base");fs::create_directories(root/"mods/Form");
    const std::string enabled="# keep comment\r\nBase\r\nForm\r\n";
    const std::string base="name=Base\nvoice=Goku\nform1=1,0,Form\n";
    assert(write(root/"mods/enabled.txt",enabled));assert(write(root/"mods/Base/warrior.txt",base));
    assert(write(root/"mods/Base/mod.txt","white=Base\ntrans0=1,0,2,-1,0,@Form\n"));
    assert(write(root/"mods/Form/warrior.txt","name=Form\nvoice=Goku\n"));
    assert(write(root/"mods/Form/mod.txt","white=Form\n"));
    assert(write(root/"mods/Form/model.pak","generated-resource"));
    std::string error,current;
    assert(!WarriorNativeDelete::archive(root,"../Form",enabled,error));
    assert(!WarriorNativeDelete::archive(root,"Form",enabled+"stale",error));
    assert(!WarriorNativeDelete::archive(root,"Form",enabled,error));
    assert(error.find("transformation")!=std::string::npos);
    // A stale manifest link also blocks deletion even when the recipe has no link.
    assert(write(root/"mods/Base/warrior.txt","name=Base\nvoice=Goku\n"));
    assert(!WarriorNativeDelete::archive(root,"Form",enabled,error));
    assert(write(root/"mods/Base/mod.txt","white=Base\n"));
    assert(!WarriorNativeDelete::archive(root,"Form",enabled,error,[](const fs::path &,const fs::path &){return false;}));
    assert(fs::exists(root/"mods/Form/model.pak"));
    assert(read(root/"mods/enabled.txt",current) && current==enabled);
    assert(WarriorNativeDelete::archive(root,"Form",enabled,error));
    assert(!fs::exists(root/"mods/Form"));
    assert(read(root/"mods/enabled.txt",current) && current=="# keep comment\r\nBase\r\n");
    assert(read(root/"mods/Base/warrior.txt",current) && current=="name=Base\nvoice=Goku\n");
    unsigned backups=0;
    for(const auto &job:fs::directory_iterator(root/".warrior-deleted"))if(fs::exists(job.path()/"character/model.pak")) {
        assert(read(job.path()/"character/model.pak",current) && current=="generated-resource");backups++;
    }
    assert(backups==1);
    std::cout<<"Character deletion, link protection, index preservation and failed-publication rollback passed.\n";
}
