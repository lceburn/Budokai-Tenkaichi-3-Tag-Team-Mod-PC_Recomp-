#include "../src/gs/warrior_native_hands.h"
#include "../src/gs/warrior_native_build.h"
#include <cassert>
#include <iostream>
static WarriorNativePack::Pack load(const char *path) {
    std::string raw,error;WarriorNativePack::Pack pack;
    assert(WarriorNativeBuild::read_file(std::filesystem::u8path(path),raw,true));
    assert(WarriorNativePack::decode(raw,pack,error));return pack;
}
int main(int argc,char **argv) {
    assert(argc==5);auto body=load(argv[1]),donor=load(argv[2]),gold=load(argv[3]);std::string error;
    WarriorNativePack::Pack result;assert(WarriorNativeHands::compose(body,donor,result,error));
    std::vector<WarriorNativeParts::Record> actual,expected;
    assert(WarriorNativeParts::records(result.entries[2],actual,error));assert(WarriorNativeParts::records(gold.entries[2],expected,error));
    unsigned vertices=0;float maximum=0;
    for(const auto &r:actual)if(WarriorNativeParts::slot(r.node)==7) {
        const WarriorNativeParts::Record *reference;assert(WarriorNativeFitting::find(expected,r.node,reference,error));
        WarriorNativeSeams::Points a,b;assert(WarriorNativeSeams::read(result.entries[2],r,a,error) && WarriorNativeSeams::read(gold.entries[2],*reference,b,error));
        assert(a.offsets.size()==b.offsets.size());vertices+=a.offsets.size();
        for(size_t v=0;v<a.offsets.size();v++)for(unsigned k=0;k<3;k++)
            maximum=std::max(maximum,std::fabs(WarriorNativeFitting::scalar(result.entries[2],a.offsets[v]+k*4)-WarriorNativeFitting::scalar(gold.entries[2],b.offsets[v]+k*4)));
    }
    assert(vertices==736 && maximum<.000005f);
    for(size_t i=0;i<body.entries.size();i++)if(i<2 || i>=12)assert(result.entries[i]==body.entries[i]);
    auto previous=result;assert(!WarriorNativeAssembly::replace(body,donor,donor.entries[2],{999},result,error) && result.entries==previous.entries);
    std::string packed;assert(WarriorNativePack::encode(result,packed,error));
    auto out=std::filesystem::u8path(argv[4]);assert(!std::filesystem::exists(out));assert(WarriorRecipe::write(out,packed));
    std::cout<<"PASS: complete native hand model emitted; 736 assembled vertices match reference; unrelated resources and invalid-output preservation verified\n";
}
