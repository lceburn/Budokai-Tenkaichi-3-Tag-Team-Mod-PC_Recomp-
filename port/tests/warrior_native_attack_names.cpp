#include "../src/gs/warrior_native_attack_names.h"
#include "../src/gs/warrior_native_sources.h"
#include <cassert>
#include <iostream>
int main(int argc,char **argv) {
    assert(argc==2);auto root=std::filesystem::u8path(argv[1]);std::string index,error;assert(WarriorNativeBuild::read_file(root/"warrior-sources/index.tsv",index));
    std::istringstream rows(index);std::string line;unsigned count=0;
    while(std::getline(rows,line)) {
        if(line.empty() || line[0]=='#')continue;auto first=line.find('\t'),second=line.find('\t',first+1);assert(first!=std::string::npos && second!=std::string::npos);
        auto name=line.substr(first+1,second-first-1);WarriorNativeSources::Model model;assert(WarriorNativeSources::model(root,name,model,error));
        std::array<std::string,5> names;if(!WarriorNativeAttackNames::extract(model.pack,names,error)){std::cerr<<name<<": "<<error<<"\n";return 1;}
        if(name=="Goku (End)"){assert(names[0]=="Instantaneous Transmission" && names[1]=="Wild Sense" && names[2]=="Kamehameha");}
        for(const auto &s:names)assert(!s.empty() && s.size()<=256);
        auto before=names;model.pack.entries[44]="odd";assert(!WarriorNativeAttackNames::extract(model.pack,names,error) && names==before);count++;
    }
    assert(count==161);std::cout<<"PASS: actual five technique names for all "<<count<<" donors; damaged text preserves prior labels\n";
}
