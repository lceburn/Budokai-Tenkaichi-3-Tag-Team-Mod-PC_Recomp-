#include "../src/gs/warrior_native_stock.h"
#include "warrior_mesh_compare.h"
#include <cassert>
#include <iostream>
static bool compare(const WarriorNativePack::Pack &native,const WarriorNativePack::Pack &reference,std::string &error) {
    unsigned mask=0;
    std::vector<WarriorNativeParts::Record> rows,gold;
    assert(WarriorNativeParts::records(native.entries[2],rows,error));
    assert(WarriorNativeParts::records(reference.entries[2],gold,error));
    for(unsigned node:{46u,47u,48u}) {
        const WarriorNativeParts::Record *a,*b;
        assert(WarriorNativeFitting::find(rows,node,a,error) && WarriorNativeFitting::find(gold,node,b,error));
        WarriorNativeSeams::Points p,q;
        assert(WarriorNativeSeams::read(native.entries[2],*a,p,error) && WarriorNativeSeams::read(reference.entries[2],*b,q,error));
        if(!WarriorMeshCompare::retained(native.entries[2],*a,p,reference.entries[2],*b,q)) {
            for(size_t v=0;v<std::min(p.offsets.size(),q.offsets.size());v++){float d=0;for(unsigned k=0;k<3;k++)d=std::max(d,std::fabs(WarriorNativeFitting::scalar(native.entries[2],p.offsets[v]+4*k)-WarriorNativeFitting::scalar(reference.entries[2],q.offsets[v]+4*k)));if(d>.00002f)std::cerr<<"v="<<v<<" delta="<<d<<"\n";}
            std::cerr<<"Head surface mismatch node "<<node<<" mask="<<mask
                <<" native vertices="<<p.offsets.size()<<" reference vertices="<<q.offsets.size()<<"\n";return false;
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
        if(!found){std::cerr<<"Unmatched native hair node "<<rows[i].node<<" vertices="<<points.offsets.size()<<"\n";return false;}
    }
    if(matched!=expected){std::cerr<<"Hair branch count native="<<matched<<" reference="<<expected<<"\n";return false;}
    return true;
}
int main(int argc,char **argv) {
    assert(argc==4); namespace fs=std::filesystem;
    auto root=fs::u8path(argv[1]),gold=fs::u8path(argv[2]),out=fs::u8path(argv[3]);
    WarriorRecipe::Document recipe; std::string error;
    assert(WarriorRecipe::read(gold/"warrior.txt",recipe.raw));
    setenv("BT3_WARRIOR_NATIVE_HEAD_TEST","1",1);
    setenv("BT3_WARRIOR_NATIVE_HEAD_PIECES_TEST","1",1);
    if(!WarriorNativeStock::generate(root,recipe,out,error)){std::cerr<<error<<"\n";return 1;}
    std::string manifest,name; assert(WarriorRecipe::read(gold/".warrior_built",manifest));
    std::istringstream lines(manifest); unsigned models=0;
    while(std::getline(lines,name)) {
        if(!name.empty() && name.back()=='\r')name.pop_back();
        if(name.empty() || name[0]=='#' || !WarriorNativeBuild::model_name(name))continue;
        std::string a,b; WarriorNativePack::Pack native,reference;
        assert(WarriorNativeBuild::read_file(gold/fs::u8path(name),a,true));
        assert(WarriorNativeBuild::read_file(out/fs::u8path(name),b,true));
        assert(WarriorNativePack::decode(a,reference,error) && WarriorNativePack::decode(b,native,error));
        if(!compare(native,reference,error)){std::cerr<<name<<"\n";return 1;}
        ++models;
    }
    assert(models==6); std::cout<<"PASS six costume/damage head-piece variants\n";
}
