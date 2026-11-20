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
    assert(argc==6);auto body=load(argv[1]),donor=load(argv[2]),reference=load(argv[3]);
    unsigned slot=std::atoi(argv[4]);std::string error;WarriorNativePack::Pack result;
    if(!WarriorNativeBodyModel::compose(body,donor,{slot},result,error)){std::cerr<<error<<"\n";return 1;}
    std::vector<WarriorNativeParts::Record> rows,gold;
    assert(WarriorNativeParts::records(result.entries[2],rows,error) && WarriorNativeParts::records(reference.entries[2],gold,error));
    unsigned parts=0,vertices=0;float maximum=0,normals=0;
    for(const auto &r:rows)if(WarriorNativeParts::slot(r.node)==int(slot)) {
        const WarriorNativeParts::Record *target;assert(WarriorNativeFitting::find(gold,r.node,target,error));
        assert(result.entries[2].substr(r.offset+16,48)==reference.entries[2].substr(target->offset+16,48));
        WarriorNativeSeams::Points a,b;assert(WarriorNativeSeams::read(result.entries[2],r,a,error) && WarriorNativeSeams::read(reference.entries[2],*target,b,error));
        assert(a.offsets.size()==b.offsets.size());parts++;vertices+=a.offsets.size();
        for(size_t v=0;v<a.offsets.size();v++)for(unsigned k=0;k<3;k++) {
            maximum=std::max(maximum,std::fabs(WarriorNativeFitting::scalar(result.entries[2],a.offsets[v]+k*4)-WarriorNativeFitting::scalar(reference.entries[2],b.offsets[v]+k*4)));
            normals=std::max(normals,std::fabs(WarriorNativeFitting::scalar(result.entries[2],a.offsets[v]+16+k*4)-WarriorNativeFitting::scalar(reference.entries[2],b.offsets[v]+16+k*4)));
        }
    }
    std::cout<<"parts="<<parts<<" vertices="<<vertices<<" max_error="<<maximum<<" normal_error="<<normals<<std::endl;
    assert(parts>0 && vertices>100 && maximum<.00001f && normals<.00001f);
    for(size_t i=0;i<body.entries.size();i++)if(i<2 || i>=12)assert(result.entries[i]==body.entries[i]);
    auto previous=result;assert(!WarriorNativeBodyModel::compose(body,donor,{999},result,error) && result.entries==previous.entries);
    std::string packed;assert(WarriorNativePack::encode(result,packed,error));
    auto out=std::filesystem::u8path(argv[5]);assert(!std::filesystem::exists(out));assert(WarriorRecipe::write(out,packed));
    std::cout<<"PASS: complete native body-part model matches reference geometry, normals and binds; unrelated data preserved\n";
}
