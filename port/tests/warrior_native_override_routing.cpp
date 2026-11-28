#include "../src/gs/warrior_native_stock.h"
#include "../include/plat_game.h"
#include <cassert>
#include <iostream>
using namespace WarriorNativePack;
extern "C" int PortWarriors_SessionActive(void){return 0;}
static Pack load(const std::filesystem::path &path) {
    std::string raw,e;Pack p;assert(WarriorNativeBuild::read_file(path,raw,true) && decode(raw,p,e));return p;
}
int main(int argc,char **argv) {
    assert(argc==3);namespace fs=std::filesystem;fs::path root=argv[1],out=argv[2];assert(!fs::exists(out));
    WarriorNativeSources::Model body,moves,selected;std::string e,raw;
    assert(WarriorNativeSources::model(root,"Broly",body,e));
    assert(WarriorNativeSources::model(root,"Goku (End)",moves,e));
    assert(WarriorNativeSources::model(root,"Vegeta (GT) Super Saiyan 4",selected,e));
    WarriorRecipe::Document base{"name=Override check\nbody=Broly\nmoves=Goku (End)\n"};
    auto specific=base;specific.raw+="stance=Vegeta (GT) Super Saiyan 4\naurafrom=Vegeta (GT) Super Saiyan 4\n";
    for(auto entry:{std::make_pair("default",base),std::make_pair("specific",specific)})
        if(!WarriorNativeStock::generate(root,entry.second,out/entry.first,e)){std::cerr<<e<<'\n';return 1;}
    Pack donorMotions,moveMotions;assert(WarriorNativeSources::asset(root,1431+selected.id*10,raw,e) && decode(raw,donorMotions,e));
    assert(WarriorNativeSources::asset(root,1431+moves.id*10,raw,e) && decode(raw,moveMotions,e));
    assert(encode(moveMotions,raw,e) && decode(raw,moveMotions,e));
    assert(encode(donorMotions,raw,e) && decode(raw,donorMotions,e));
    auto defaults=load(out/"default/Goku_3_anm.pak"),overridden=load(out/"specific/Goku_3_anm.pak");
    assert(defaults.entries==moveMotions.entries);
    std::set<unsigned> stance;const auto *ranges=WarriorNativeMotionGroups::ranges(9);
    for(unsigned j=0;j<8 && ranges[j+1];j+=2)for(unsigned i=ranges[j];i<ranges[j]+ranges[j+1];i++)stance.insert(i);
    for(unsigned i=0;i<defaults.entries.size();i++)assert(overridden.entries[i]==(stance.count(i)?donorMotions.entries[i]:defaults.entries[i]));
    assert(load(out/"default/Goku_3_eff.pak").entries==load(out/"specific/Goku_3_eff.pak").entries);
    auto model=load(out/"specific/Goku_3_1p.pak");std::string bytes;assert(encode(model,bytes,e));
    assert(PortGame_WarriorAuraOverride(bytes.data(),int(bytes.size()))==(unsigned char)selected.pack.entries[17][3]);
    assert(WarriorNativeAura::apply(model,moves.pack,e,false) && encode(model,bytes,e));
    assert(PortGame_WarriorAuraOverride(bytes.data(),int(bytes.size()))==-1);
    assert(model.entries[17][3]==moves.pack.entries[17][3]);
    float bodyHeight,sourceHeight;assert(WarriorNativeMotionGroups::height(body.pack,bodyHeight,e) && WarriorNativeMotionGroups::height(selected.pack,sourceHeight,e));
    auto expected=selected.pack.entries[35];assert(WarriorNativeMotionGroups::scale_standing(expected,bodyHeight/sourceHeight,e));
    assert(load(out/"specific/Goku_3_1p.pak").entries[35]==expected);
    // Every exposed animation donor must replace its advertised ranges.
    for(unsigned group=0;group<10;group++)if(group!=2) {
        auto choice=base;choice.raw+=std::string(WarriorNativeMotionGroups::key(group))+"=Vegeta (GT) Super Saiyan 4\n";
        std::vector<WarriorNativeMotionGroups::Selection> selections;
        assert(WarriorNativeMotionGroups::prepare(root,choice,selections,e));
        auto motions=moveMotions;assert(WarriorNativeMotionGroups::apply(motions,selections,e));
        std::set<unsigned> affected;const auto *r=WarriorNativeMotionGroups::ranges(group);
        for(unsigned j=0;j<8 && r[j+1];j+=2)for(unsigned i=r[j];i<r[j]+r[j+1];i++)affected.insert(i);
        for(unsigned i=0;i<motions.entries.size();i++) {
            const auto &wanted=affected.count(i)?selections[0].motions.entries[i]:moveMotions.entries[i];
            assert(motions.entries[i]==wanted);
        }
    }
    auto camera=base;camera.raw+="cam_blast2_1=Vegeta (GT) Super Saiyan 4:1\ncam_blast2_2=Vegeta (GT) Super Saiyan 4:2\ncam_ultimate_loop=Vegeta (GT) Super Saiyan 4:3\n";
    auto visuals=camera;visuals.raw+="effects=Broly\n";
    Pack originalFx,rebuiltFx,donorFx,common,cameras,donorCommon,donorCameras;
    assert(WarriorNativeEffects::assemble(root,camera,originalFx,e) && WarriorNativeEffects::assemble(root,visuals,rebuiltFx,e));
    for(unsigned i=0;i<5;i++)assert(originalFx.entries[i]==rebuiltFx.entries[i]);
    auto mixed=visuals;mixed.raw+="blast2_1=Vegeta (GT) Super Saiyan 4:2\nultimatefx=Vegeta (GT) Super Saiyan 4:1\n";
    Pack mixedFx;assert(WarriorNativeEffects::assemble(root,mixed,mixedFx,e));
    Pack selectedFx;assert(WarriorNativeEffects::load(root,"Vegeta (GT) Super Saiyan 4",selectedFx,e));
    assert(mixedFx.entries[2]==selectedFx.entries[3] && mixedFx.entries[4]==selectedFx.entries[2]);
    for(unsigned i:{0u,1u,3u})assert(mixedFx.entries[i]==originalFx.entries[i]);
    auto resetFx=mixedFx;assert(WarriorNativeEffects::update(root,resetFx,mixed,visuals,e));
    assert(resetFx.entries==rebuiltFx.entries && resetFx.flags==rebuiltFx.flags);
    assert(WarriorNativeEffects::load(root,"Vegeta (GT) Super Saiyan 4",donorFx,e));
    assert(decode(rebuiltFx.entries[5],common,e) && decode(common.entries[7],cameras,e));
    assert(decode(donorFx.entries[5],donorCommon,e) && decode(donorCommon.entries[7],donorCameras,e));
    for(unsigned i=0;i<3;i++)assert(cameras.entries[i]==donorCameras.entries[i]);
    auto updatedFx=originalFx;assert(WarriorNativeEffects::update(root,updatedFx,camera,visuals,e));
    assert(updatedFx.entries==rebuiltFx.entries && updatedFx.flags==rebuiltFx.flags);
    auto coloured=visuals;coloured.raw+="aura=000000\n";
    assert(WarriorNativeEffects::update(root,updatedFx,visuals,coloured,e));
    Pack colouredCommon;assert(decode(updatedFx.entries[5],colouredCommon,e));
    assert(colouredCommon.entries[7]==common.entries[7]);
    puts("PASS complete Move Set defaults, explicit stance priority, aura marker/reset and unchanged attacks/cameras");
    puts("PASS all exposed animation groups, selected loop-camera routing and camera preservation across effect/colour edits");
}
