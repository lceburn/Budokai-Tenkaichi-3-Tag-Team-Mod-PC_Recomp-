#include "../src/gs/warrior_native_stock.h"
#include <cassert>
#include <iostream>
static bool standing_equal(const std::string &a,const std::string &b,float &maximum) {
    if(a.size()!=b.size() || a.size()<148)return false;
    auto half=[&](size_t at){return unsigned(static_cast<unsigned char>(a[at]))|(unsigned(static_cast<unsigned char>(a[at+1]))<<8);};
    std::set<size_t> translations;
    for(unsigned node=0;node<71;node++) {
        size_t off=size_t(half(6+node*2))*4;if(!off)continue;
        if(off>a.size() || a.size()-off<4)return false;if(half(off)&1)continue;
        unsigned n=half(off+2);if(n>(a.size()-off-4)/24)return false;
        for(unsigned i=0;i<n;i++)for(unsigned k=0;k<3;k++)translations.insert(off+4+i*24+k*4);
    }
    std::set<size_t> bytes;
    for(auto at:translations) {
        auto x=WarriorNativePack::word(a,at),y=WarriorNativePack::word(b,at);float p,q;std::memcpy(&p,&x,4);std::memcpy(&q,&y,4);
        if(!std::isfinite(p)||!std::isfinite(q))return false;maximum=std::max(maximum,std::fabs(p-q));
        for(unsigned k=0;k<4;k++)bytes.insert(at+k);
    }
    for(size_t i=0;i<a.size();i++)if(a[i]!=b[i] && !bytes.count(i))return false;
    return maximum<.00001f;
}
int main(int argc,char **argv) {
    {
        std::string absent,error;
        assert(WarriorNativeMotionGroups::scale_standing(absent,1.5f,error) && absent.empty());
        assert(!WarriorNativeMotionGroups::scale_standing(absent,0,error));
        std::string incomplete(64,'\0');auto saved=incomplete;
        assert(!WarriorNativeMotionGroups::scale_standing(incomplete,1,error) && incomplete==saved);
    }
    {
        WarriorNativePack::Pack bt4;bt4.entries.resize(252);bt4.flags.resize(252);
        bt4.entries[25]="original BT4 fighter data";std::string error;
        assert(WarriorNativeMotionGroups::scales(bt4,-5,-10,-5,{},error));
        assert(bt4.entries.size()==253 && bt4.flags.size()==253 &&
               bt4.entries[25]=="original BT4 fighter data" &&
               bt4.entries[252].compare(0,8,"BT3WARRI")==0);
        assert(WarriorNativeMotionGroups::scales(bt4,-5,-10,-4,{},error));
        float scale;auto bits=WarriorNativePack::word(bt4.entries[252],8);std::memcpy(&scale,&bits,4);
        assert(scale==.4f); // Alternate costumes use their own height, not costume one's.
        assert(WarriorNativeMotionGroups::scales(bt4,-5,-5,-4,{},error,true));
        bits=WarriorNativePack::word(bt4.entries[252],8);std::memcpy(&scale,&bits,4);
        assert(scale==1.f); // Matching body/moves already use that costume's animations.
        assert(WarriorNativeMotionGroups::scales(bt4,-5,-10,-5,{},error) && bt4.entries.size()==253);
        bt4.entries[252].replace(0,8,"BT4WARRI");
        assert(WarriorNativeMotionGroups::scales(bt4,-5,-10,-5,{},error) &&
               bt4.entries[252].compare(0,8,"BT4WARRI")==0 && bt4.entries[25]=="original BT4 fighter data");
        bt4.entries[252]="unknown extension";auto before=bt4;
        assert(!WarriorNativeMotionGroups::scales(bt4,-5,-10,-5,{},error) && bt4.entries==before.entries);
    }
    assert(argc==5);namespace fs=std::filesystem;auto root=fs::u8path(argv[1]),base=fs::u8path(argv[2]),gold=fs::u8path(argv[3]),out=fs::u8path(argv[4]);
    WarriorRecipe::Document recipe;std::string raw,wanted,error;assert(WarriorRecipe::read(gold/"warrior.txt",recipe.raw));assert(WarriorNativeStock::supports(recipe));
    std::vector<WarriorNativeMotionGroups::Selection> selections;
    assert(WarriorNativeMotionGroups::prepare(root,recipe,selections,error) && selections.size()==1);
    WarriorNativePack::Pack motions;assert(WarriorNativeBuild::read_file(base/"Goku_3_anm.pak",raw,true) && WarriorNativePack::decode(raw,motions,error));
    assert(WarriorNativeMotionGroups::apply(motions,selections,error) && WarriorNativePack::encode(motions,raw,error));assert(WarriorNativeBuild::read_file(gold/"Goku_3_anm.pak",wanted,true) && raw==wanted);
    auto previous=motions;auto invalid=selections;invalid[0].motions.entries.pop_back();assert(!WarriorNativeMotionGroups::apply(motions,invalid,error) && motions.entries==previous.entries);
    std::string broken(148,'\0');broken[6]=char(255);broken[7]=char(255);auto saved=broken;assert(!WarriorNativeMotionGroups::scale_standing(broken,1.5f,error) && broken==saved);
    if(!WarriorNativeStock::generate(root,recipe,out,error)){std::cerr<<error<<"\n";return 1;}
    std::string manifest,name;assert(WarriorRecipe::read(gold/".warrior_built",manifest));std::istringstream lines(manifest);unsigned models=0,files=0;
    while(std::getline(lines,name)) {
        if(!name.empty() && name.back()=='\r')name.pop_back();if(name.empty() || name[0]=='#')continue;
        assert(WarriorNativeBuild::read_file(gold/fs::u8path(name),wanted,true) && WarriorNativeBuild::read_file(out/fs::u8path(name),raw,true));
        if(raw!=wanted) {
            WarriorNativePack::Pack actual,expected;float maximum=0;
            if(!WarriorNativeBuild::model_name(name) || !WarriorNativePack::decode(raw,actual,error) || !WarriorNativePack::decode(wanted,expected,error) || actual.entries.size()<36 || expected.entries.size()<36 || !standing_equal(actual.entries[35],expected.entries[35],maximum)) {std::cerr<<"Reference difference: "<<name<<"\n";return 2;}
            actual.entries[35]=expected.entries[35];std::string checked;assert(WarriorNativePack::encode(actual,checked,error) && checked==wanted);
            std::cout<<name<<" standing translation max rounding="<<maximum<<"\n";
        }files++;models+=WarriorNativeBuild::model_name(name);
    }
    assert(models==6 && files==11);std::cout<<"PASS native motion category="<<selections[0].group<<" all six models/scales and five supporting resources match reference; bad layouts preserve output\n";
}
