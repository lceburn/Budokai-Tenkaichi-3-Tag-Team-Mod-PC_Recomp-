#include "../src/gs/warrior_native_parts.h"
#include "../src/gs/warrior_native_build.h"
#include <cassert>
#include <iostream>
int main(int argc,char **argv) {
    using namespace WarriorNativeParts;std::string error;std::vector<Record> result;
    std::string mesh(0x110,'\0');WarriorNativePack::put(mesh,0,0x6c646d70);WarriorNativePack::put(mesh,0x6c,0x90);
    WarriorNativePack::put(mesh,0x90,64);WarriorNativePack::put(mesh,0xd0,64);mesh[0xd6]=1;mesh[0xda]=46;
    assert(records(mesh,result,error) && result.size()==2 && result[1].parent==0 && slot(result[1].node)==1);
    auto previous=result;auto invalid=mesh;WarriorNativePack::put(invalid,0x6c,0xfffffff0);
    assert(!records(invalid,result,error) && result.size()==previous.size());
    invalid=mesh;WarriorNativePack::put(invalid,0x90,0xffffffff);assert(!records(invalid,result,error));
    invalid=mesh;invalid[0x94]=2;assert(!records(invalid,result,error));
    invalid=mesh;invalid[0xd6]=0;assert(!records(invalid,result,error));
    assert(slot(21)==7 && slot(45)==7 && slot(0)==-1 && slot(71)==-1);
    assert(argc==2);unsigned models=0,total=0,maximum=0;
    for(const auto &p:std::filesystem::directory_iterator(std::filesystem::u8path(argv[1])))if(p.path().extension()==".pak") {
        std::string raw;WarriorNativePack::Pack pack;
        assert(WarriorNativeBuild::read_file(p.path(),raw,true));assert(WarriorNativePack::decode(raw,pack,error));
        if(pack.entries.size()<3 || !records(pack.entries[2],result,error)) {
            std::cerr<<p.path().filename()<<": "<<error<<"\n";return 1;
        }
        for(size_t n=0;n<result.size();n++)assert(result[n].parent< int(n));
        models++;total+=unsigned(result.size());maximum=std::max(maximum,unsigned(result.size()));
    }
    assert(models==161);std::cout<<"PASS: native hierarchy reader across "<<models<<" donors, "<<total<<" records, maximum "<<maximum<<"; malformed records preserve output\n";
}
