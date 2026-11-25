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
            auto original=out;float height=0;
            assert(WarriorStockTaunts::relative(root,donor.id,out,height,error));
            assert(std::isfinite(height) && height>0 && out.size()==original.size());
            std::string modelRaw;WarriorNativePack::Pack model;
            assert(WarriorNativeSources::asset(root,1423+donor.id*10,modelRaw,error));
            assert(WarriorNativePack::decode(modelRaw,model,error));
            std::vector<WarriorNativeParts::Record> rows;
            assert(WarriorNativeParts::records(model.entries[2],rows,error));
            // Reconstruct every authored key for an unchanged donor rig. This
            // catches the old double hip offset while preserving all rotations.
            for(const auto &row:rows)if(row.node<71) {
                size_t at=4*WarriorNativeParts::half(original,6+row.node*2);
                if(!at || (WarriorNativeParts::half(original,at)&1))continue;
                for(unsigned k=0;k<WarriorNativeParts::half(original,at+2);k++) {
                    size_t key=at+4+k*24;
                    assert(original.compare(key+12,12,out,key+12,12)==0);
                    for(unsigned axis=0;axis<3;axis++) {
                        float before,relative,rest;
                        std::memcpy(&before,original.data()+key+axis*4,4);
                        std::memcpy(&relative,out.data()+key+axis*4,4);
                        std::memcpy(&rest,model.entries[2].data()+row.offset+0x30+axis*4,4);
                        assert(std::fabs(before-(relative+rest))<0.0001f);
                    }
                }
            }

            // A truncated payload never changes the caller's previous result.
            std::string raw;WarriorNativePack::Pack motions;
            assert(WarriorNativeSources::asset(root,1431+donor.id*10,raw,error));
            assert(WarriorNativePack::decode(raw,motions,error));
            auto compressed=motions.entries[0x185];
            auto packed=WarriorNativePack::word(compressed,4);
            compressed.resize(8+packed-1);std::string retained="keep";
            assert(!WarriorStockTaunts::decode(compressed,retained) && retained=="keep");
        }
        std::cout<<"PASS: "<<count<<" stock idle taunts with donor-rest translation round trips, largest "<<largest<<" bytes, truncated payloads rejected\n";
    }
}
