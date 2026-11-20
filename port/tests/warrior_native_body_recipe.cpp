#include "../src/gs/warrior_native_stock.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include "warrior_mesh_compare.h"
int main(int argc,char **argv) {
    assert(argc==5);namespace fs=std::filesystem;
    auto root=fs::u8path(argv[1]),gold=fs::u8path(argv[2]),out=fs::u8path(argv[4]);unsigned slot=std::atoi(argv[3]);
    WarriorRecipe::Document recipe;std::string error;
    assert(WarriorRecipe::read(gold/"warrior.txt",recipe.raw));assert(!WarriorNativeStock::supports(recipe));
    setenv("BT3_WARRIOR_NATIVE_BODY_TEST","1",1);setenv("BT3_WARRIOR_NATIVE_HAND_TEST","1",1);assert(WarriorNativeStock::supports(recipe));
    if(!WarriorNativeStock::generate(root,recipe,out,error)){std::cerr<<error<<"\n";return 1;}
    std::string manifest;assert(WarriorRecipe::read(gold/".warrior_built",manifest));std::istringstream lines(manifest);std::string name;
    unsigned models=0,resources=0;float maximum=0,normals=0;
    while(std::getline(lines,name)) {
        if(!name.empty() && name.back()=='\r')name.pop_back();if(name.empty() || name[0]=='#')continue;
        std::string a,b;assert(WarriorNativeBuild::read_file(gold/fs::u8path(name),a,true) && WarriorNativeBuild::read_file(out/fs::u8path(name),b,true));
        if(!WarriorNativeBuild::model_name(name)){assert(a==b);resources++;continue;}
        WarriorNativePack::Pack reference,native;
        assert(WarriorNativePack::decode(a,reference,error) && WarriorNativePack::decode(b,native,error));
        std::vector<WarriorNativeParts::Record> rows,expected;
        assert(WarriorNativeParts::records(native.entries[2],rows,error) && WarriorNativeParts::records(reference.entries[2],expected,error));
        // Compare both directions: removing an attachment must not make a
        // missing reference part disappear from this regression's coverage.
        std::multiset<unsigned> actualNodes,expectedNodes;
        for(const auto &r:rows)actualNodes.insert(r.node);
        for(const auto &r:expected)expectedNodes.insert(r.node);
        assert(actualNodes==expectedNodes);
        unsigned vertices=0;
        for(const auto &r:rows) {
            const WarriorNativeParts::Record *target;assert(WarriorNativeFitting::find(expected,r.node,target,error));
            // The reference rewrites -0 as +0 in some retained root anchors.
            // Require identical finite values, rather than identical zero signs.
            for(unsigned k=0;k<12;k++) {
                float a=WarriorNativeFitting::scalar(native.entries[2],r.offset+16+k*4);
                float b=WarriorNativeFitting::scalar(reference.entries[2],target->offset+16+k*4);
                if(!std::isfinite(a) || !std::isfinite(b) || a!=b) {
                    std::cerr<<name<<" different bind node="<<r.node<<"\n";return 1;
                }
            }
            WarriorNativeSeams::Points p,q;
            assert(WarriorNativeSeams::read(native.entries[2],r,p,error) && WarriorNativeSeams::read(reference.entries[2],*target,q,error));
            if(p.offsets.size()!=q.offsets.size()) {
                std::cerr<<name<<" node="<<r.node<<" native="<<p.offsets.size()<<" reference="<<q.offsets.size()<<"\n";return 1;
            }
            vertices+=p.offsets.size();
            bool selected=slot?WarriorNativeParts::slot(r.node)==int(slot):WarriorNativeParts::slot(r.node)>=3 && WarriorNativeParts::slot(r.node)<=10;
            if(!selected) {
                if(!WarriorMeshCompare::retained(native.entries[2],r,p,reference.entries[2],*target,q)) {
                    std::cerr<<name<<" different retained triangles node="<<r.node<<"\n";return 1;
                }
                continue;
            }
            for(size_t v=0;v<p.offsets.size();v++)for(unsigned k=0;k<3;k++) {
                maximum=std::max(maximum,std::fabs(WarriorNativeFitting::scalar(native.entries[2],p.offsets[v]+k*4)-WarriorNativeFitting::scalar(reference.entries[2],q.offsets[v]+k*4)));
                normals=std::max(normals,std::fabs(WarriorNativeFitting::scalar(native.entries[2],p.offsets[v]+16+k*4)-WarriorNativeFitting::scalar(reference.entries[2],q.offsets[v]+16+k*4)));
            }
        }
        assert(vertices>100 && ((b.size()+2047)&~size_t(2047))<=0xE6800);
        auto seam=WarriorNativePack::word(native.entries[2],0x70),goldSeam=WarriorNativePack::word(reference.entries[2],0x70);
        assert(bool(seam)==bool(goldSeam));
        if(seam) {
            unsigned count=WarriorNativePack::word(native.entries[2],seam);assert(count==WarriorNativePack::word(reference.entries[2],goldSeam));
            auto start=WarriorNativePack::word(native.entries[2],seam+8),goldStart=WarriorNativePack::word(reference.entries[2],goldSeam+8);
            for(unsigned i=0;i<count;i++)assert(native.entries[2].substr(start+i*112,48)==reference.entries[2].substr(goldStart+i*112,48));
        }
        for(size_t i=17;i<reference.entries.size();i++)if(i!=22)assert(native.entries[i]==reference.entries[i]);models++;
    }
    std::cout<<"slot="<<slot<<" max_error="<<maximum<<" normal_error="<<normals<<std::endl;
    assert(models==6 && resources==5 && maximum<.00002f && normals<.00001f);
    std::cout<<"PASS: six native body-part recipe variants match reference geometry/binds/normals/CPU skin data and supporting resources\n";
}
