#include "../src/gs/warrior_native_sources.h"
#include "../src/gs/warrior_native_parts.h"
#include <cassert>
#include <iostream>
int main(int argc,char **argv) {
    assert(argc==2);auto root=std::filesystem::u8path(argv[1]);
    std::vector<WarriorNativeSources::CatalogueEntry> entries;std::string error;
    assert(WarriorNativeSources::catalogue(root,entries,error));
    unsigned later=0;
    for(const auto &entry:entries) {
        WarriorNativeSources::Model source;
        if(!WarriorNativeSources::model(root,entry.name,source,error)) {
            std::cerr<<entry.id<<": "<<error<<"\n";return 1;
        }
        assert(source.id==entry.id && WarriorNativeSources::bt4_origin(source.pack));
        std::vector<WarriorNativeParts::Record> records;
        if(!WarriorNativeParts::records(source.pack.entries[2],records,error)) {
            std::cerr<<entry.id<<": "<<error<<"\n";return 1;
        }
        later+=entry.id>=161;
        std::string raw;
        // The final identity exercises generation asset IDs beyond the former
        // 3400 ceiling, independent of any particular mixture of body parts.
        if(entry.id==249)for(int id:{1423+2490,1431+2490,1432+2490})
            assert(WarriorNativeSources::asset(root,id,raw,error));
    }
    assert(entries.size()==241 && later==80);
    std::cout<<"PASS all 241 BT4 donor identities, hashes, model hierarchies and high-ID generation assets\n";
}
