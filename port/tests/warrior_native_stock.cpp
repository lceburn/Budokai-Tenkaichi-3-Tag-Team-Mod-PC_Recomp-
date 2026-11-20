#include "../src/gs/warrior_native_stock.h"
#include <cassert>
#include <iostream>
static bool script_padding_equal(const std::string &a,const std::string &b) {
    auto used=[](const std::string &s) {
        if(s.size()%2)return std::string::npos;
        for(size_t i=0;i<s.size();i+=2)if(!s[i] && !s[i+1]) {
            for(size_t j=i;j<s.size();j++)if(s[j])return std::string::npos;
            return i+2;
        }
        return std::string::npos;
    };
    auto x=used(a),y=used(b);return x!=std::string::npos && x==y && a.compare(0,x,b,0,y)==0;
}
static bool model_padding_equal(const std::string &a,const std::string &b) {
    WarriorNativePack::Pack actual,expected;std::string error;
    if(!WarriorNativePack::decode(a,actual,error) || !WarriorNativePack::decode(b,expected,error) ||
       actual.entries.size()!=expected.entries.size() || actual.entries.size()<52)return false;
    for(unsigned i=43;i<52;i++)if(actual.entries[i]!=expected.entries[i]) {
        if(!script_padding_equal(actual.entries[i],expected.entries[i]))return false;
        actual.entries[i]=expected.entries[i];
    }
    std::string encoded;return WarriorNativePack::encode(actual,encoded,error) && encoded==b;
}
int main(int argc,char **argv) {
    using namespace WarriorNativeStock;
    assert(argc==5);auto root=fs::u8path(argv[1]),out=fs::u8path(argv[2]),golden=fs::u8path(argv[3]);
    Document recipe;assert(WarriorRecipe::read(fs::u8path(argv[4]),recipe.raw));std::string error;
    assert(supports(recipe));
    WarriorNativePack::Pack bad;bad.entries.resize(26);float height;
    assert(!skeleton_height(bad,height));
    bad.entries[2]=std::string(0x70,'\0');WarriorNativePack::put(bad.entries[2],0x6c,0xffffffff);
    assert(!skeleton_height(bad,height));
    bad.entries[2]=std::string(0xb0,'\0');WarriorNativePack::put(bad.entries[2],0x6c,0x70);
    bad.entries[2][0x7a]=3;WarriorNativePack::put(bad.entries[2],0x84,0x7fc00000);
    assert(!skeleton_height(bad,height));
    WarriorNativePack::put(bad.entries[2],0x84,0xbf800000);assert(skeleton_height(bad,height) && height==-1.f);
    auto invalid=recipe;invalid.raw+="hair=Kid Goku\n";assert(!supports(invalid));
    if(!generate(root,recipe,out,error)){std::cerr<<error<<"\n";return 1;}
    // Compare the published resource set, excluding later diagnostic artifacts
    // that may have been placed next to the reference output.
    std::string published;assert(WarriorRecipe::read(golden/".warrior_built",published));
    std::vector<std::pair<std::string,std::string>> expected={{"mod.txt","mod.txt"},{"native-colours.tsv","native-colours.tsv"}};
    std::string nativePublished;assert(WarriorRecipe::read(out/".warrior_built",nativePublished));
    auto resources=[](const std::string &text) {
        std::vector<std::string> names;std::istringstream lines(text);std::string line;
        while(std::getline(lines,line)) {
            if(!line.empty() && line.back()=='\r')line.pop_back();
            if(!line.empty() && line[0]!='#')names.push_back(line);
        }
        return names;
    };
    const auto referenceNames=resources(published),nativeNames=resources(nativePublished);
    assert(referenceNames.size()==nativeNames.size());
    // The router assigns resources by manifest order, so a source fighter's
    // historical filename prefix is not part of the generated file format.
    for(size_t i=0;i<referenceNames.size();i++)expected.emplace_back(referenceNames[i],nativeNames[i]);
    unsigned files=0;
    for(const auto &entry:expected) {
        auto relative=fs::u8path(entry.first);std::string before,after;
        assert(WarriorNativeBuild::read_file(golden/relative,before,true));
        assert(WarriorNativeBuild::read_file(out/fs::u8path(entry.second),after,true));
        if(relative=="mod.txt" || relative==".warrior_built" || relative=="native-colours.tsv") {
            auto normalize=[](std::string &text){size_t at=0;while((at=text.find("\r\n",at))!=std::string::npos)text.replace(at,2,"\n");};
            normalize(before);normalize(after);
        }
        if(before!=after && (!WarriorNativeBuild::model_name(relative.generic_string()) || !model_padding_equal(after,before))) {
            std::cerr<<"DIFFERENT "<<relative.generic_string()<<"\n";return 1;
        }files++;
    }
    assert(files>=12);
    std::cout<<"PASS: "<<files<<" native stock outputs match reference resources, with equivalent text manifests and zero script padding\n";
}
