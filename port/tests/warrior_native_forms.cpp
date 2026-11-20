#include "../src/gs/warrior_native_build.h"
#include <cassert>
#include <iostream>
int main(int argc,char **argv) {
    using namespace WarriorNativeForms;using WarriorRecipe::Document;
    assert(argc==3);auto base=std::filesystem::u8path(argv[1]),reference=std::filesystem::u8path(argv[2]);
    Document before,edited,manifest;std::string expected,error;
    assert(WarriorRecipe::read(base/"warrior.txt",before.raw));assert(WarriorRecipe::read(base/"mod.txt",manifest.raw));
    assert(WarriorRecipe::read(reference/"warrior.txt",edited.raw));assert(WarriorRecipe::read(reference/"mod.txt",expected));
    assert(WarriorNativeBuild::nonvisual_only(before,edited));
    assert(update(before,edited,manifest,{"Base","Next"},"Base",error));
    auto normalize=[](std::string s){size_t n=0;while((n=s.find("\r\n",n))!=std::string::npos)s.replace(n,2,"\n");return s;};
    assert(normalize(manifest.raw)==normalize(expected));
    for(auto invalid:{"6,0,Next","1,1,Next","1,0,Base","1,0,Unknown","1,0,../Next","1,0,Next/","1,0,Next\n"}) {
        std::string value;assert(!record(invalid,0,{"Base","Next"},"Base",value,error));
    }
    std::string value;assert(record("",3,{},"Base",value,error) && value=="0,1,1,-1,0,-1");
    auto original=manifest.raw;auto invalid=edited;replace(invalid,"form4","9,0,Next");
    assert(!update(before,invalid,manifest,{"Base","Next"},"Base",error) && manifest.raw==original);
    assert(update(edited,before,manifest,{"Base","Next"},"Base",error));
    Document stock;assert(WarriorRecipe::read(base/"mod.txt",stock.raw));assert(normalize(manifest.raw)==normalize(stock.raw));
    std::cout<<"PASS: all four native form records match reference, link removal and invalid-link rollback\n";
}
