#include "../src/gs/warrior_native_hands.h"
#include "../src/gs/warrior_native_build.h"
#include <cassert>
#include <iostream>
static std::string load(const char *path) {
    std::string raw,error;WarriorNativePack::Pack pack;
    assert(WarriorNativeBuild::read_file(std::filesystem::u8path(path),raw,true));
    assert(WarriorNativePack::decode(raw,pack,error));return pack.entries[2];
}
int main(int argc,char **argv) {
    assert(argc==4);auto body=load(argv[1]),source=load(argv[2]),gold=load(argv[3]);
    std::string mesh,error;assert(WarriorNativeHands::fit(body,source,mesh,error));
    std::vector<WarriorNativeParts::Record> rows,target;
    assert(WarriorNativeParts::records(source,rows,error));assert(WarriorNativeParts::records(gold,target,error));
    unsigned parts=0,vertices=0;float maximum=0;
    for(const auto &r:rows) {
        if(WarriorNativeParts::slot(r.node)!=7) {
            assert(mesh.substr(r.offset,r.size)==source.substr(r.offset,r.size));continue;
        }
        const WarriorNativeParts::Record *reference;
        assert(WarriorNativeFitting::find(target,r.node,reference,error));
        assert(mesh.substr(r.offset+16,48)==gold.substr(reference->offset+16,48));
        WarriorNativeSeams::Points actual,expected;
        assert(WarriorNativeSeams::read(mesh,r,actual,error));assert(WarriorNativeSeams::read(gold,*reference,expected,error));
        assert(actual.offsets.size()==expected.offsets.size());
        // Require a valid drawing enclosure of the final stitched geometry;
        // reference vertex and bind-coordinate parity is checked separately.
        for(auto vertex:actual.offsets)for(unsigned k=0;k<3;k++) {
            float center=WarriorNativeFitting::scalar(mesh,r.offset+64+k*4);
            float extent=WarriorNativeFitting::scalar(mesh,r.offset+80+k*4);
            assert(std::fabs(WarriorNativeFitting::scalar(mesh,vertex+k*4)-center)<=extent+.00001f);
        }
        for(size_t v=0;v<actual.offsets.size();v++)for(unsigned k=0;k<3;k++)
            maximum=std::max(maximum,std::fabs(WarriorNativeFitting::scalar(mesh,actual.offsets[v]+k*4)-WarriorNativeFitting::scalar(gold,expected.offsets[v]+k*4)));
        vertices+=actual.offsets.size();parts++;
    }
    assert(parts==22 && vertices==736 && maximum<.000005f);
    auto previous=mesh;assert(!WarriorNativeSeams::anchors(mesh,body,21,error,0.f) && mesh==previous);
    assert(!WarriorNativeSeams::anchors(mesh,body,21,error,INFINITY) && mesh==previous);
    auto invalid=source;WarriorNativePack::put(invalid,0,0);
    assert(!WarriorNativeHands::fit(body,invalid,mesh,error) && mesh==previous);
    std::cout<<"PASS: "<<parts<<" native hand/finger records and "<<vertices<<" vertices match reference; max_error="<<maximum<<"; unrelated geometry and failed outputs preserved\n";
}
