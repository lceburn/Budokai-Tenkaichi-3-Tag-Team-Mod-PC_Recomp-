#include "../src/gs/warrior_native_parts.h"
#include "../src/gs/warrior_native_build.h"
#include <cassert>
#include <iostream>
#include <algorithm>
static WarriorNativePack::Pack load(const std::filesystem::path &p) {
    std::string raw,error;WarriorNativePack::Pack pack;
    assert(WarriorNativeBuild::read_file(p,raw,true));assert(WarriorNativePack::decode(raw,pack,error));return pack;
}
int main(int argc,char **argv) {
    using namespace WarriorNativeParts;namespace fs=std::filesystem;
    assert(argc==4);auto base=load(fs::u8path(argv[1])),donor=load(fs::u8path(argv[2])),reference=load(fs::u8path(argv[3]));
    auto originalBody=base.entries[2],originalHead=donor.entries[2];std::string error;HeadPlan plan;
    assert(whole_head_plan(originalBody,originalHead,plan,error));
    unsigned replaced=0,retained=0;std::vector<Record> expected;assert(records(reference.entries[2],expected,error));
    assert(plan.choices.size()==plan.base.size());
    for(const auto &choice:plan.choices) {
        auto before=plan.base[choice.base];auto selected=choice.imported?plan.donor[choice.donor]:before;
        assert(before.node==selected.node);
        if(choice.imported){assert(before.node>=46 && before.node<=48);replaced++;}else retained++;
        auto found=std::find_if(expected.begin(),expected.end(),[&](const Record &r){return r.node==before.node;});
        assert(found!=expected.end() && selected.size==found->size);
    }
    assert(replaced==3 && retained>40);
    assert(base.entries[2]==originalBody && donor.entries[2]==originalHead);
    auto saved=plan.choices.size();auto malformed=originalHead;
    for(auto &r:plan.donor)if(r.node==48)malformed[r.offset+10]=55;
    assert(!whole_head_plan(originalBody,malformed,plan,error) && plan.choices.size()==saved);
    assert(!whole_head_plan(std::string(20,'\0'),originalHead,plan,error) && plan.choices.size()==saved);
    std::cout<<"PASS: whole-head anchor selection matches reference record sizes, preserves body records and inputs; not a geometry merge\n";
}
