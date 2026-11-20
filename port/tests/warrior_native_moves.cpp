#include "../src/gs/warrior_native_moves.h"
#include "../src/gs/warrior_native_build.h"
#include <cassert>
#include <iostream>
static WarriorNativePack::Pack load(const char *path) {
    std::string raw,error;WarriorNativePack::Pack pack;
    assert(WarriorNativeBuild::read_file(std::filesystem::u8path(path),raw,true));
    assert(WarriorNativePack::decode(raw,pack,error));return pack;
}
int main(int argc,char **argv) {
    assert(argc==4);auto body=load(argv[1]),moves=load(argv[2]),reference=load(argv[3]);
    auto model=body;std::string error;assert(WarriorNativeMoves::common(model,moves,error));
    unsigned changed=0;
    for(size_t i=0;i<model.entries.size();i++) {
        if(i==17 || (i>=18 && i<52 && i!=22 && i!=25 && i!=35)) {
            assert(model.entries[i]==reference.entries[i]);changed++;
        }else assert(model.entries[i]==body.entries[i]);
    }
    assert(changed==32);auto saved=model;auto invalid=moves;invalid.entries[17].resize(12);
    assert(!WarriorNativeMoves::common(model,invalid,error) && model.entries==saved.entries);
    auto same=moves;assert(WarriorNativeMoves::common(same,moves,error) && same.entries==moves.entries);
    std::cout<<"PASS: 32 native common-move entries match cross-body reference; body geometry/standing/chains/scaling preserved; invalid source preserves output\n";
}
