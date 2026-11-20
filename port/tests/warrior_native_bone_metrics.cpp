#include "../src/gs/warrior_native_fitting.h"
#include "../src/gs/warrior_native_build.h"
#include <cassert>
#include <iostream>
int main(int argc,char **argv) {
    using namespace WarriorNativeFitting;namespace fs=std::filesystem;assert(argc==2);
    unsigned models=0,bounds=0;std::string error;
    for(const auto &p:fs::directory_iterator(fs::u8path(argv[1])))if(p.path().extension()==".pak") {
        std::string raw;WarriorNativePack::Pack pack;
        assert(WarriorNativeBuild::read_file(p.path(),raw,true));assert(WarriorNativePack::decode(raw,pack,error));
        std::vector<WarriorNativeParts::Record> rows;assert(WarriorNativeParts::records(pack.entries[2],rows,error));
        Bone measured;assert(bone(pack.entries[2],16,46,measured,error));
        assert(measured.length>=0 && std::isfinite(measured.length));
        for(const auto &r:rows) {
            float width=-1;
            if(!girth(pack.entries[2],r.node,measured.axis,width,error)){
                std::cerr<<p.path().filename()<<" bone "<<r.node<<": "<<error<<"\n";return 1;
            }
            assert(width>=0 && std::isfinite(width));bounds++;
        }
        Fit fitted;
        for(unsigned node:{9u,10u,13u,14u,19u,20u,25u,33u,34u,40u,46u,47u,48u}) {
            assert(fit(pack.entries[2],pack.entries[2],node,fitted,error));
            assert(fitted.length==1.f && fitted.width==1.f);
        }
        auto scaled=pack.entries[2];
        for(const auto &r:rows) {
            for(unsigned k=0;k<3;k++)scalar(scaled,r.offset+16+k*4,scalar(scaled,r.offset+16+k*4)*1.25f);
            if(r.size>=96)for(unsigned k=0;k<3;k++)scalar(scaled,r.offset+80+k*4,scalar(scaled,r.offset+80+k*4)*.75f);
        }
        assert(fit(scaled,pack.entries[2],46,fitted,error));
        assert(std::fabs(fitted.length-1.25f)<.00001f);
        if(fitted.width!=1.25f)assert(std::fabs(fitted.width-.84f)<.00001f);
        auto previous=measured;assert(!bone(pack.entries[2],999,46,measured,error) && measured.length==previous.length);
        float width=37;assert(!girth(pack.entries[2],46,{{0,2,0}},width,error) && width==37);
        auto corrupted=pack.entries[2];auto neck=rows.front();for(const auto &r:rows)if(r.node==46)neck=r;
        WarriorNativePack::put(corrupted,neck.offset+20,0x7fc00000);
        assert(!bone(corrupted,16,46,measured,error) && measured.length==previous.length);
        fitted.length=37;
        assert(!fit(corrupted,pack.entries[2],46,fitted,error) && fitted.length==37);
        models++;
    }
    assert(models==161);std::cout<<"PASS: native bone metrics and fitting scales across "<<models<<" donors and "<<bounds<<" bounds; invalid inputs preserve results\n";
}
