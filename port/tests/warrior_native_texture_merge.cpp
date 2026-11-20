#include "../src/gs/warrior_native_texture_bindings.h"
#include "../src/gs/warrior_native_build.h"
#include <cassert>
#include <iostream>
static WarriorNativePack::Pack load(const std::filesystem::path &p) {
    std::string raw,error;WarriorNativePack::Pack pack;assert(WarriorNativeBuild::read_file(p,raw,true));assert(WarriorNativePack::decode(raw,pack,error));return pack;
}
int main(int argc,char **argv) {
    using namespace WarriorNativeTextureBindings;assert(argc==4);
    auto body=load(std::filesystem::u8path(argv[1])),donor=load(std::filesystem::u8path(argv[2])),reference=load(std::filesystem::u8path(argv[3]));
    std::vector<WarriorNativeParts::Record> records;std::string error;
    assert(WarriorNativeParts::records(donor.entries[2],records,error));std::set<size_t> selected;
    for(size_t i=0;i<records.size();i++)if(records[i].node>=46 && records[i].node<=48)selected.insert(i);
    assert(selected.size()==3);std::string combined;
    if(!merge(body.entries[11],donor.entries[11],donor.entries[2],selected,combined,error)){std::cerr<<error<<"\n";return 1;}
    std::vector<Texture> actual,expected;assert(textures(combined,actual,error));assert(textures(reference.entries[11],expected,error));
    std::cout<<"native textures="<<actual.size()<<" reference="<<expected.size()<<"\n";
    assert(actual.size()==expected.size());
    for(const auto &t:expected)assert(destination(t,actual));
    for(const auto &t:actual)assert(destination(t,expected));
    auto mesh=donor.entries[2];assert(relocate(mesh,donor.entries[11],combined,selected,error));
    auto saved=combined;
    assert(!merge(body.entries[11],std::string(8,'\0'),donor.entries[2],selected,combined,error) && combined==saved);
    assert(!merge(body.entries[11],donor.entries[11],donor.entries[2],{records.size()},combined,error) && combined==saved);
    std::cout<<"PASS: native texture packing matches reference texture content and supports donor head rebinding; invalid inputs preserve output\n";
}
