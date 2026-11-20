#include "../src/gs/warrior_native_head.h"
#include "../src/gs/warrior_native_build.h"
#include "warrior_mesh_compare.h"
#include <cassert>
#include <iostream>
static WarriorNativePack::Pack load(const char *path) {
    std::string data,error;WarriorNativePack::Pack pack;
    assert(WarriorNativeBuild::read_file(std::filesystem::u8path(path),data,true));
    assert(WarriorNativePack::decode(data,pack,error));return pack;
}
int main(int argc,char **argv) {
    assert(argc==4 || argc==5);auto body=load(argv[1]),donor=load(argv[2]),reference=load(argv[3]);
    WarriorNativePack::Pack native;std::string error;
    unsigned mask=argc==5?unsigned(std::atoi(argv[4])):7;
    if(!WarriorNativeHead::compose(body,donor,native,error,mask)){std::cerr<<error<<"\n";return 1;}
    std::vector<WarriorNativeParts::Record> rows,gold;
    assert(WarriorNativeParts::records(native.entries[2],rows,error));
    assert(WarriorNativeParts::records(reference.entries[2],gold,error));
    for(unsigned node:{46u,47u,48u}) {
        const WarriorNativeParts::Record *a,*b;
        assert(WarriorNativeFitting::find(rows,node,a,error) && WarriorNativeFitting::find(gold,node,b,error));
        WarriorNativeSeams::Points p,q;
        assert(WarriorNativeSeams::read(native.entries[2],*a,p,error) && WarriorNativeSeams::read(reference.entries[2],*b,q,error));
        if(!WarriorMeshCompare::retained(native.entries[2],*a,p,reference.entries[2],*b,q)) {
            std::cerr<<"Head surface mismatch node "<<node<<" mask="<<mask
                <<" native vertices="<<p.offsets.size()<<" reference vertices="<<q.offsets.size()<<"\n";return 1;
        }
    }
    std::set<size_t> used;unsigned matched=0,expected=0;
    for(size_t i=0;i<gold.size();i++)expected+=WarriorNativeHeadChains::owned(gold,i);
    for(size_t i=0;i<rows.size();i++)if(WarriorNativeHeadChains::owned(rows,i)) {
        WarriorNativeSeams::Points points;assert(WarriorNativeSeams::read(native.entries[2],rows[i],points,error));
        bool found=false;
        for(size_t j=0;j<gold.size();j++)if(!used.count(j) && WarriorNativeHeadChains::owned(gold,j)) {
            WarriorNativeSeams::Points other;assert(WarriorNativeSeams::read(reference.entries[2],gold[j],other,error));
            bool binds=true;
            for(unsigned k=0;k<12;k++)binds&=std::fabs(WarriorNativeFitting::scalar(native.entries[2],rows[i].offset+16+k*4)-
                WarriorNativeFitting::scalar(reference.entries[2],gold[j].offset+16+k*4))<.00002f;
            if(binds && WarriorMeshCompare::retained(native.entries[2],rows[i],points,reference.entries[2],gold[j],other)) {
                found=true;used.insert(j);matched++;break;
            }
        }
        if(!found){std::cerr<<"Unmatched native hair node "<<rows[i].node<<" vertices="<<points.offsets.size()<<"\n";return 1;}
    }
    if(matched!=expected){std::cerr<<"Hair branch count native="<<matched<<" reference="<<expected<<"\n";return 1;}
    std::cout<<"PASS "<<matched<<" native hair branches match reference binds and oriented surfaces\n";
}
