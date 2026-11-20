#include "../src/gs/warrior_native_pack.h"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace WarriorNativePack;
static std::string read(const char *path){std::ifstream f(path,std::ios::binary);assert(f);return std::string(std::istreambuf_iterator<char>(f),{});}
int main(int argc,char **argv) {
    std::string error,encoded,again;Pack p,q;
    p.entries={"first",{},"last"};p.flags={1,0,2};p.endFlags=3;
    assert(encode(p,encoded,error));assert(decode(encoded,q,error));
    assert(q.entries[0].substr(0,5)=="first" && q.entries[1].empty() && q.entries[2].substr(0,4)=="last");
    assert(q.flags==p.flags && q.endFlags==3);assert(encode(q,again,error));assert(again==encoded);
    auto bad=encoded;put(bad,8,4);assert(!decode(bad,q,error));
    assert(!decode(encoded.substr(0,11),q,error));
    Pack source;source.entries={"replacement"};
    assert(!copy_entries(p,source,0,1,error));assert(!copy_entries(p,source,1,2,error));
    assert(copy_entries(p,source,1,1,error));assert(p.entries[0]=="replacement" && p.entries[2]=="last");
    if(argc==6) {
        auto raw=read(argv[2]);Pack base,donor;
        assert(decode(raw,base,error));assert(decode(read(argv[3]),donor,error));
        if(std::string(argv[1])=="style")assert(copy_style(base,donor,error));
        else {assert(std::string(argv[1])=="voice");assert(copy_entries(base,donor,53,252,error));}
        assert(encode(base,encoded,error));
        std::ofstream out(argv[5],std::ios::binary);out.write(encoded.data(),encoded.size());assert(out);out.close();
        auto expected=read(argv[4]);
        if(encoded!=expected){std::cerr<<"DIFFERENT native="<<encoded.size()<<" reference="<<expected.size()<<"\n";return 1;}
        std::cout<<"PASS: native "<<argv[1]<<" model pack matches the reference byte for byte\n";
    }else assert(argc==1);
    std::cout<<"PASS: native pack bounds, flags, empty entries, deterministic writer and donor ranges\n";
}
