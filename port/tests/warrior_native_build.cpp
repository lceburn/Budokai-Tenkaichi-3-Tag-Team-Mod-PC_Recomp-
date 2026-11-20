#include "../src/gs/warrior_native_build.h"
#include <cassert>
#include <iostream>
using namespace WarriorNativeBuild;
static std::string contents(const fs::path &p){std::string s;assert(WarriorRecipe::read(p,s));return s;}
int main(int argc,char **argv) {
    assert(argc==2);auto root=fs::absolute(fs::u8path(argv[1]));
    fs::create_directories(root/"mods/A");fs::create_directories(root/"mods/B");
    auto a=root/"mods/A/warrior.txt",b=root/"mods/B/warrior.txt";
    const std::string original="name=One\nbody=Goku (End)\n";
    Document before{original},named{"name=Two\nbody=Goku (End)\nblast1_1name=Custom\n"};
    assert(metadata_only(before,named));
    assert(!metadata_only(before,Document{original+"colour=1,2,3,4\n"}));
    assert(!metadata_only(before,Document{original+"voice=Nappa\n"}));
    assert(!metadata_only(before,Document{original+"style=Nappa\n"}));
    assert(!metadata_only(before,Document{original+"blast2_1fx=Nappa:1\n"}));
    assert(!metadata_only(before,Document{original+"unknown=future feature\n"}));
    assert(WarriorRecipe::write(a,original));assert(WarriorRecipe::write(b,original));
    assert(WarriorRecipe::write(root/"mods/A/model.pak","MODEL BYTES"));
    std::vector<FileEdit> files={{a,original,named.raw},{b,original,named.raw}};
    std::string error;
    assert(publish_metadata(root,files,error));
    assert(contents(a)==named.raw && contents(b)==named.raw);
    assert(contents(root/"mods/A/model.pak")=="MODEL BYTES");
    auto model=root/"mods/A/Model_1p.pak";
    std::string binaryBefore(128,'\0'),binaryAfter(192,'\0');binaryAfter[4]='A';
    assert(WarriorRecipe::write(model,binaryBefore));
    assert(publish(root,{{model,binaryBefore,binaryAfter,true}},error));
    std::string binaryRead;assert(read_file(model,binaryRead,true) && binaryRead==binaryAfter);
    assert(!publish(root,{{model,binaryAfter,std::string(WarriorNativePack::ModelLimit+1,'x'),true}},error));
    assert(read_file(model,binaryRead,true) && binaryRead==binaryAfter);
    std::string boundary(WarriorNativePack::ModelLimit,'x');
    assert(publish(root,{{model,binaryAfter,boundary,true}},error));
    assert(read_file(model,binaryRead,true) && binaryRead==boundary);
    assert(publish(root,{{model,boundary,binaryAfter,true}},error));
    assert(!publish_metadata(root,{{model,binaryAfter,binaryBefore,true}},error));
    auto effect=root/"mods/A/Model_eff.pak";assert(WarriorRecipe::write(effect,binaryBefore));
    assert(publish(root,{{effect,binaryBefore,binaryAfter,false,true}},error));
    assert(read_file(effect,binaryRead,true) && binaryRead==binaryAfter);
    assert(!publish_metadata(root,{{effect,binaryAfter,binaryBefore,false,true}},error));
    assert(!publish(root,{{model,binaryAfter,binaryBefore,false,true}},error));
    int binaryCalls=0;
    assert(!publish(root,{{model,binaryAfter,binaryBefore,true},{a,named.raw,original}},error,
        [&](const fs::path &from,const fs::path &to){return ++binaryCalls==2?false:atomic_replace(from,to);}));
    assert(read_file(model,binaryRead,true) && binaryRead==binaryAfter && contents(a)==named.raw);

    assert(WarriorRecipe::write(a,"external edit"));
    assert(!publish_metadata(root,files,error));
    assert(contents(a)=="external edit" && contents(b)==named.raw);

    assert(WarriorRecipe::write(a,original));assert(WarriorRecipe::write(b,original));
    int calls=0;
    assert(!publish_metadata(root,files,error,[&](const fs::path &from,const fs::path &to){
        return ++calls==2?false:atomic_replace(from,to);
    }));
    assert(calls==2 && contents(a)==original && contents(b)==original);

    calls=0;
    assert(!publish_metadata(root,files,error,[&](const fs::path &from,const fs::path &to){
        if(++calls==2){assert(WarriorRecipe::write(a,"newer external edit"));return false;}
        return atomic_replace(from,to);
    }));
    assert(contents(a)=="newer external edit" && contents(b)==original);
    assert(error.find("recovery")!=std::string::npos);

    assert(WarriorRecipe::write(a,original));
    assert(!publish_metadata(root,{files[0],files[0]},error));
    assert(contents(a)==original);
    assert(!publish_metadata(root,{{a,original,std::string(WarriorRecipe::Limit+1,'x')}},error));
    assert(contents(a)==original);

    fs::create_directories(root/"outside");
    assert(WarriorRecipe::write(root/"outside/warrior.txt",original));
    std::error_code ec;fs::create_directory_symlink(root/"outside",root/"mods/Escape",ec);
    if(!ec)assert(!publish_metadata(root,{{root/"mods/Escape/warrior.txt",original,named.raw}},error));
    assert(contents(root/"outside/warrior.txt")==original);
    assert(contents(root/"mods/A/model.pak")=="MODEL BYTES");
    auto stage=root/".warrior-builds/new-test/Warrior";
    fs::create_directories(stage);
    for(const char *name:{"warrior.txt","mod.txt",".warrior_built"})assert(WarriorRecipe::write(stage/name,"draft\n"));
    assert(WarriorRecipe::write(root/"mods/enabled.txt","A\nB\n"));
    assert(!publish_new(root,stage,"../escape","A\nB\n",error));
    assert(!publish_new(root,stage,"New","wrong snapshot",error));
    assert(!publish_new(root,stage,"A","A\nB\n",error));
    assert(!publish_new(root,stage,"New","A\nB\n",error,[](const fs::path &,const fs::path &){return false;}));
    assert(fs::is_directory(stage) && !fs::exists(root/"mods/New"));
    assert(contents(root/"mods/enabled.txt")=="A\nB\n");
    // A new transaction has a separate staging job; failed jobs remain recoverable.
    auto stage2=root/".warrior-builds/new-success/Warrior";
    fs::create_directories(stage2);
    for(const char *name:{"warrior.txt","mod.txt",".warrior_built"})assert(WarriorRecipe::write(stage2/name,"draft\n"));
    assert(publish_new(root,stage2,"New","A\nB\n",error));
    assert(contents(root/"mods/enabled.txt")=="A\nB\nNew\n" && fs::is_directory(root/"mods/New"));
    auto linkedStage=root/".warrior-builds/linked-failure/Warrior";
    fs::create_directories(linkedStage);
    for(const char *name:{"warrior.txt","mod.txt",".warrior_built"})assert(WarriorRecipe::write(linkedStage/name,"draft\n"));
    calls=0;const std::string enabled="A\nB\nNew\n";
    assert(!publish_new_linked(root,linkedStage,"Linked",enabled,{{a,original,named.raw}},error,
        [&](const fs::path &from,const fs::path &to){return ++calls==2?false:atomic_replace(from,to);}));
    assert(calls==2 && contents(a)==original && contents(root/"mods/enabled.txt")==enabled);
    assert(fs::is_directory(linkedStage) && !fs::exists(root/"mods/Linked"));
    auto linkedSuccess=root/".warrior-builds/linked-success/Warrior";
    fs::create_directories(linkedSuccess);
    for(const char *name:{"warrior.txt","mod.txt",".warrior_built"})assert(WarriorRecipe::write(linkedSuccess/name,"draft\n"));
    assert(publish_new_linked(root,linkedSuccess,"Linked",enabled,{{a,original,named.raw}},error));
    assert(contents(a)==named.raw && contents(root/"mods/enabled.txt")==enabled+"Linked\n");
    std::cout<<"PASS: native metadata isolation, atomic publication, rollback and external-edit protection\n";
}
