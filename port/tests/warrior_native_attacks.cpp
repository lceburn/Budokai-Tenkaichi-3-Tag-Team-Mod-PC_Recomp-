#include "../src/gs/warrior_native_attacks.h"
#include <cassert>
#include <iostream>
using namespace WarriorNativeAttacks;
static Pack load(const char *path) {
    Pack p;std::string raw,error;assert(WarriorNativeBuild::read_file(std::filesystem::u8path(path),raw,true));
    assert(WarriorNativePack::decode(raw,p,error));return p;
}
int main(int argc,char **argv) {
    assert(argc==8);auto body=load(argv[1]),donor=load(argv[2]),gold=load(argv[3]);
    auto motion=load(argv[4]),donorMotion=load(argv[5]),goldMotion=load(argv[6]);unsigned slot=unsigned(std::atoi(argv[7]));
    std::string error;auto model=body;
    assert(parameters(model,donor,slot,slot,error));
    assert(names(model,donor,slot,slot,error));
    for(unsigned i:{23u,24u,27u,40u,41u,42u})if(model.entries[i]!=gold.entries[i]) {
        std::cerr<<"Different native attack model entry "<<i<<"\n";return 1;
    }
    for(unsigned i=43;i<52;i++)assert(model.entries[i]==gold.entries[i]);
    assert(motions(motion,donorMotion,slot,slot,error));
    assert(motion.entries==goldMotion.entries && motion.flags==goldMotion.flags);
    auto before=model;auto bad=donor;bad.entries[slot<2?24:23].clear();
    assert(!parameters(model,bad,slot,slot,error) && model.entries==before.entries);
    assert(!parameters(model,donor,slot,slot<2?2:0,error) && model.entries==before.entries);
    auto previous=motion;bad=donorMotion;bad.flags.resize(1);
    assert(!motions(motion,bad,slot,slot,error) && motion.entries==previous.entries);
    // BT4 has one technique script and extra costume partner fields. Keep its
    // other language/AI records intact while transferring the selected lane.
    Pack bt4,bt4Source;bt4.entries.resize(253);bt4Source.entries.resize(253);
    bt4.entries[252]=bt4Source.entries[252]="BT4WARRI";
    bt4.entries[23]=std::string(0x280,'A');bt4Source.entries[23]=std::string(0x280,'B');
    bt4.entries[27]=bt4Source.entries[27]=std::string(0x90,'C');
    assert(parameters(bt4,bt4Source,3,2,error));
    for(unsigned i=0;i<4;i++) {
        assert(bt4.entries[23].substr(0x244+i*6+2,2)=="BB");
        assert(bt4.entries[23].substr(0x244+i*6,2)=="AA");
    }
    bt4.entries[44]=body.entries[44];bt4Source.entries[44]=donor.entries[44];
    bt4.entries[49]="preserved BT4 AI";bt4Source.entries[49]="different source AI";
    assert(names(bt4,bt4Source,2,2,error) && bt4.entries[49]=="preserved BT4 AI");
    std::cout<<"PASS native attack behavior/cameras and complete motion pack match reference; invalid lanes preserve output\n";
}
