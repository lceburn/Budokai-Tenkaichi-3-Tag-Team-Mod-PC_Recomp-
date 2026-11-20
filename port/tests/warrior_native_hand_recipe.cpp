#include "../src/gs/warrior_native_stock.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
int main(int argc,char **argv) {
    assert(argc==4);namespace fs=std::filesystem;
    auto root=fs::u8path(argv[1]),gold=fs::u8path(argv[2]),out=fs::u8path(argv[3]);
    WarriorRecipe::Document recipe;std::string error;
    assert(WarriorRecipe::read(gold/"warrior.txt",recipe.raw));assert(!WarriorNativeStock::supports(recipe));
    setenv("BT3_WARRIOR_NATIVE_HAND_TEST","1",1);assert(WarriorNativeStock::supports(recipe));
    if(!WarriorNativeStock::generate(root,recipe,out,error)){std::cerr<<error<<"\n";return 1;}
    unsigned models=0,resources=0;float maximum=0;
    std::string list;assert(WarriorRecipe::read(gold/".warrior_built",list));std::istringstream lines(list);std::string name;
    while(std::getline(lines,name)) {
        if(!name.empty() && name.back()=='\r')name.pop_back();if(name.empty() || name[0]=='#')continue;
        std::string a,b;assert(WarriorNativeBuild::read_file(gold/fs::u8path(name),a,true));
        assert(WarriorNativeBuild::read_file(out/fs::u8path(name),b,true));
        if(!WarriorNativeBuild::model_name(name)){assert(a==b);resources++;continue;}
        WarriorNativePack::Pack reference,native;
        assert(WarriorNativePack::decode(a,reference,error) && WarriorNativePack::decode(b,native,error));
        std::vector<WarriorNativeParts::Record> actual,expected;
        assert(WarriorNativeParts::records(native.entries[2],actual,error) && WarriorNativeParts::records(reference.entries[2],expected,error));
        unsigned vertices=0;
        for(const auto &r:actual)if(WarriorNativeParts::slot(r.node)==7) {
            const WarriorNativeParts::Record *target;assert(WarriorNativeFitting::find(expected,r.node,target,error));
            WarriorNativeSeams::Points p,q;
            assert(WarriorNativeSeams::read(native.entries[2],r,p,error) && WarriorNativeSeams::read(reference.entries[2],*target,q,error));
            assert(p.offsets.size()==q.offsets.size());vertices+=p.offsets.size();
            for(size_t v=0;v<p.offsets.size();v++)for(unsigned k=0;k<3;k++)
                maximum=std::max(maximum,std::fabs(WarriorNativeFitting::scalar(native.entries[2],p.offsets[v]+k*4)-WarriorNativeFitting::scalar(reference.entries[2],q.offsets[v]+k*4)));
        }
        assert(vertices>250 && ((b.size()+2047)&~size_t(2047))<=0xE6800);
        for(size_t i=17;i<reference.entries.size();i++)if(i!=22)assert(native.entries[i]==reference.entries[i]);
        models++;
    }
    std::cout<<"hand variant max_error="<<maximum<<"\n";
    assert(models==6 && resources==5 && maximum<.00001f);
    std::cout<<"PASS: all six native hand recipe variants match reference geometry and supporting resources\n";
}
