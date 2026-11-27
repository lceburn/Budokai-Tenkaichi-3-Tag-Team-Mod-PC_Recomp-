/* Exercise the production draft operation without game assets or save writes. */
#include "../src/gs/warrior_native.cpp"
#include <cassert>
/* Unrelated game-boundary calls must never execute in this draft-only test. */
extern "C" void WarriorView_Clear(void){abort();}
extern "C" void Port_FileWarriorClose(void){abort();}
extern "C" void PortWarriors_Reload(void){abort();}
extern "C" int PortWarriors_Preview(int){abort();}
extern "C" const char *Port_FileRoot(void){abort();return "";}
extern "C" void *Port_LowAlloc(size_t){abort();return nullptr;}
extern "C" int PortGame_CheckAnimation(void *,int){abort();return 0;}
int main() {
    Draft base;base.folder="Warrior - ICEBURN";base.source="original-source";
    base.edited.raw="name=ICEBURN\nbody=Vegeta\nhair=Vegito\naura=12abef\nblast2_1=Goku,1\nform1=1,0,Warrior - ICEBURN 2\n";
    base.original=base.edited;library.entries.push_back(base);
    auto original=library.entries[0].edited.raw;
    int copy=PortZwNative_Duplicate(0);assert(copy==1);
    assert(library.entries[0].edited.raw==original);
    auto &draft=library.entries[copy];
    assert(draft.fresh && draft.folder!=base.folder && draft.source==base.source);
    assert(draft.edited.get("name")=="ICEBURN Copy");
    assert(draft.edited.get("hair")=="Vegito" && draft.edited.get("aura")=="12abef");
    assert(draft.edited.get("blast2_1")=="Goku,1" && draft.edited.get("form1").empty());
    patch_field(draft.edited,"hair","Gohan");assert(library.entries[0].edited.raw==original);
    assert(PortZwNative_Duplicate(0)==2 && library.entries[2].edited.get("name")=="ICEBURN Copy 2");
    assert(PortZwNative_Duplicate(0)==3);
    assert(PortZwNative_Duplicate(0)==4);
    assert(PortZwNative_Duplicate(0)==5);
    assert(PortZwNative_Duplicate(0)==-1 && library.entries.size()==6);
    assert(library.entries[0].edited.raw==original);
    library.entries.resize(1);patch_field(library.entries[0].edited,"form1","");
    assert(PortZwNative_New(0)==1); /* Existing linked-form command still links both directions. */
    assert(!library.entries[0].edited.get("form1").empty());
    assert(library.entries[1].edited.get("form1")=="1,0,Warrior - ICEBURN");
    puts("PASS: independent duplicate, unchanged source, unique names, preserved design, capacity and linked-form regression");
}
