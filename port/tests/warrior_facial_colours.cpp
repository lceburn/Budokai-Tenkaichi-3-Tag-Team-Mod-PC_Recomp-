#include "../src/gs/warrior_native_stock.h"
#include <cassert>
#include <iostream>
using namespace WarriorNativeMaterials;
using namespace WarriorNativePack;
static Pack load(const std::filesystem::path&p){Pack m;std::string raw,e;assert(WarriorNativeBuild::read_file(p,raw,true)&&decode(raw,m,e));return m;}
int main(int argc,char**argv){
 assert(argc==3 || argc==4);namespace fs=std::filesystem;fs::path root=argv[1],out=argv[2];std::string e,head=argc==4?argv[3]:"Kefla";
 WarriorRecipe::Document recipe{"name=Facial color check\nbody=Zangya\nmoves=Goku (End)\nhead="+head+"\nface="+head+"\nhair="+head+"\n"};
 if(!WarriorNativeStock::generate(root,recipe,out,e)){std::cerr<<e<<'\n';return 1;}
 unsigned models=0,frames=0,pixels=0,failed=0;
 for(auto&file:fs::directory_iterator(out))if(WarriorNativeBuild::model_name(file.path().filename().string())){
  auto original=load(file.path()),painted=original;unsigned face=(unsigned char)original.entries[2][0x4c];
  std::vector<uint32_t> keys;std::vector<Material> m;assert(material_keys(original.entries[11],keys,e)&&catalogue(original,m,e));
  auto chosen=std::find_if(m.begin(),m.end(),[&](const Material&v){return v.key==keys[face];});assert(chosen!=m.end());
  assert(recolour(painted,{{chosen->key,128,75,45}},e));
  std::vector<WarriorNativeTextureBindings::Texture> base,changed;assert(WarriorNativeTextureBindings::textures(original.entries[11],base,e)&&WarriorNativeTextureBindings::textures(painted.entries[11],changed,e));
  auto &from=base[face].palette;auto &to=changed[face].palette;assert(from!=to);
  for(unsigned i=0;i<(unsigned char)original.entries[2][0x4f];i++)if(keys[i]!=chosen->key) {
   assert(base[i].pixels==changed[i].pixels && base[i].palette==changed[i].palette);
  }
  for(unsigned entry=12;entry<17;entry++)if(!original.entries[entry].empty()){
   std::vector<WarriorNativeTextureBindings::Texture> a,b;
   assert(WarriorNativeTextureBindings::textures(original.entries[entry],a,e)&&WarriorNativeTextureBindings::textures(painted.entries[entry],b,e));
   for(unsigned i=0;i<a.size();i++){
    assert(a[i].pixels==b[i].pixels && a[i].palette.size()==b[i].palette.size());frames++;
    for(unsigned n=0;n<16;n++){
     char expected[3];for(unsigned k=0;k<3;k++)expected[k]=a[i].palette[n*4+k];
     turn(hsv(chosen->r,chosen->g,chosen->b),hsv(128,75,45),expected);
     for(unsigned k=0;k<3;k++){
      int wanted=(unsigned char)expected[k];
      if((unsigned char)b[i].palette[n*4+k]!=wanted)failed++;pixels++;
     }
     assert(a[i].palette[n*4+3]==b[i].palette[n*4+3]);
    }
   }
  }
  auto bodyOnly=original;auto body=m.front();if(body.key==chosen->key)body=m.back();
  assert(recolour(bodyOnly,{{body.key,43,87,129}},e));
  for(unsigned i=12;i<17;i++)if(bodyOnly.entries[i]!=original.entries[i]){std::cerr<<"Unrelated body edit changed facial set "<<i<<'\n';failed++;}
  models++;
 }
 std::cout<<head<<": models="<<models<<" blink/speech frames="<<frames<<" checked facial RGB channels="<<pixels<<" mismatches="<<failed<<'\n';
 assert(models && frames && pixels);return failed?2:0;
}
