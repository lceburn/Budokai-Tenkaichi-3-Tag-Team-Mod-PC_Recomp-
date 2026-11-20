#include "../src/gs/warrior_native_seams.h"
#include "../src/gs/warrior_native_build.h"
#include <cassert>
#include <iostream>
int main(int argc,char **argv) {
    using namespace WarriorNativeSeams;using WarriorNativePack::put;
    std::string error;
    const Vec3 corners[]={{0,0,0},{1,0,0},{1,1,0},{0,1,0},{.5f,.5f,0}};
    size_t bytes=((4*236-8)+15)&~size_t(15);
    std::string mesh(96+16+bytes,'\0');put(mesh,96,0x60000000u|unsigned(bytes/16));
    for(unsigned t=0;t<4;t++) {
        size_t at=104+t*236;put(mesh,at,0x6c058000);put(mesh,at+52,3);
        unsigned triangle[]={t,(t+1)%4,4};
        for(unsigned v=0;v<3;v++)for(unsigned k=0;k<3;k++)
            WarriorNativeFitting::scalar(mesh,at+88+v*48+k*4,corners[triangle[v]][k]);
    }
    WarriorNativeParts::Record record{0,mesh.size(),0,1,46,0,-1};Points points;
    assert(read(mesh,record,points,error));assert(points.positions.size()==5 && points.offsets.size()==12);
    unsigned boundary=0;for(bool b:points.boundary)boundary+=b;assert(boundary==4);
    for(size_t i=0;i<points.positions.size();i++)
        assert(points.boundary[i]!=(points.positions[i]==corners[4]));
    auto bad=mesh;put(bad,156,32767);assert(!read(bad,record,points,error) && points.offsets.size()==12);
    bad=mesh;put(bad,192,0x7fc00000);assert(!read(bad,record,points,error) && points.offsets.size()==12);
    auto original=mesh;mesh+=original;
    WarriorNativeParts::Record parent{original.size(),original.size(),0,1,16,0,-1};
    for(size_t offset:points.offsets)WarriorNativeFitting::scalar(mesh,parent.offset+offset+8,.1f);
    auto before=mesh;assert(stitch(mesh,record,parent,&record,error) && mesh==before);
    assert(stitch(mesh,record,parent,nullptr,error));
    assert(mesh.substr(parent.offset)==before.substr(parent.offset));
    for(size_t v=0;v<points.offsets.size();v++) {
        bool edge=points.boundary[points.indices[v]];
        assert(std::fabs(WarriorNativeFitting::scalar(mesh,points.offsets[v]+8)-(edge?.1f:0.f))<.000001f);
    }
    // Nearby interior vertices receive the reference's bounded falloff.
    mesh=before;
    for(size_t offset:points.offsets)for(unsigned k=0;k<2;k++) {
        float value=WarriorNativeFitting::scalar(mesh,offset+k*4)*.4f;
        WarriorNativeFitting::scalar(mesh,offset+k*4,value);
        WarriorNativeFitting::scalar(mesh,parent.offset+offset+k*4,value);
    }
    assert(stitch(mesh,record,parent,nullptr,error));
    float attenuation=1.f-2.f*std::sqrt(.08f);attenuation*=attenuation;
    for(size_t v=0;v<points.offsets.size();v++)if(!points.boundary[points.indices[v]])
        assert(std::fabs(WarriorNativeFitting::scalar(mesh,points.offsets[v]+8)-.1f*attenuation)<.000001f);
    before=mesh;parent.size=mesh.size();assert(!stitch(mesh,record,parent,nullptr,error) && mesh==before);
    assert(argc==2);unsigned models=0,records=0,vertices=0;
    for(const auto &p:std::filesystem::directory_iterator(std::filesystem::u8path(argv[1])))if(p.path().extension()==".pak") {
        std::string raw;WarriorNativePack::Pack pack;std::vector<WarriorNativeParts::Record> rows;
        assert(WarriorNativeBuild::read_file(p.path(),raw,true));assert(WarriorNativePack::decode(raw,pack,error));
        assert(WarriorNativeParts::records(pack.entries[2],rows,error));
        for(const auto &r:rows) {
            if(!read(pack.entries[2],r,points,error)){std::cerr<<p.path()<<" node "<<r.node<<": "<<error<<"\n";return 1;}
            assert(points.offsets.size()==points.indices.size());vertices+=points.offsets.size();records++;
        }
        models++;
    }
    assert(models==161);std::cout<<"PASS: native seam boundaries, shared edges and failed-output preservation; "<<models<<" donors, "<<records<<" records, "<<vertices<<" vertices\n";
}
