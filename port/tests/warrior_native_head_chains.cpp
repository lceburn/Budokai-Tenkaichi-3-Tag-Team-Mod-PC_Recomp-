#include "../src/gs/warrior_native_head_chains.h"
#include <cassert>
#include <iostream>
int main() {
    using namespace WarriorNativeHeadChains;
    std::string body(896,'\0'),donor(896,'\0'),out,error;
    body[0]=79;body[1]=87;body[0x2f0]=71;
    body.replace(0x114,4,"keep");body.replace(0x300,4,"base");
    donor[0]=79;donor[1]=81;donor.replace(0x110,4,"hair");
    donor.replace(0x114,4,"next");donor[0x190]=17;donor[0x1a0]=23;
    assert(chains(body,donor,{71,87},{{79,72},{81,73}},out,error));
    assert(static_cast<unsigned char>(out[0])==72 && out[1]==87 && out[2]==73);
    assert(out.substr(0x110,4)=="hair" && out.substr(0x114,4)=="keep" && out.substr(0x118,4)=="next");
    assert(out[0x190]==17 && out[0x1a0]==23 && out.substr(0x300,4)=="base");
    assert(out[0x2f0]==71 && body[0]==79); // input, retained clothing and tail stay intact
    auto saved=out;std::map<unsigned,unsigned> many;
    for(unsigned i=0;i<16;i++){donor[i]=char(79+i);many[79+i]=100+i;}
    assert(!chains(body,donor,{71,87},many,out,error) && out==saved);
    assert(!chains(body.substr(0,32),donor,{},many,out,error) && out==saved);
    std::cout<<"PASS native hair chain remapping, retained cloth/tail parameters, capacity and transactional rejection\n";
}
