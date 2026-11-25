/* Exercise production draft editing: variant isolation and original defaults. */
#include "../src/gs/warrior_native.cpp"
#include <cassert>
extern "C" void WarriorView_Clear(void){abort();}
extern "C" void Port_FileWarriorClose(void){abort();}
extern "C" void PortWarriors_Reload(void){abort();}
extern "C" int PortWarriors_Preview(int){abort();}
int main() {
    constexpr unsigned normal=0x40000030,damage=0x40000050;
    Draft d;d.folder="Warrior - Colour probe";d.coloursOriginal=true;
    d.original.raw="name=Colour probe\nbody=Goku (End)\nmoves=Goku (End)\naura=123456\ncolour="+std::to_string(normal)+",180,150,120\n";
    d.edited=d.original;
    d.colours={{normal,0,{100,80,60},0,0,0},{damage,2,{90,70,50},1,0,normal}};
    library.entries={d};library.selected=0;
    PortZwNative_ColourMode(0);assert(PortZwNative_ColourCount()==1);
    assert(PortZwNative_ColourValue(0,0)==180);
    assert(PortZwNative_ColourWrite(0,60,40,20));assert(PortZwNative_ColourCopy(0));
    PortZwNative_ColourMode(1);assert(PortZwNative_ColourCount()==1);
    assert(PortZwNative_ColourValue(0,0)==60); // automatic normal -> damage
    assert(PortZwNative_ColourPaste(0));
    assert(PortZwNative_ColourReset(0));assert(PortZwNative_ColourValue(0,0)==90);
    PortZwNative_ColourMode(0);assert(PortZwNative_ColourWrite(0,70,50,30));
    PortZwNative_ColourMode(1);assert(PortZwNative_ColourValue(0,0)==90); // independent reset override
    PortZwNative_ColourMode(0);assert(PortZwNative_ColourReset(0));
    assert(PortZwNative_ColourValue(0,0)==100); // donor default, not saved 180
    assert(library.entries[0].edited.get("aura")=="123456");
    assert(library.entries[0].original.raw==d.original.raw);
    // Reopening the saved draft must retain original metadata defaults.
    library.entries[0].original=library.entries[0].edited;
    assert(PortZwNative_ColourWrite(0,1,2,3));assert(PortZwNative_ColourReset(0));
    assert(PortZwNative_ColourValue(0,0)==100);
    assert(!PortZwNative_ColourWrite(0,-1,2,3));assert(!PortZwNative_ColourReset(99));
    puts("PASS: normal/damaged isolation, automatic inheritance, cross-tab copy/paste, original-default reset and preserved recipe fields");
}
