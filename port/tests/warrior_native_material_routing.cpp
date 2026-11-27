#include "../src/gs/warrior_native_head.h"
#include "../src/gs/warrior_native_build.h"
#include <cassert>
#include <iostream>
using namespace WarriorNativeTextureBindings;
static WarriorNativePack::Pack load(const char *path) {
    std::string raw,e;WarriorNativePack::Pack p;
    assert(WarriorNativeBuild::read_file(path,raw,true) && WarriorNativePack::decode(raw,p,e));return p;
}
static std::vector<std::pair<unsigned,unsigned>> bindings(const std::string &mesh,const WarriorNativeParts::Record &r) {
    std::vector<std::pair<unsigned,unsigned>> result;
    if(r.size<112)return result;
    for(size_t dma=r.offset+96;dma+16<=r.offset+r.size;) {
        auto tag=WarriorNativePack::word(mesh,dma);size_t bytes=(tag&65535)*16;
        assert(bytes<=r.offset+r.size-dma-16);
        for(size_t j=0,words=bytes/4+2;j+23<=words;) {
            size_t at=dma+8+j*4;if(WarriorNativePack::word(mesh,at)!=0x6c058000)break;
            size_t count=WarriorNativePack::word(mesh,at+52)&0x7fff;assert(count<=(words-j-23)/12);
            result.push_back({WarriorNativePack::word(mesh,at+20)&0x3fff,(WarriorNativePack::word(mesh,at+24)>>5)&0x3fff});
            j+=23+count*12;
        }
        dma+=bytes+16;if((tag&0xf0000000)==0x60000000)break;
    }
    return result;
}
// Resolve only the original model's actual per-frame upload range, independent
// of the builder's material resolver. Fade aliases must never win this lookup.
static Texture sampled(const WarriorNativePack::Pack &p,unsigned tbp,unsigned cbp) {
    std::vector<Texture> all;std::string e;assert(textures(p.entries[11],all,e));
    unsigned first=(unsigned char)p.entries[2][0x4e],count=(unsigned char)p.entries[2][0x4f];
    const Texture *pixels=nullptr,*palette=nullptr;
    for(unsigned i=first;i<first+count;i++) {
        const auto &t=all.at(i);unsigned extent=WarriorNativePack::word(t.header,16);
        if(tbp>=t.tbp && tbp-t.tbp<extent && (!pixels || t.tbp>pixels->tbp))pixels=&t;
        if(!palette && t.cbp==cbp)palette=&t;
    }
    assert(pixels && palette);auto result=*pixels;result.palette=palette->palette;return result;
}
int main(int argc,char **argv) {
    assert(argc==3);auto body=load(argv[1]),head=load(argv[2]);WarriorNativePack::Pack out;std::string e;
    if(!WarriorNativeHead::compose(body,head,out,e)){std::cerr<<e<<'\n';return 1;}
    std::vector<WarriorNativeParts::Record> before,after;
    assert(WarriorNativeParts::records(body.entries[2],before,e) && WarriorNativeParts::records(out.entries[2],after,e));
    unsigned checked=0;
    for(size_t i=0;i<before.size();i++)if(!WarriorNativeHeadChains::owned(before,i) && (before[i].node<46 || before[i].node>48)) {
        auto found=std::find_if(after.begin(),after.end(),[&](const auto &r){return r.node==before[i].node;});assert(found!=after.end());
        auto a=bindings(body.entries[2],before[i]),b=bindings(out.entries[2],*found);assert(a.size()==b.size());
        for(size_t k=0;k<a.size();k++) {
            auto x=sampled(body,a[k].first,a[k].second),y=sampled(out,b[k].first,b[k].second);
            assert(x.pixels==y.pixels && x.palette==y.palette && x.format==y.format);checked++;
        }
    }
    std::vector<Texture> original,combined;assert(textures(body.entries[11],original,e) && textures(out.entries[11],combined,e));
    for(unsigned i=0;i<(unsigned char)out.entries[2][0x4f];i++)assert(!combined[i].auxiliary && combined[i].psm!=0);
    for(unsigned field:{0x4du,0x52u}) {
        unsigned count=field==0x4d?1:(unsigned char)body.entries[2][0x53];
        for(unsigned i=0;i<count;i++) {
            auto &a=original.at((unsigned char)body.entries[2][field]+i),&b=combined.at((unsigned char)out.entries[2][field]+i);
            assert(a.pixels==b.pixels && a.palette==b.palette && a.auxiliary==b.auxiliary);
        }
    }
    std::cout<<"PASS "<<checked<<" retained body materials preserve pixel/palette content; fade resources remain separate\n";
}
