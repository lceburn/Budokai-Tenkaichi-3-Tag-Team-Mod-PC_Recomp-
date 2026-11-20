#include "../src/gs/warrior_native_body_model.h"
#include "../src/gs/warrior_native_build.h"
#include <cassert>
#include <iostream>
static WarriorNativePack::Pack load(const char *path) {
    std::string raw,error;WarriorNativePack::Pack pack;
    assert(WarriorNativeBuild::read_file(std::filesystem::u8path(path),raw,true));
    assert(WarriorNativePack::decode(raw,pack,error));return pack;
}
int main(int argc,char **argv) {
    assert(argc==8);auto body=load(argv[1]),nappa=load(argv[2]),vegeta=load(argv[3]),kid=load(argv[4]),gold=load(argv[5]);
    bool whole=std::string(argv[6])=="whole";std::vector<WarriorNativeBodyModel::Selection> selections;
    if(whole)selections.push_back({&nappa,{3,4,8,9}});
    selections.push_back({&vegeta,whole?std::set<unsigned>{5,6,10}:std::set<unsigned>{5,6}});
    selections.push_back({&kid,{7}});
    WarriorNativePack::Pack result;std::string error;
    if(!WarriorNativeBodyModel::compose(body,selections,result,error)){std::cerr<<error<<"\n";return 1;}
    std::vector<WarriorNativeParts::Record> rows,reference;assert(WarriorNativeParts::records(result.entries[2],rows,error));assert(WarriorNativeParts::records(gold.entries[2],reference,error));
    size_t count=0;float maximum=0;
    for(const auto &r:rows) {
        auto slot=WarriorNativeParts::slot(r.node);if(whole?(slot<3 || slot>10):(slot<5 || slot>7))continue;
        const WarriorNativeParts::Record *target;assert(WarriorNativeFitting::find(reference,r.node,target,error));
        assert(result.entries[2].substr(r.offset+16,48)==gold.entries[2].substr(target->offset+16,48));
        WarriorNativeSeams::Points a,b;assert(WarriorNativeSeams::read(result.entries[2],r,a,error));assert(WarriorNativeSeams::read(gold.entries[2],*target,b,error));assert(a.offsets.size()==b.offsets.size());
        for(size_t i=0;i<a.offsets.size();i++)for(unsigned k=0;k<3;k++)maximum=std::max(maximum,std::fabs(WarriorNativeFitting::scalar(result.entries[2],a.offsets[i]+4*k)-WarriorNativeFitting::scalar(gold.entries[2],b.offsets[i]+4*k)));
        count+=a.offsets.size();
    }
    std::cout<<"vertices="<<count<<" maximum="<<maximum<<"\n";assert(count>1000 && maximum<.00001f);
    auto old=result;selections.push_back({&kid,{7}});assert(!WarriorNativeBodyModel::compose(body,selections,result,error));assert(old.entries==result.entries);
    std::string raw;assert(WarriorNativePack::encode(result,raw,error));assert(!std::filesystem::exists(argv[7]));assert(WarriorRecipe::write(std::filesystem::u8path(argv[7]),raw));
    std::cout<<"PASS combined native fitting and replacement, reference geometry and binds; conflicting selections preserve output\n";
}
