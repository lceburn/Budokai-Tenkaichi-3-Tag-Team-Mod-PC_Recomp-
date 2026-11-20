#include "../src/gs/warrior_native_texture_bindings.h"
#include "../src/gs/warrior_native_build.h"
#include <cassert>
#include <iostream>
static WarriorNativePack::Pack load(const std::filesystem::path &p) {
    std::string data,error;WarriorNativePack::Pack pack;assert(WarriorNativeBuild::read_file(p,data,true));assert(WarriorNativePack::decode(data,pack,error));return pack;
}
int main(int argc,char **argv) {
    using namespace WarriorNativeTextureBindings;assert(argc==3);auto base=load(std::filesystem::u8path(argv[1])),reference=load(std::filesystem::u8path(argv[2]));
    std::vector<WarriorNativeParts::Record> records,expected;std::string error;
    assert(WarriorNativeParts::records(base.entries[2],records,error));assert(WarriorNativeParts::records(reference.entries[2],expected,error));
    std::set<size_t> selected;
    for(size_t i=0;i<records.size();i++) {
        bool head=false;
        for(int ancestor=int(i);ancestor>=0;ancestor=records[size_t(ancestor)].parent)
            if(records[size_t(ancestor)].node==46)head=true;
        if(!head)selected.insert(i);
    }
    auto mesh=base.entries[2];
    if(!relocate(mesh,base.entries[11],reference.entries[11],selected,error)){std::cerr<<error<<"\n";return 1;}
    unsigned compared=0;
    for(auto index:selected) {
        const auto &r=records[index];const WarriorNativeParts::Record *match=nullptr;
        for(const auto &e:expected)if(e.node==r.node)match=&e;
        assert(match && r.size==match->size);
        // The reference also rebuilds descriptors and canonicalizes signed
        // zero vertex floats. This stage changes address fields only.
        if(r.size<=96){compared++;continue;}
        std::set<size_t> addressBytes;
        for(size_t at=96;at+92<=r.size;at+=4)if(WarriorNativePack::word(base.entries[2],r.offset+at)==0x6c058000) {
            assert(WarriorNativePack::word(reference.entries[2],match->offset+at)==0x6c058000);
            for(size_t field:{20u,24u,40u}) {
                auto actual=WarriorNativePack::word(mesh,r.offset+at+field),expectedWord=WarriorNativePack::word(reference.entries[2],match->offset+at+field);
                if(actual!=expectedWord){std::cerr<<"DIFFERENT material node="<<r.node<<" offset="<<at+field<<"\n";return 1;}
                for(size_t b=0;b<4;b++)addressBytes.insert(at+field+b);
            }
        }
        for(size_t at=0;at<r.size;at++)if(!addressBytes.count(at))assert(mesh[r.offset+at]==base.entries[2][r.offset+at]);
        compared++;
    }
    auto before=mesh;assert(!relocate(mesh,base.entries[11],std::string(8,'\0'),selected,error) && mesh==before);
    assert(!relocate(mesh,base.entries[11],reference.entries[11],{records.size()},error) && mesh==before);
    std::cout<<"PASS: reference material bindings match across "<<compared<<" retained records; all other mesh bytes preserved; invalid edits preserve mesh\n";
}
