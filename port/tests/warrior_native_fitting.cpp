#include "../src/gs/warrior_native_fitting.h"
#include "../src/gs/warrior_native_build.h"
#include <cassert>
#include <iostream>
static WarriorNativePack::Pack load(const std::filesystem::path &p) {
    std::string raw,error;WarriorNativePack::Pack pack;assert(WarriorNativeBuild::read_file(p,raw,true));assert(WarriorNativePack::decode(raw,pack,error));return pack;
}
static WarriorNativeParts::Record face(const std::string &mesh) {
    std::string error;std::vector<WarriorNativeParts::Record> records;assert(WarriorNativeParts::records(mesh,records,error));
    for(const auto &r:records)if(r.node==48)return r;assert(false);return {};
}
int main(int argc,char **argv) {
    assert(argc==4);auto source=load(std::filesystem::u8path(argv[1])),body=load(std::filesystem::u8path(argv[2])),reference=load(std::filesystem::u8path(argv[3]));
    auto mesh=source.entries[2],original=mesh;std::string error;assert(WarriorNativeFitting::face(mesh,body.entries[2],error));
    auto r=face(mesh),gold=face(reference.entries[2]);assert(r.size==gold.size);
    unsigned vertices=0;float maximum=0;std::set<size_t> allowed;
    for(size_t at=96;at+92<=r.size;at+=4)if(WarriorNativePack::word(mesh,r.offset+at)==0x6c058000) {
        auto count=WarriorNativePack::word(mesh,r.offset+at+52)&0x7fff;assert(at+92+count*48<=r.size);
        for(unsigned v=0;v<count;v++) {
            vertices++;
            for(unsigned k=0;k<3;k++) {
                size_t p=at+88+v*48+k*4;
                float delta=std::fabs(WarriorNativeFitting::scalar(mesh,r.offset+p)-WarriorNativeFitting::scalar(reference.entries[2],gold.offset+p));
                maximum=std::max(maximum,delta);
                for(size_t b=0;b<4;b++)allowed.insert(r.offset+p+b);
            }
        }
    }
    for(size_t n=r.offset+16;n<r.offset+76;n++)allowed.insert(n);
    for(size_t n=0;n<mesh.size();n++)if(!allowed.count(n))assert(mesh[n]==original[n]);
    assert(vertices>100 && maximum<0.000002f);
    auto same=mesh;assert(WarriorNativeFitting::face(mesh,body.entries[2],error) && mesh==same);
    auto bad=body;auto target=face(bad.entries[2]);WarriorNativePack::put(bad.entries[2],target.offset+20,0x7fc00000);
    assert(!WarriorNativeFitting::face(mesh,bad.entries[2],error) && mesh==same);
    std::cout<<"PASS: "<<vertices<<" cross-body face vertices match reference, max error="<<maximum<<"; all other geometry preserved; invalid target preserves result\n";
}
