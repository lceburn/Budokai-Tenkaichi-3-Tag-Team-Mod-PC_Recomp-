#include "../src/gs/warrior_native_stock.h"
#include "../src/gs/warrior_native_regenerate.h"
#include "menu/warrior_fields.h"
#include <cassert>
#include <iostream>
int main(int argc,char **argv) {
    using namespace WarriorNativeStock;
    assert(argc==3);auto data=fs::u8path(argv[1]),out=fs::absolute(fs::u8path(argv[2]));
    Document base{"name=Portrait Probe\nbody=Goku (End)\nmoves=Goku (End)\n"};
    auto edited=base;edited.raw+="portrait=Vegeta (Scouter)\n";std::string error;
    assert(zw_native_editable(ZW_PORTRAIT));
    assert(generate(data,base,out/"base",error));
    assert(generate(data,edited,out/"edited",error));
    WarriorNativeSources::Model donor;assert(WarriorNativeSources::model(data,"Vegeta (Scouter)",donor,error));
    const int ids[]={760+donor.id,7+donor.id*2,8+donor.id*2};
    std::vector<std::string> names;assert(WarriorNativeRegenerate::manifest(out/"base",names));
    unsigned portrait=0;
    for(const auto &name:names) {
        std::string before,after;assert(WarriorNativeBuild::read_file(out/"base"/name,before,true));
        assert(WarriorNativeBuild::read_file(out/"edited"/name,after,true));
        int role=WarriorNativeRegenerate::role(name);
        if(role>=11) {
            std::string expected;assert(WarriorNativeSources::asset(data,ids[portrait++],expected,error));assert(after==expected);
        }else if(role<=8) {
            WarriorNativePack::Pack original,changed;
            assert(WarriorNativePack::decode(before,original,error) && WarriorNativePack::decode(after,changed,error));
            assert(changed.entries[0]==donor.pack.entries[0]);
            assert(original.entries.size()==changed.entries.size());
            for(size_t i=1;i<original.entries.size();i++)assert(original.entries[i]==changed.entries[i]);
        }else assert(before==after); // no moves, attacks or effects change
    }
    assert(portrait==3);
    auto library=out/"library",saved=library/"mods/Probe";
    for(const auto &entry:fs::recursive_directory_iterator(out/"base"))if(entry.is_regular_file()) {
        auto target=saved/entry.path().lexically_relative(out/"base");
        fs::create_directories(target.parent_path());fs::copy_file(entry.path(),target);
    }
    std::vector<WarriorNativeBuild::FileEdit> files;
    assert(WarriorNativeRegenerate::plan(saved,out/"edited",files,error));
    assert(files.size()==12); // eight model HUD sheets, three image resources and recipe
    unsigned commits=0;
    assert(!WarriorNativeBuild::publish(library,files,error,[&](const fs::path &from,const fs::path &to){
        return ++commits!=10 && WarriorNativeBuild::atomic_replace(from,to);
    }));
    for(const auto &file:files){std::string current;assert(WarriorNativeBuild::read_file(file.path,current,true) && current==file.original);}
    assert(WarriorNativeBuild::publish(library,files,error));
    files.clear();assert(WarriorNativeRegenerate::plan(saved,out/"base",files,error));
    assert(WarriorNativeBuild::publish(library,files,error)); // Default restores body portrait
    auto invalid=base;invalid.raw+="portrait=Not a donor\n";
    assert(!generate(data,invalid,out/"invalid",error));
    std::cout<<"PASS: selected portrait/HUD/picker donor, unchanged models/moves, atomic rollback, save and Default restore\n";
}
