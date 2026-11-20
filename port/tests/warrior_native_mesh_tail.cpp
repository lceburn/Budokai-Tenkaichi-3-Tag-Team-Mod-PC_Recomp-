#include "../src/gs/warrior_native_mesh_tail.h"
#include "../src/gs/warrior_native_build.h"
#include <cassert>
#include <iostream>
static WarriorNativePack::Pack load(const char *path) {
    std::string raw,error;WarriorNativePack::Pack pack;
    assert(WarriorNativeBuild::read_file(std::filesystem::u8path(path),raw,true));
    assert(WarriorNativePack::decode(raw,pack,error));return pack;
}
int main(int argc,char **argv) {
    assert(argc==5);auto body=load(argv[1]),candidate=load(argv[2]),reference=load(argv[3]);std::string error;
    auto original=candidate.entries[2],mesh=original;
    if(!WarriorNativeMeshTail::append(mesh,body.entries[2],body.entries[11],candidate.entries[11],error)){std::cerr<<error<<"\n";return 1;}
    auto seam=WarriorNativePack::word(mesh,0x70),gold=WarriorNativePack::word(reference.entries[2],0x70);
    assert(seam && seam==gold && seam+16<=mesh.size());
    unsigned vertices=WarriorNativePack::word(mesh,seam),triangles=WarriorNativePack::word(mesh,seam+4);
    assert(vertices==48 && triangles==16);
    auto first=WarriorNativePack::word(mesh,seam+8),expected=WarriorNativePack::word(reference.entries[2],gold+8);
    for(unsigned i=0;i<vertices;i++)assert(mesh.substr(first+i*112,48)==reference.entries[2].substr(expected+i*112,48));
    auto old=body.entries[2];WarriorNativePack::put(old,0x70,unsigned(old.size()));
    auto unchanged=original;assert(!WarriorNativeMeshTail::append(unchanged,old,body.entries[11],candidate.entries[11],error) && unchanged==original);
    old=body.entries[2];auto oldSeam=WarriorNativePack::word(old,0x70),oldTriangles=WarriorNativePack::word(old,oldSeam+12);
    WarriorNativePack::put(old,oldTriangles,0xffffffffu);
    assert(!WarriorNativeMeshTail::append(unchanged,old,body.entries[11],candidate.entries[11],error) && unchanged==original);
    candidate.entries[2]=mesh;std::string packed;assert(WarriorNativePack::encode(candidate,packed,error));
    auto out=std::filesystem::u8path(argv[4]);assert(!std::filesystem::exists(out));assert(WarriorRecipe::write(out,packed));
    std::cout<<"PASS: relocated CPU mesh section retains 48 reference vertices and 16 triangles; invalid offsets/indices preserve output\n";
}
