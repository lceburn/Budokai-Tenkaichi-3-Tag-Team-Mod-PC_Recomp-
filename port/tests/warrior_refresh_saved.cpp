/* Refresh an unchanged selected warrior using real native donor fixtures. */
#include "../src/gs/warrior_native.cpp"
#include <cassert>
extern "C" void WarriorView_Clear(void){}
extern "C" void Port_FileWarriorClose(void){}
extern "C" void PortWarriors_Reload(void){}
extern "C" int PortWarriors_Preview(int index){assert(index==0);return 0;}
extern "C" const char *Port_FileRoot(void){abort();return "";}
extern "C" void *Port_LowAlloc(size_t){abort();return nullptr;}
extern "C" int PortGame_CheckAnimation(void *,int){abort();return 0;}
int main(int argc,char **argv) {
    assert(argc==2);
#ifdef _WIN32
    assert(!_putenv_s("BT3_DATA",argv[1]));
#else
    assert(!setenv("BT3_DATA",argv[1],1));
#endif
    assert(PortZwNative_Open()>0 && selected());
    auto folder=selected()->folder;auto before=selected()->edited.raw;
    assert(refresh_selected(*selected()));
    assert(PortZwNative_Build() && PortZwNative_Poll()==1);
    assert(selected()->folder==folder && selected()->edited.raw==before);
    assert(!refresh_selected(*selected()));
    std::string marker;assert(read(selected()->source/".warrior_built",marker));
    assert(marker.compare(0,12,"#warrior 49\n")==0);
    puts("PASS unchanged Save all refreshes older model resources once, preserving character identity and recipe");
}
