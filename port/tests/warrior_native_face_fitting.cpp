#include "../src/gs/warrior_native_stock.h"
#include <cassert>
#include <iostream>
static std::string wrap(const std::string &stream) {
    std::string mesh(0xf0,'\0');WarriorNativePack::put(mesh,0,0x6c646d70);
    WarriorNativePack::put(mesh,0x6c,0x90);WarriorNativePack::put(mesh,0x90,unsigned(stream.size()+96));
    mesh[0x96]=1;mesh[0x9a]=48;mesh+=stream;return mesh;
}
int main(int argc,char **argv) {
    assert(argc==2);auto root=std::filesystem::u8path(argv[1]);std::string error;
    unsigned streams=0,vertices=0;
    for(const char *bodyName:{"Krillin","Goku (End)"})for(const char *headName:{"Vegeta Super Saiyan God","Goku (Early)","Gohan Beast","Android #18"}) {
        WarriorNativeSources::Model body,head;
        assert(WarriorNativeSources::model(root,bodyName,body,error));
        assert(WarriorNativeSources::model(root,headName,head,error));
        std::vector<WarriorNativeParts::Record> a,b;
        assert(WarriorNativeParts::records(body.pack.entries[2],a,error));
        assert(WarriorNativeParts::records(head.pack.entries[2],b,error));
        const WarriorNativeParts::Record *to,*from;
        assert(WarriorNativeFitting::find(a,48,to,error) && WarriorNativeFitting::find(b,48,from,error));
        for(size_t i=3;i<11;i++) {
            auto before=head.pack.entries[i],after=before;
            assert(WarriorNativeFitting::face_stream(after,head.pack.entries[2],body.pack.entries[2],error));
            if(before.empty()){assert(after.empty());continue;}
            assert(before.size()==after.size());auto original=wrap(before),fitted=wrap(after);
            std::vector<WarriorNativeParts::Record> records;assert(WarriorNativeParts::records(original,records,error));
            WarriorNativeSeams::Points p,q;
            assert(WarriorNativeSeams::read(original,records[0],p,error));
            assert(WarriorNativeSeams::read(fitted,records[0],q,error));assert(p.offsets==q.offsets);
            auto expected=original;
            for(auto offset:p.offsets)for(unsigned k=0;k<3;k++) {
                float x=WarriorNativeFitting::scalar(original,offset+4*k);
                float src=WarriorNativeFitting::scalar(head.pack.entries[2],from->offset+16+4*k);
                float dst=WarriorNativeFitting::scalar(body.pack.entries[2],to->offset+16+4*k);
                bool sameAnchor=true;for(unsigned axis=0;axis<3;axis++)sameAnchor&=WarriorNativeFitting::scalar(head.pack.entries[2],from->offset+16+4*axis)==WarriorNativeFitting::scalar(body.pack.entries[2],to->offset+16+4*axis);
                float value=sameAnchor?x:(x-src)+dst;
                if(WarriorNativeFitting::scalar(fitted,offset+4*k)!=value){std::cerr<<bodyName<<" / "<<headName<<" stream="<<i<<" vertex="<<offset<<" axis="<<k<<" expected="<<value<<" actual="<<WarriorNativeFitting::scalar(fitted,offset+4*k)<<" src="<<src<<" dst="<<dst<<"\n";return 1;}
                WarriorNativeFitting::scalar(expected,offset+4*k,value);
            }
            assert(expected==fitted); // All topology, weights, normals, UVs and material commands stay intact.
            auto same=before;assert(WarriorNativeFitting::face_stream(same,head.pack.entries[2],head.pack.entries[2],error) && same==before);
            vertices+=p.offsets.size();++streams;
        }
        std::string malformed(16,'\0');WarriorNativePack::put(malformed,0,0x6000ffff);auto prior=malformed;
        assert(!WarriorNativeFitting::face_stream(malformed,head.pack.entries[2],body.pack.entries[2],error) && malformed==prior);
    }
    assert(streams>=20 && vertices>1000);std::cout<<"PASS "<<streams<<" expression streams / "<<vertices<<" vertices: consistent fitting, unchanged attributes, malformed input rejected\n";
}
