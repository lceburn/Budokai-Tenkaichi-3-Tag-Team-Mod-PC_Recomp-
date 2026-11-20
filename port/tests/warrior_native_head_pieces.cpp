#include "../src/gs/warrior_native_head_pieces.h"
#include "../src/gs/warrior_native_build.h"
#include <cassert>
#include <iostream>
int main(int argc,char **argv) {
    assert(argc==2);std::string bytes,error;WarriorNativePack::Pack pack;
    assert(WarriorNativeBuild::read_file(std::filesystem::u8path(argv[1]),bytes,true));
    assert(WarriorNativePack::decode(bytes,pack,error));
    std::vector<WarriorNativeParts::Record> rows;assert(WarriorNativeParts::records(pack.entries[2],rows,error));
    for(const auto &row:rows)if(row.node==46 || row.node==47) {
        std::vector<WarriorNativeHeadPieces::Batch> batches;
        assert(WarriorNativeHeadPieces::batches(pack.entries[2],row,batches,error) && !batches.empty());
        auto original=batches;auto malformed=row;malformed.size=65;
        assert(!WarriorNativeHeadPieces::batches(pack.entries[2],malformed,batches,error));
        assert(batches.size()==original.size() && batches.front().offset==original.front().offset);
        auto broken=pack.entries[2];WarriorNativePack::put(broken,original.front().offset+52,4097);
        assert(!WarriorNativeHeadPieces::batches(broken,row,batches,error));
        assert(batches.size()==original.size() && batches.front().offset==original.front().offset);
        broken=pack.entries[2];WarriorNativePack::put(broken,row.offset+96,0x6000ffff);
        assert(!WarriorNativeHeadPieces::batches(broken,row,batches,error));
        assert(batches.size()==original.size() && batches.front().offset==original.front().offset);
    }
    std::cout<<"PASS malformed head batches reject transactionally\n";
}
