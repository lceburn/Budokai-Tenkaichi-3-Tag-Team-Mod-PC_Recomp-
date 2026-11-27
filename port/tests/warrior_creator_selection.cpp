/* Save/reopen identity and unsaved colour errors, using synthetic recipes. */
#include "../src/gs/warrior_native.cpp"
#include <cassert>
extern "C" void WarriorView_Clear(void) { abort(); }
extern "C" void Port_FileWarriorClose(void) {}
extern "C" void PortWarriors_Reload(void) {}
extern "C" int PortWarriors_Preview(int) { abort(); return 0; }
extern "C" const char *Port_FileRoot(void) { abort(); return ""; }
extern "C" void *Port_LowAlloc(size_t) { abort(); return nullptr; }
extern "C" int PortGame_CheckAnimation(void *,int) { abort(); return 0; }
int main(int argc,char **argv) {
    assert(argc==2);fs::path root=fs::u8path(argv[1]);
    assert(!fs::exists(root));fs::create_directories(root/"mods");
#ifdef _WIN32
    assert(_putenv_s("BT3_DATA",root.u8string().c_str())==0);
#else
    assert(setenv("BT3_DATA",root.u8string().c_str(),1)==0);
#endif
    assert(write(root/"native-roster.tsv","3\tGoku (End)\t\n"));
    for(const char *folder:{"First","Second"}) {
        fs::create_directory(root/"mods"/folder);
        assert(write(root/"mods"/folder/"warrior.txt",std::string("name=")+folder+"\nbody=Goku (End)\nmoves=Goku (End)\n"));
        assert(write(root/"mods"/folder/"mod.txt",std::string("white=")+folder+"\n"));
    }
    assert(write(root/"mods/enabled.txt","First\nSecond\n"));
    assert(PortZwNative_Open()==2 && PortZwNative_Select(1));
    assert(patch_field(selected()->edited,"name","Second renamed"));
    assert(save_native_metadata()==1);
    assert(PortZwNative_Selected()==1 && selected()->folder=="Second");
    assert(selected()->edited.get("name")=="Second renamed");
    assert(PortZwNative_Poll()==1);
    // Identity survives reordered registry entries, rather than retaining an index.
    assert(write(root/"mods/enabled.txt","Second\nFirst\n"));
    assert(PortZwNative_Open()==2 && PortZwNative_Selected()==0 && selected()->folder=="Second");
    assert(PortZwNative_New(-1)==2);
    const auto createdFolder=selected()->folder;
    fs::create_directory(root/"mods"/createdFolder);
    assert(write(root/"mods"/createdFolder/"warrior.txt",selected()->edited.raw));
    assert(write(root/"mods/enabled.txt","Second\nFirst\n"+createdFolder+"\n"));
    assert(PortZwNative_Open()==3 && PortZwNative_Selected()==2 && selected()->folder==createdFolder);
    selected()->fresh=true;library.status="A donor material has no source texture.";
    assert(!PortZwNative_ColourRefresh());
    assert(library.status=="Save your character first to edit its surface colours.");
    selected()->fresh=false;
    assert(patch_field(selected()->edited,"head","Vegeta"));library.status="old error";
    assert(!PortZwNative_ColourRefresh() && library.status=="Save your current parts first to refresh the surface colour choices.");
    selected()->edited=selected()->original;
    selected()->edited.raw="name=Refresh\nbody=Goku\nmoves=Goku\n";selected()->original=selected()->edited;
    assert(write(selected()->source/".warrior_built","#warrior 47\n"));
    fs::create_directory(root/"warrior-sources");assert(write(root/"warrior-sources/assets.tsv","fixture"));
    assert(refresh_selected(*selected()));
    assert(write(selected()->source/".warrior_built","#warrior 48\n"));assert(refresh_selected(*selected()));
    assert(write(selected()->source/".warrior_built","#warrior 49\n"));assert(refresh_selected(*selected()));
    assert(write(selected()->source/".warrior_built","#warrior 50\n"));assert(!refresh_selected(*selected()));
    assert(write(root/"mods/enabled.txt","First\n"));
    assert(PortZwNative_Open()==1 && PortZwNative_Selected()==0 && selected()->folder=="First");
    puts("PASS save/reopen keeps selected identity and edits; new character, reorder/removal and unsaved colour message");
}
