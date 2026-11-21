#include "../../include/warrior_net_pack.h"
#include "../../include/plat_warriors.h"
#include "../../include/plat_game.h"
#include "../../include/warrior_limits.h"
#include "warrior_net_resources.h"
#include <array>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <chrono>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <unistd.h>
#endif
extern "C" const char *Port_FileRoot(void);
namespace {
namespace fs=std::filesystem;
bool linked(const fs::path &path) {
#ifdef _WIN32
    DWORD flags=GetFileAttributesW(path.c_str());
    return flags!=INVALID_FILE_ATTRIBUTES && (flags&FILE_ATTRIBUTE_REPARSE_POINT)!=0;
#else
    return fs::is_symlink(fs::symlink_status(path));
#endif
}
constexpr const char *magic="TTWPK001";
std::set<std::string> ownedRooms;
bool prefix_ok(const std::string &relative) {
    if(relative.rfind(".warrior-sessions/",0)!=0 || relative.size()<=18 || relative.size()>96)return false;
    for(size_t i=18;i<relative.size();i++)if(!((relative[i]>='a' && relative[i]<='z') || (relative[i]>='0' && relative[i]<='9') || relative[i]=='-'))return false;
    return true;
}
const char *files[PORT_WARRIOR_FILE_ROLES]={"Room_1p.pak","Room_2p.pak","Room_1p_dmg.pak","Room_2p_dmg.pak",
    "Room_anm.pak","Room_eff.pak","pzs3us1/00000.bin","pzs3us1/00001.bin","Room_img.cdbt","chip.bin",
    "Room_3p.pak","Room_3p_dmg.pak","Room_4p.pak","Room_4p_dmg.pak"};
struct Form {
    std::string name,style,voice,attack[5];int aura=-1,rules=0,target[4],field[4][5];
    struct Resource {unsigned offset=0,size=0;} resource[PORT_WARRIOR_FILE_ROLES];
};
bool text_ok(const std::string &s,unsigned limit,bool empty=false) {
    if((!empty && s.empty()) || s.size()>=limit)return false;
    for(unsigned char c:s)if(c<32 || c==127)return false;
    return true;
}
bool donor_ok(const std::string &name) {
    for(int i=0;i<250;i++){const char *stock=PortGame_CharacterName(i);if(stock && name==stock)return true;}
    return false;
}
void word(std::vector<unsigned char>& b,unsigned n){for(int i=3;i>=0;i--)b.push_back((unsigned char)(n>>(i*8)));}
void text(std::vector<unsigned char>& b,const char *s){unsigned n=s?(unsigned)strlen(s):0;word(b,n);if(n)b.insert(b.end(),s,s+n);}
struct Reader {
    const unsigned char *data;unsigned size,at=8;
    unsigned word(){if(size-at<4)throw 1;unsigned n=0;for(int i=0;i<4;i++)n=(n<<8)|data[at++];return n;}
    std::string text(unsigned limit,bool empty=false){unsigned n=word();if(n>=limit || n>size-at)throw 1;
        std::string s((const char*)data+at,n);at+=n;if(!text_ok(s,limit,empty))throw 1;return s;}
};
std::vector<Form> parse(const void *data,unsigned size) {
    if(!data || size<12 || size>WNP_MAX_BYTES || memcmp(data,magic,8))throw 1;
    Reader r{(const unsigned char*)data,size};unsigned count=r.word();
    if(!count || count>PORT_WARRIOR_PLAYABLE_MAX)throw 1;
    std::vector<Form> forms(count);
    for(auto &f:forms) {
        f.name=r.text(192);f.style=r.text(192);f.voice=r.text(192);
        if(!PortGame_IsBT4() || !donor_ok(f.style) || !donor_ok(f.voice))throw 1;
        for(auto &s:f.attack)s=r.text(32,true);
        f.aura=(int)r.word();f.rules=(int)r.word();
        if(f.aura< -1 || f.aura>0xffffff || f.rules<0 || f.rules>1)throw 1;
        for(int s=0;s<4;s++) {
            f.target[s]=(int)r.word();if(f.target[s]< -1 || f.target[s]>=(int)count)throw 1;
            for(int &v:f.field[s]){v=(int)r.word();if(v< -32768 || v>32767)throw 1;}
        }
        for(int role=0;role<PORT_WARRIOR_FILE_ROLES;role++) {
            unsigned n=r.word();if(n>r.size-r.at || n>64u*1024*1024)throw 1;
            for(int costume=0;costume<4;costume++)if(role==PortWarriors_ModelRole(costume,0) || role==PortWarriors_ModelRole(costume,1))
                if(n>PORT_WARRIOR_MODEL_CAPACITY)throw 1;
            f.resource[role]={r.at,n};r.at+=n;
            if(n && !WarriorNetResources::check(role,r.data+f.resource[role].offset,n))throw 1;
        }
        for(int role=PORT_WARRIOR_ANIMATION;role<PORT_WARRIOR_RESOURCES;role++)if(!f.resource[role].size)throw 1;
        bool gap=false;
        for(int c=0;c<4;c++) {
            bool a=f.resource[PortWarriors_ModelRole(c,0)].size!=0,b=f.resource[PortWarriors_ModelRole(c,1)].size!=0;
            if(a!=b || (gap && a) || (!c && !a))throw 1;if(!a)gap=true;
        }
        if(f.resource[PORT_WARRIOR_CHIP].size && !PortGame_CheckWarriorChip(r.data+f.resource[PORT_WARRIOR_CHIP].offset,(int)f.resource[PORT_WARRIOR_CHIP].size))throw 1;
    }
    if(r.at!=size)throw 1;return forms;
}
void write(const fs::path &p,const void *data,size_t size) {
    fs::create_directories(p.parent_path());std::ofstream out(p,std::ios::binary);
    out.write((const char*)data,(std::streamsize)size);out.close();if(!out)throw 1;
}
void write(const fs::path &p,const std::string &text){write(p,text.data(),text.size());}
std::string folder(int owner,int form){return "p"+std::to_string(owner)+"-f"+std::to_string(form);}
}
extern "C" void WarriorNetPack_Free(void *data){free(data);}
extern "C" int WarriorNetPack_Check(const void *data,unsigned size) {
    try{return (int)parse(data,size).size();}catch(...){return 0;}
}
extern "C" int WarriorNetPack_CheckSelection(const void *data,unsigned size,unsigned form,unsigned costume) {
    if(costume>=4)return 0;
    try {auto forms=parse(data,size);return form<forms.size() &&
        forms[form].resource[PortWarriors_ModelRole(costume,0)].size &&
        forms[form].resource[PortWarriors_ModelRole(costume,1)].size;}catch(...){return 0;}
}
extern "C" int WarriorNetPack_CreateRoom(char prefix[WNP_ROOT_SIZE]) {
    if(!prefix)return 0;prefix[0]=0;
    try {
        fs::path base=fs::path(Port_FileRoot())/".warrior-sessions";
        if(linked(base))return 0;fs::create_directories(base);
#ifdef _WIN32
        auto pid=GetCurrentProcessId();
#else
        auto pid=getpid();
#endif
        auto tick=std::chrono::steady_clock::now().time_since_epoch().count();
        for(unsigned attempt=0;attempt<32;attempt++) {
            std::string relative=".warrior-sessions/room-"+std::to_string(pid)+"-"+std::to_string(tick)+"-"+std::to_string(attempt);
            if(!prefix_ok(relative))return 0;auto room=fs::path(Port_FileRoot())/relative;
            if(!fs::create_directory(room))continue;
            write(room/".room-owned","TTWROOM1\n");ownedRooms.insert(relative);
            snprintf(prefix,WNP_ROOT_SIZE,"%s",relative.c_str());return 1;
        }
    }catch(...){}return 0;
}
extern "C" int WarriorNetPack_RemoveRoom(const char *prefix) {
    if(!prefix || PortWarriors_SessionActive())return 0;
    try {
        std::string relative=prefix;if(!prefix_ok(relative) || !ownedRooms.count(relative))return 0;
        auto base=fs::path(Port_FileRoot())/".warrior-sessions",room=fs::path(Port_FileRoot())/relative;
        if(linked(base) || linked(room) || fs::weakly_canonical(room).parent_path()!=fs::weakly_canonical(base))return 0;
        std::ifstream marker(room/".room-owned",std::ios::binary);std::string line;
        if(!std::getline(marker,line) || line!="TTWROOM1" || marker.peek()!=std::char_traits<char>::eof())return 0;
        marker.close();
        /* Default iterators do not follow symlinks. Explicitly detect Windows
           junctions too, and stop before recursing into any linked directory. */
        for(auto it=fs::recursive_directory_iterator(room);it!=fs::recursive_directory_iterator();++it)
            if(linked(it->path()))return 0;
        fs::remove_all(room);ownedRooms.erase(relative);return 1;
    }catch(...){return 0;}
}
extern "C" int WarriorNetPack_ExportLocal(int selected,unsigned char **data,unsigned *size,unsigned *base) {
    if(!data || !size || !base)return 0;*data=nullptr;*size=0;*base=0;
    int count=PortWarriors_LocalCount();if(count<1 || count>PORT_WARRIOR_PLAYABLE_MAX)return 0;
    if(selected<0 || selected>=count)return 0;
    try {
        std::vector<PortWarriorLocal> local(count);
        for(int i=0;i<count;i++)if(!PortWarriors_LocalCopy(i,&local[i]))throw 1;
        std::array<int,PORT_WARRIOR_PLAYABLE_MAX> mapping;mapping.fill(-1);
        std::vector<int> order{selected};mapping[selected]=0;
        for(size_t at=0;at<order.size();at++)for(int s=0;s<4;s++) {
            int target=local[order[at]].target[s];if(target<0)continue;
            if(target>=count)throw 1;
            if(mapping[target]<0){mapping[target]=(int)order.size();order.push_back(target);}
        }
        std::sort(order.begin(),order.end());
        for(size_t i=0;i<order.size();i++)mapping[order[i]]=(int)i;
        std::vector<unsigned char> b(magic,magic+8);word(b,(unsigned)order.size());
        for(int i:order) {
            const auto &w=local[i];
            text(b,w.name);text(b,w.style);text(b,w.voice);
            for(int s=0;s<5;s++)text(b,w.attack[s]);
            word(b,w.aura);word(b,w.rules);
            for(int s=0;s<4;s++){int target=w.target[s];word(b,target<0?-1:mapping[target]);for(int k=0;k<5;k++)word(b,w.field[s][k]);}
            for(int role=0;role<PORT_WARRIOR_FILE_ROLES;role++) {
                std::ifstream in(w.file[role],std::ios::binary|std::ios::ate);
                if(!in){if(role==PORT_WARRIOR_CHIP || role>=PORT_WARRIOR_MODEL_3 || role<PORT_WARRIOR_ANIMATION){word(b,0);continue;}throw 1;}
                auto length=in.tellg();if(length<=0 || length>64*1024*1024 || b.size()+4+(size_t)length>WNP_MAX_BYTES)throw 1;
                word(b,(unsigned)length);size_t at=b.size();b.resize(at+(size_t)length);in.seekg(0);in.read((char*)b.data()+at,length);if(!in)throw 1;
            }
        }
        if(!WarriorNetPack_Check(b.data(),(unsigned)b.size()))return 0;
        auto *copy=(unsigned char*)malloc(b.size());if(!copy)return 0;
        memcpy(copy,b.data(),b.size());*data=copy;*size=(unsigned)b.size();*base=(unsigned)mapping[selected];return (int)order.size();
    }catch(...){return 0;}
}
extern "C" int WarriorNetPack_Export(int selected,unsigned char **data,unsigned *size,unsigned *base) {
    if(PortWarriors_SessionActive()) {
        if(data)*data=nullptr;if(size)*size=0;if(base)*base=0;return 0;
    }
    return WarriorNetPack_ExportLocal(selected,data,size,base);
}
extern "C" int WarriorNetPack_Install(const void *data,unsigned size,const char *prefix,int owner,char roots[][WNP_ROOT_SIZE],int capacity) {
    std::vector<fs::path> created;
    try {
        auto forms=parse(data,size);if(!prefix || !roots || owner<0 || owner>=PORT_WARRIOR_SESSION_MAX || capacity<(int)forms.size())return 0;
        std::string relative=prefix;
        if(!prefix_ok(relative) || !ownedRooms.count(relative))return 0;
        fs::path base=fs::path(Port_FileRoot())/".warrior-sessions";
        if(linked(base))return 0;fs::create_directories(base);
        fs::path room=fs::path(Port_FileRoot())/relative;
        if(linked(room))return 0;fs::create_directories(room);
        for(size_t i=0;i<forms.size();i++) {
            auto &f=forms[i];auto dir=room/folder(owner,(int)i);
            if(!fs::create_directory(dir))throw 1;created.push_back(dir);
            std::string built="#warrior 1\n",identity="name="+f.name+"\nstyle="+f.style+"\nvoice="+f.voice+"\nrules="+(f.rules?"bt4\n":"bt3\n");
            static const char *names[]={"blast1_1name","blast1_2name","blast2_1name","blast2_2name","ultimatename"};
            for(int s=0;s<5;s++)if(!f.attack[s].empty())identity+=std::string(names[s])+"="+f.attack[s]+"\n";
            if(f.aura>=0){char rgb[16];snprintf(rgb,sizeof(rgb),"%02x%02x%02x",f.aura&255,(f.aura>>8)&255,(f.aura>>16)&255);identity+="aura="+std::string(rgb)+"\n";}
            std::string mod="mode=new\n";
            for(int s=0;s<4;s++){mod+="trans"+std::to_string(s)+"=";for(int v:f.field[s])mod+=std::to_string(v)+",";
                mod+=(f.target[s]<0?"-1":"@"+folder(owner,f.target[s]));mod+='\n';}
            for(int role=0;role<PORT_WARRIOR_FILE_ROLES;role++)if(f.resource[role].size) {
                write(dir/files[role],(const unsigned char*)data+f.resource[role].offset,f.resource[role].size);
                if(role!=PORT_WARRIOR_CHIP)built+=std::string(files[role])+"\n";
            }
            write(dir/".warrior_built",built);write(dir/"warrior.txt",identity);write(dir/"mod.txt",mod);
        }
        for(size_t i=0;i<forms.size();i++)snprintf(roots[i],WNP_ROOT_SIZE,"%s/%s",prefix,folder(owner,(int)i).c_str());
        return (int)forms.size();
    }catch(...){for(auto &p:created){std::error_code e;fs::remove_all(p,e);}return 0;}
}
