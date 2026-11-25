#include "../src/gs/warrior_stock_taunts.h"
#include <cassert>
#include <iostream>
int main(int argc,char **argv) {
    std::string out="unchanged",error;
    assert(!WarriorStockTaunts::decode("",out) && out=="unchanged");
    std::string invalid(16,'\0');WarriorNativePack::put(invalid,0,65537);
    assert(!WarriorStockTaunts::decode(invalid,out));
    if(argc>1) {
        std::vector<WarriorNativeSources::CatalogueEntry> donors;
        auto root=std::filesystem::u8path(argv[1]);
        assert(WarriorNativeSources::catalogue(root,donors,error));
        unsigned count=0,largest=0;
        for(const auto &donor:donors) {
            if(!WarriorStockTaunts::load(root,donor.id,out,error)) {
                std::cerr<<donor.name<<": "<<error<<'\n';return 1;
            }
            largest=std::max(largest,(unsigned)out.size());count++;
            // A truncated payload never changes the caller's previous result.
            std::string raw;WarriorNativePack::Pack motions;
            assert(WarriorNativeSources::asset(root,1431+donor.id*10,raw,error));
            assert(WarriorNativePack::decode(raw,motions,error));
            auto compressed=motions.entries[0x185];
            auto packed=WarriorNativePack::word(compressed,4);
            compressed.resize(8+packed-1);std::string retained="keep";
            assert(!WarriorStockTaunts::decode(compressed,retained) && retained=="keep");
        }
        std::cout<<"PASS: "<<count<<" stock idle taunts, largest "<<largest<<" bytes, truncated payloads rejected\n";
    }
}
