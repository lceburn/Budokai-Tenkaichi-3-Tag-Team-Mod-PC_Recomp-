#include "../src/gs/warrior_native_materials.h"
#include "../src/gs/warrior_native_build.h"
#include <cassert>
#include <iostream>
using namespace WarriorNativeMaterials;
using namespace WarriorNativePack;
static Pack load(const std::filesystem::path &p){std::string raw,e;Pack m;assert(WarriorNativeBuild::read_file(p,raw,true)&&decode(raw,m,e));return m;}
static std::string shade(const Pack &m,unsigned address){std::vector<WarriorNativeTextureBindings::Texture> t;std::string e;assert(WarriorNativeTextureBindings::textures(m.entries[11],t,e));for(auto&v:t)if(v.psm==0&&v.tbp==address)return v.pixelUpload;assert(false);return {};}
static void verify_bindings(const Pack &original,const Pack &painted,const std::vector<Material>&materials,const std::vector<Colour>&changes){
 std::vector<WarriorNativeTextureBindings::Texture> t;std::string e;assert(WarriorNativeTextureBindings::textures(original.entries[11],t,e));
 std::map<unsigned,Pack> isolated;
 for(auto&m:materials)for(auto&c:changes)if(c.key==m.key){auto p=original;assert(recolour(p,{c},e));isolated.emplace(m.texture,std::move(p));}
 unsigned checked=0;
 for(unsigned entry=2;entry<=10;entry++){
  auto &a=original.entries[entry];auto &b=painted.entries[entry];assert(a.size()==b.size());auto geometry=b;
  for(size_t at=0;at+88<=a.size();at+=4)if(word(a,at)==0x6c058000){
   unsigned tbp=word(a,at+20)&0x3fff,cbp=(word(a,at+24)>>5)&0x3fff,old=(word(a,at+40)>>5)&0x3fff,now=(word(b,at+40)>>5)&0x3fff;
   for(auto&m:materials)if(t[m.texture].tbp==tbp && t[m.texture].cbp==cbp && !t[m.texture].auxiliary){
    auto found=isolated.find(m.texture);auto &reference=found==isolated.end()?original:found->second;
    unsigned wanted=(word(reference.entries[entry],at+40)>>5)&0x3fff;
    assert(shade(painted,now)==shade(reference,wanted));checked++;break;
   }
   put(geometry,at+40,word(a,at+40));
  }
  if(entry==2)for(unsigned field:{0x4cu,0x4du,0x51u,0x52u})geometry[field]=a[field];
  assert(geometry==a);
 }
 assert(checked);
}
int main(int argc,char**argv){
 assert(argc==2);namespace fs=std::filesystem;fs::path root=argv[1];unsigned count=0,aliasModels=0,editedModels=0;
 for(auto&file:fs::directory_iterator(root))if(file.path().extension()==".pak"){
  auto original=load(file.path()),noop=original;std::vector<Material> m;std::string e;assert(catalogue(original,m,e));
  std::vector<Colour> same;for(auto&v:m)same.push_back({v.key,v.r,v.g,v.b});
  if(!recolour(noop,same,e)||noop.entries!=original.entries){std::cerr<<"No-op failed for "<<file.path()<<": "<<e<<'\n';return 2;}count++;
  std::vector<uint32_t> keys;assert(material_keys(original.entries[11],keys,e));
  std::vector<WarriorNativeTextureBindings::Texture> t;
  if(!WarriorNativeTextureBindings::textures(original.entries[11],t,e)){std::cout<<"Existing malformed upload rejected for edits: "<<file.path().stem()<<'\n';continue;}
  bool alias=false;for(unsigned i=0;i<t.size();i++)for(unsigned j=0;j<i;j++)if(!t[i].auxiliary&&!t[j].auxiliary&&t[i].psm!=0&&t[j].psm!=0&&t[i].pixels==t[j].pixels&&t[i].palette!=t[j].palette){assert(keys[i]!=keys[j]);alias=true;}
  aliasModels+=alias;
  for(auto &surface:m) {
   Material readable=surface;size_t table=word(original.entries[11],4)*4;
   if(!dominant(original.entries[11],table+surface.texture*64,readable))continue;
   auto edited=original;
   if(!recolour(edited,{{surface.key,128,75,45}},e)){std::cerr<<"Single edit failed for "<<file.path()<<": "<<e<<'\n';return 3;}
   editedModels++;break;
  }
 }
 for(int id:{187,188,118,191,3,38,120,121,126}){
  auto original=load(root/(std::to_string(id)+".pak"));std::vector<Material> m;std::string e;assert(catalogue(original,m,e));
  std::vector<Colour> changes;
  for(auto&v:m)if(id!=187 || (v.g>v.r+8&&v.g>v.b+8))changes.push_back({v.key,128,75,45});
  auto a=original,b=original;assert(recolour(a,changes,e));std::reverse(changes.begin(),changes.end());assert(recolour(b,changes,e)&&a.entries==b.entries);
  verify_bindings(original,a,m,changes);
  auto invalid=original;auto saved=invalid.entries;assert(!recolour(invalid,{{m[0].key,-1,0,0}},e)&&invalid.entries==saved);
  std::cout<<"PASS donor "<<id<<": each mesh/facial shadow matches an isolated edit, unchanged surfaces and geometry preserved; request order independent\n"<<std::flush;
 }
 std::cout<<"PASS "<<count<<" donor identity edits are byte-exact no-ops; "<<aliasModels<<" palette-alias donor identities separated; "<<editedModels<<" single-surface donor edits\n";
}
