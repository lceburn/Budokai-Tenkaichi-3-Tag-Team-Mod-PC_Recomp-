#include "../src/gs/warrior_native_stock.h"
#include <cassert>
#include <iostream>
int main(int argc,char **argv) {
    assert(argc==4);namespace fs=std::filesystem;
    auto root=fs::u8path(argv[1]),baseline=fs::u8path(argv[2]),out=fs::u8path(argv[3]);
    WarriorRecipe::Document base,next;std::string error;
    assert(WarriorRecipe::read(baseline/"warrior.txt",base.raw));next=base;
    assert(WarriorNativeForms::replace(base,"form1","3,0,Next"));
    assert(WarriorNativeForms::replace(next,"form1","0,0,Base"));
    assert(WarriorNativeStock::supports(base) && WarriorNativeStock::supports(next));
    assert(WarriorNativeStock::generate(root,base,out/"Base",error,{"Base","Next"},"Base"));
    assert(WarriorNativeStock::generate(root,next,out/"Next",error,{"Base","Next"},"Next"));
    for(const char *folder:{"Base","Next"}) {
        WarriorRecipe::Document manifest;assert(WarriorRecipe::read(out/folder/"mod.txt",manifest.raw));
        assert(manifest.get("trans0")==std::string(folder==std::string("Base")?"3,0,2,-1,0,@Next":"0,0,2,-1,0,@Base"));
        for(const auto &entry:fs::recursive_directory_iterator(baseline))if(entry.is_regular_file()) {
            auto relative=fs::relative(entry.path(),baseline);if(relative=="warrior.txt" || relative=="mod.txt")continue;
            std::string expected,actual;
            assert(WarriorNativeBuild::read_file(entry.path(),expected,true) &&
                   WarriorNativeBuild::read_file(out/folder/relative,actual,true) && expected==actual);
        }
    }
    auto invalid=base;assert(WarriorNativeForms::replace(invalid,"form1","1,0,Missing"));
    assert(!WarriorNativeStock::generate(root,invalid,out/"Invalid",error,{"Base","Next"},"Base"));
    assert(!fs::exists(out/"Invalid"));
    auto stripped=WarriorNativeForms::without_links(base);
    assert(stripped.get("form1").empty() && stripped.get("body")==base.get("body"));
    assert(WarriorNativeBuild::metadata_only(stripped,WarriorNativeForms::without_links(next)));
    std::cout<<"PASS native linked-form generation, exact resource preservation and invalid-target rejection\n";
}
