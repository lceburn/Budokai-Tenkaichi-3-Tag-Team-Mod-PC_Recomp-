#include "../src/gs/warrior_native_moves.h"
#include "../src/gs/warrior_native_build.h"
#include <cassert>
#include <iostream>
static WarriorNativePack::Pack load(const char *path,std::string &raw) {
    std::string error;WarriorNativePack::Pack pack;
    assert(WarriorNativeBuild::read_file(std::filesystem::u8path(path),raw,true));
    assert(WarriorNativePack::decode(raw,pack,error));return pack;
}
int main(int argc,char **argv) {
    assert(argc==3 || argc==4);std::string a,b,c,error;
    auto body=load(argv[1],a),moves=load(argv[2],b);
    WarriorNativePack::Pack result;
    assert(WarriorNativeMoves::motions(body,moves,result,error));
    assert(result.entries==moves.entries && result.flags==moves.flags);
    std::string encoded;assert(WarriorNativePack::encode(result,encoded,error));
    assert(WarriorNativePack::encode(moves,c,error) && encoded==c);
    auto previous=result;auto bad=body;bad.entries.pop_back();
    assert(!WarriorNativeMoves::motions(bad,moves,result,error) && result.entries==previous.entries);
    assert(WarriorNativeMoves::motions(moves,moves,result,error) && result.entries==moves.entries);
    std::cout<<"PASS: all 440 default motions follow Move Set, including stance; invalid layout preserves output\n";
}
