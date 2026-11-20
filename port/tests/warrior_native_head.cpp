#include "../src/gs/warrior_native_head.h"
#include "../src/gs/warrior_native_build.h"
#include <cassert>
#include <iostream>
static WarriorNativePack::Pack load(const std::filesystem::path &p) {
    std::string raw,error;WarriorNativePack::Pack pack;assert(WarriorNativeBuild::read_file(p,raw,true));assert(WarriorNativePack::decode(raw,pack,error));return pack;
}
int main(int argc,char **argv) {
    assert(argc==4);auto body=load(std::filesystem::u8path(argv[1])),head=load(std::filesystem::u8path(argv[2]));
    std::string error,original;assert(WarriorNativePack::encode(body,original,error));WarriorNativePack::Pack out;
    if(!WarriorNativeHead::compose(body,head,out,error)){std::cerr<<error<<"\n";return 1;}
    std::string encoded,after;assert(WarriorNativePack::encode(out,encoded,error));assert(WarriorNativePack::encode(body,after,error) && after==original);
    assert(out.entries[2]!=body.entries[2] && out.entries[11]!=body.entries[11]);
    assert(out.entries[17]==body.entries[17] && out.entries[25]==body.entries[25]);
    auto output=std::filesystem::u8path(argv[3]);assert(!std::filesystem::exists(output));assert(WarriorRecipe::write(output,encoded));
    auto previous=out.entries[2];auto bad=head;bad.entries[2]=std::string(12,'\0');
    assert(!WarriorNativeHead::compose(body,bad,out,error) && out.entries[2]==previous);
    std::cout<<"PASS: experimental native head model emitted, inputs preserved, invalid source preserves result; visual/reference validation pending\n";
}
