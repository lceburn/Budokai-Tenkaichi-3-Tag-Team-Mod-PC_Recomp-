#include "../src/gs/warrior_native_head.h"
#include "../src/gs/warrior_native_build.h"
#include <cassert>
#include <iostream>
static WarriorNativePack::Pack load(const char *path) {
    std::string raw,error;WarriorNativePack::Pack pack;
    assert(WarriorNativeBuild::read_file(std::filesystem::u8path(path),raw,true));
    assert(WarriorNativePack::decode(raw,pack,error));return pack;
}
static WarriorNativeParts::Record node(const std::string &mesh,unsigned id) {
    std::string error;std::vector<WarriorNativeParts::Record> rows;
    assert(WarriorNativeParts::records(mesh,rows,error));
    for(auto r:rows)if(r.node==id)return r;assert(false);return {};
}
int main(int argc,char **argv) {
    assert(argc==4);auto body=load(argv[1]),head=load(argv[2]),reference=load(argv[3]);
    WarriorNativePack::Pack result;std::string error;
    if(!WarriorNativeHead::compose(body,head,result,error)){std::cerr<<error<<"\n";return 1;}
    std::vector<WarriorNativeTextureBindings::Texture> retained,combined;
    assert(WarriorNativeTextureBindings::textures(body.entries[11],retained,error));
    assert(WarriorNativeTextureBindings::textures(result.entries[11],combined,error));
    for(const auto &t:retained)assert(WarriorNativeTextureBindings::destination(t,combined));
    unsigned indexed=0;for(const auto &t:combined)indexed+=t.psm!=0 && !t.auxiliary;
    assert(static_cast<unsigned char>(result.entries[2][0x4e])==0);
    assert(static_cast<unsigned char>(result.entries[2][0x4f])==indexed);
    assert(static_cast<unsigned char>(result.entries[2][0x50])==indexed);
    assert(static_cast<unsigned char>(result.entries[2][0x51])==combined.size()-indexed);
    for(unsigned id:{46u,48u}) {
        auto a=node(result.entries[2],id),b=node(reference.entries[2],id);
        WarriorNativeSeams::Points actual,gold;
        assert(WarriorNativeSeams::read(result.entries[2],a,actual,error));
        assert(WarriorNativeSeams::read(reference.entries[2],b,gold,error));
        assert(actual.offsets.size()==gold.offsets.size());float maximum=0;
        for(size_t v=0;v<actual.offsets.size();v++)for(unsigned k=0;k<3;k++)
            maximum=std::max(maximum,std::fabs(WarriorNativeFitting::scalar(result.entries[2],actual.offsets[v]+4*k)-
                                             WarriorNativeFitting::scalar(reference.entries[2],gold.offsets[v]+4*k)));
        std::cout<<"node="<<id<<" vertices="<<actual.offsets.size()<<" max_error="<<maximum<<"\n";
        assert(maximum<.000002f);
        assert(result.entries[2].substr(a.offset+16,48)==reference.entries[2].substr(b.offset+16,48));
    }
    auto sentinel=result.entries[2];auto bad=head;auto neck=node(bad.entries[2],46);
    WarriorNativePack::put(bad.entries[2],neck.offset+20,0x7fc00000);
    assert(!WarriorNativeHead::compose(body,bad,result,error) && result.entries[2]==sentinel);
    std::cout<<"PASS: assembled native cross-body neck and face match reference; invalid donor preserves output\n";
}
