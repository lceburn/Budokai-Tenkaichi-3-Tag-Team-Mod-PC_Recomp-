// Native creator's draft library and in-process resource generation. The
// reference builder remains a fallback only for paths not yet implemented.
#include "warrior_recipe.h"
#include "warrior_native_build.h"
#include "warrior_native_sources.h"
#include "warrior_native_stock.h"
#include "warrior_native_regenerate.h"
#include "warrior_native_effects.h"
#include "warrior_native_attack_names.h"
#include "../port_addr.h"
#include "../../../include/menu/warrior_fields.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <set>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <map>
#ifndef _WIN32
#include <unistd.h>
#endif
extern "C" void Port_FileWarriorClose(void);
extern "C" void PortWarriors_Reload(void);
extern "C" int PortWarriors_Preview(int on);
extern "C" void WarriorView_Clear(void);

namespace {
using namespace WarriorRecipe;
struct Surface {unsigned key;int texture,rgb[3];};
struct Draft {std::string folder;fs::path source;Document original,edited;bool fresh=false;std::vector<Surface> colours;};
struct Library {
    fs::path root;std::vector<Draft> entries;std::set<std::string> roster;
    std::vector<WarriorNativeSources::CatalogueEntry> donors;
    int selected=-1,nativeResult=0;std::string status,enabled;SDL_Process *process=nullptr;
} library;
std::map<std::string,std::array<std::string,5>> attackNames;
struct PreviewJob {
    SDL_Process *process=nullptr;fs::path drafts;
    std::string wanted,running,applied,failed;
    Uint64 changed=0;bool ready=false,saveRequested=false;
    std::vector<std::pair<std::string,fs::path>> cache;
} previewJob;
int copy_out(const std::string &value,uint32_t address,int capacity) {
    if(!address || capacity<1 || capacity>1024)return 0;
    auto out=static_cast<char *>(Port_Ptr(address));
    std::snprintf(out,size_t(capacity),"%s",value.c_str());return 1;
}
Draft *selected() {return library.selected>=0 && library.selected<int(library.entries.size())?&library.entries[library.selected]:nullptr;}
std::set<std::string> native_family() {
    std::set<std::string> result;for(const auto &entry:library.entries)result.insert(entry.folder);return result;
}
bool patch_field(Document &document,const std::string &key,const std::string &value) {
    if(value.size()>512 || value.find_first_of("\r\n")!=std::string::npos || value.find('\0')!=std::string::npos)return false;
    std::string out,prefix=key+"=";bool found=false;
    for(size_t at=0;at<document.raw.size();) {
        size_t end=document.raw.find('\n',at);if(end==std::string::npos)end=document.raw.size();
        size_t next=end<document.raw.size()?end+1:end;
        auto line=document.raw.substr(at,next-at);
        if(document.raw.compare(at,prefix.size(),prefix)==0) {
            if(!found && !value.empty()) {
                std::string nl=line.size()>=2 && line.substr(line.size()-2)=="\r\n"?"\r\n":!line.empty() && line.back()=='\n'?"\n":"";
                out+=prefix+value+nl;
            }
            found=true;
        }else out+=line;
        at=next;
    }
    if(!found && !value.empty()) {
        std::string nl=document.raw.find("\r\n")!=std::string::npos?"\r\n":"\n";
        if(!out.empty() && out.back()!='\n')out+=nl;
        out+=prefix+value+nl;
    }
    if(out.size()>Limit)return false;
    document.raw=std::move(out);return true;
}
const char *const headParts[]={"hair","head","face"};
const char *const outfitParts[]={"chest","waist","upperarms","forearms","hands","hips","shins","feet","accessories"};
bool patch_choice(Document &document,int field,const std::string &value) {
    if(field!=ZW_HEADSET && field!=ZW_BODYSET)return patch_field(document,zwFields[field].key,value);
    Document changed=document;
    if(field==ZW_HEADSET){for(auto key:headParts)if(!patch_field(changed,key,value))return false;}
    else {for(auto key:outfitParts)if(!patch_field(changed,key,value))return false;}
    document=std::move(changed);return true;
}
std::string field_value(const Draft &draft,int field) {
    // The reference's whole-head control groups hair, skull and face. A virtual
    // control here must not introduce a new recipe key unknown to the builder.
    if(field==ZW_HEADSET) {
        auto value=draft.edited.get("head");
        for(auto key:headParts)if(draft.edited.get(key)!=value)return "Mixed";
        return value;
    }
    if(field==ZW_BODYSET) {
        auto value=draft.edited.get("chest");
        for(auto key:outfitParts)if(draft.edited.get(key)!=value)return "Mixed";
        return value;
    }
    auto value=draft.edited.get(zwFields[field].key);
    if(value.empty() && field==ZW_VOICE)value=draft.edited.get("body");
    if(value.empty() && field==ZW_STYLE)value=draft.edited.get("moves");
    return value;
}
bool copy_assets(const fs::path &source,const fs::path &dest) {
    std::error_code ec;
    if(source.empty())return true;
    if(fs::is_directory(source/"paint",ec)) {
        fs::copy(source/"paint",dest/"paint",fs::copy_options::recursive|fs::copy_options::skip_symlinks,ec);
        if(ec)return false;
    }
    for(const char *name:{"portrait_custom.bin","hud_photo.raw","chip_photo.raw"})
        if(fs::is_regular_file(source/name,ec) && !fs::is_symlink(source/name,ec)) {
            fs::copy_file(source/name,dest/name,fs::copy_options::none,ec);if(ec)return false;
        }
    return true;
}
}

static SDL_Process *start_worker(std::vector<std::string> &command) {
#ifdef _WIN32
    // The interpreter itself must be windowless, in addition to suppressing
    // the reference executable's console in the worker's subprocess call.
    if(!command.empty()) {
        auto windowless=fs::u8path(command[0]).parent_path()/"pythonw.exe";
        std::error_code ec;
        if(fs::is_regular_file(windowless,ec))command[0]=windowless.u8string();
    }
#endif
    std::vector<const char *> args;for(auto &part:command)args.push_back(part.c_str());args.push_back(nullptr);
    auto props=SDL_CreateProperties();
    SDL_SetPointerProperty(props,SDL_PROP_PROCESS_CREATE_ARGS_POINTER,args.data());
    SDL_SetNumberProperty(props,SDL_PROP_PROCESS_CREATE_STDIN_NUMBER,SDL_PROCESS_STDIO_NULL);
    SDL_SetNumberProperty(props,SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER,SDL_PROCESS_STDIO_APP);
    SDL_SetBooleanProperty(props,SDL_PROP_PROCESS_CREATE_STDERR_TO_STDOUT_BOOLEAN,true);
    auto process=SDL_CreateProcessWithProperties(props);SDL_DestroyProperties(props);return process;
}
static bool preview_apply(const fs::path &path) {
    std::error_code ec;auto relative=fs::relative(path,library.root,ec).generic_u8string();
    if(ec || relative.compare(0,16,".warrior-builds/") || relative.find("..")!=std::string::npos)return false;
#ifdef _WIN32
    int owner=int(GetCurrentProcessId());
#else
    int owner=int(getpid());
#endif
    if(!WarriorRecipe::write(library.root/(".warrior-preview-"+std::to_string(owner)+"-path.txt"),relative+"\n"))return false;
    WarriorView_Clear();Port_FileWarriorClose();
    return PortWarriors_Preview(owner)!=0;
}
static void preview_poll() {
    auto &p=previewJob;if(!p.process)return;int code=0;
    if(!SDL_WaitProcess(p.process,false,&code))return;
    size_t size=0;void *output=SDL_ReadProcess(p.process,&size,&code);
    if(output){std::fprintf(stderr,"warrior preview: %.*s\n",int(std::min(size,size_t(4096))),static_cast<char *>(output));SDL_free(output);}
    SDL_DestroyProcess(p.process);p.process=nullptr;
    std::string path;
    if(!code && WarriorRecipe::read(p.drafts/"preview-result.txt",path)) {
        auto built=fs::u8path(path);
        if(p.cache.size()>=16)p.cache.erase(p.cache.begin());
        p.cache.push_back({p.running,built});
        if(p.running==p.wanted && !p.saveRequested && preview_apply(built)) {p.ready=true;p.applied=p.running;}
    }else p.failed=p.running;
}
extern "C" void PortZwNative_PreviewStop() {
    auto &p=previewJob;p.wanted.clear();p.applied.clear();p.ready=false;
    WarriorView_Clear();Port_FileWarriorClose();PortWarriors_Preview(0);
}
extern "C" int PortZwNative_Preview(int field,uint32_t highlighted) {
    auto &p=previewJob;auto e=selected();if(!e || library.process || p.saveRequested)return 0;
    Document draft=e->edited;
    if(field>=0 && field<ZW_FIELD_COUNT && highlighted) {
        auto text=static_cast<const char *>(Port_Ptr(highlighted));size_t n=0;while(n<=512 && text[n])n++;
        if(n>512)return -2;std::string v(text,n);
        if(!patch_choice(draft,field,v))return -2;
    }
    if(!e->fresh && draft.raw==e->original.raw) {
        if(!p.wanted.empty() || p.ready)PortZwNative_PreviewStop();
        preview_poll();return 0;
    }
    // Metadata does not alter the assembled model. Keep this warrior's saved
    // resource binding, rather than generating or binding a temporary slot.
    if(!e->fresh && WarriorNativeBuild::nonvisual_only(e->original,draft)) {
        if(!p.wanted.empty() || p.ready)PortZwNative_PreviewStop();
        preview_poll();return 3;
    }
    // Asset origins are part of the identity even when two recipes match.
    auto key=e->source.u8string()+"\n"+draft.raw;
    if(p.wanted!=key){p.wanted=key;p.changed=SDL_GetTicks();}
    preview_poll();
    if(p.applied==key && p.ready)return 1;
    if(p.failed==key)return -2;
    for(auto &cached:p.cache)if(cached.first==key) {
        if(preview_apply(cached.second)){p.ready=true;p.applied=key;return 1;}
        p.failed=key;return -2;
    }
    if(p.process || SDL_GetTicks()-p.changed<180)return p.ready?2:-1;
    if((e->fresh || WarriorNativeStock::supports(e->original)) &&
       WarriorNativeStock::supports(draft) && WarriorNativeSources::generation_available(library.root)) {
        auto built=library.root/".warrior-builds"/("native-preview-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))/"Preview";
        std::string error;
        if(!WarriorNativeStock::generate(library.root,draft,built,error,native_family(),e->folder) || !preview_apply(built)) {
            library.status=error.empty()?"Could not display the native warrior preview.":error;p.failed=key;return -2;
        }
        if(p.cache.size()>=16)p.cache.erase(p.cache.begin());
        p.cache.push_back({key,built});p.ready=true;p.applied=key;
        std::fprintf(stderr,"warrior native stock preview: helper=none\n");return 1;
    }
    const char *python=std::getenv("BT3_WARRIOR_PYTHON"),*tool=std::getenv("BT3_WARRIOR_BUILD_TOOL"),*helper=std::getenv("BT3_WARRIOR_BUILDER_COPY");
    if(!python || !tool || !helper)return -2;
    std::error_code ec;
    p.drafts=library.root/".warrior-preview-drafts"/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    auto dest=p.drafts/"Preview";fs::create_directories(dest,ec);
    if(ec || !write(dest/"warrior.txt",draft.raw) || !copy_assets(e->source,dest)){p.failed=key;return -2;}
    std::vector<std::string> command={python,tool,"--builder-copy",helper,"--data",library.root.u8string(),"--drafts",p.drafts.u8string(),"--folder","Preview","--preview"};
    p.running=key;p.process=start_worker(command);if(!p.process)p.failed=key;
    return p.ready?2:-1;
}

extern "C" int PortZwNative_Open() {
    attackNames.clear();
    auto &l=library;if(l.process)return int(l.entries.size());
    l.entries.clear();l.roster.clear();l.donors.clear();l.selected=-1;l.nativeResult=0;l.status.clear();
    const char *data=std::getenv("BT3_DATA");l.root=fs::u8path(data?data:"gamedata");
    if(WarriorNativeSources::available(l.root)) {
        std::string error;
        if(!WarriorNativeSources::catalogue(l.root,l.donors,error))l.status=error;
    }
    std::string list,rows;
    if(read(l.root/"native-roster.tsv",rows))for(size_t at=0;at<rows.size();) {
        size_t end=rows.find('\n',at);if(end==std::string::npos)end=rows.size();
        auto line=rows.substr(at,end-at);if(!line.empty() && line.back()=='\r')line.pop_back();
        auto first=line.find('\t'),last=first==std::string::npos?first:line.find('\t',first+1);
        if(first!=std::string::npos && last!=std::string::npos) {
            auto name=line.substr(first+1,last-first-1),form=line.substr(last+1);
            if(!form.empty())name+=" "+form;l.roster.insert(name);
        }at=end+1;
    }
    if(read(l.root/"mods/enabled.txt",list))for(size_t at=0;at<list.size() && l.entries.size()<32;) {
        size_t end=list.find('\n',at);if(end==std::string::npos)end=list.size();
        auto name=list.substr(at,end-at);if(!name.empty() && name.back()=='\r')name.pop_back();at=end+1;
        if(name.empty() || name[0]=='#')continue;
        fs::path path;Draft entry;std::string error;
        if(!recipe_path(l.root,name,path) || !read(path,entry.original.raw) || !entry.original.valid(error))continue;
        entry.folder=name;entry.source=path.parent_path();entry.edited=entry.original;
        std::string catalogue;
        if(read(entry.source/"native-colours.tsv",catalogue)) {
            std::istringstream lines(catalogue);std::string line;
            while(std::getline(lines,line) && entry.colours.size()<48) {
                Surface s;
                if(std::sscanf(line.c_str(),"%u\t%d\t%d\t%d\t%d",&s.key,&s.texture,&s.rgb[0],&s.rgb[1],&s.rgb[2])==5 &&
                   (s.key&0xe000001f)==0x40000010 && s.texture>=0 && s.texture<128 &&
                   s.rgb[0]>=0 && s.rgb[0]<=255 && s.rgb[1]>=0 && s.rgb[1]<=255 && s.rgb[2]>=0 && s.rgb[2]<=255)
                    entry.colours.push_back(s);
            }
        }
        l.entries.push_back(std::move(entry));
    }
    l.enabled=list;
    if(!l.entries.empty())l.selected=0;
    if(std::getenv("BT3_TAG_TRACE"))std::fprintf(stderr,"warrior-menu open count=%d\n",int(l.entries.size()));
    return int(l.entries.size());
}
extern "C" int PortZwNative_Count(){return int(library.entries.size());}
extern "C" int PortZwNative_Select(int index){if(index<0 || index>=int(library.entries.size()) || library.process)return 0;library.selected=index;return 1;}
extern "C" int PortZwNative_List(int index,uint32_t out,int capacity,int folder) {
    if(index<0 || index>=int(library.entries.size()))return 0;
    auto &e=library.entries[index];return copy_out(folder?e.folder:e.edited.get("name"),out,capacity);
}
extern "C" int PortZwNative_Read(int field,uint32_t out,int capacity) {
    auto e=selected();return e && field>=0 && field<ZW_FIELD_COUNT?copy_out(field_value(*e,field),out,capacity):0;
}
extern "C" int PortZwNative_Status(uint32_t out,int capacity){return copy_out(library.status,out,capacity);}
extern "C" int PortZwNative_Dirty(){for(auto &e:library.entries)if(e.fresh || e.edited.raw!=e.original.raw)return 1;return 0;}
extern "C" int PortZwNative_ColourCount() {
    auto e=selected();if(!e || e->fresh)return 0;
    // A changed mesh needs a build before its texture identities can be chosen.
    for(int field=0;field<ZW_FIELD_COUNT;field++)if(field==ZW_BODY || zwFields[field].tab==1 ||
        (field>=ZW_LOOK2 && field<=ZW_LOOK4))
        if(e->edited.get(zwFields[field].key)!=e->original.get(zwFields[field].key))return 0;
    return int(e->colours.size());
}
extern "C" int PortZwNative_ColourValue(int index,int channel) {
    auto e=selected();if(!e || index<0 || index>=PortZwNative_ColourCount() || channel<0 || channel>3)return 0;
    const auto &s=e->colours[index];if(channel==3)return s.texture;
    std::istringstream lines(e->edited.raw);std::string line;
    while(std::getline(lines,line)) {
        unsigned key;int rgb[3];
        if(std::sscanf(line.c_str(),"colour=%u,%d,%d,%d",&key,&rgb[0],&rgb[1],&rgb[2])==4 && key==s.key)
            return std::max(0,std::min(255,rgb[channel]));
    }
    return s.rgb[channel];
}
extern "C" int PortZwNative_ColourWrite(int index,int red,int green,int blue) {
    auto e=selected();if(!e || library.process || index<0 || index>=PortZwNative_ColourCount() ||
        red<0 || red>255 || green<0 || green>255 || blue<0 || blue>255)return 0;
    auto key=e->colours[index].key;std::string output;bool changed=false;int count=0;
    auto replacement="colour="+std::to_string(key)+","+std::to_string(red)+","+std::to_string(green)+","+std::to_string(blue);
    for(size_t at=0;at<e->edited.raw.size();) {
        auto end=e->edited.raw.find('\n',at);if(end==std::string::npos)end=e->edited.raw.size();
        auto next=end<e->edited.raw.size()?end+1:end;auto line=e->edited.raw.substr(at,next-at);
        unsigned existing;int r,g,b,used=0;
        if(line.compare(0,7,"colour=")==0)count++;
        if(std::sscanf(line.c_str(),"colour=%u,%d,%d,%d%n",&existing,&r,&g,&b,&used)==4 && existing==key) {
            if(!changed)output+=replacement+line.substr(size_t(used));changed=true;
        }else output+=line;
        at=next;
    }
    if(!changed) {
        if(count>=48){library.status="This warrior already uses the maximum number of material colour changes.";return 0;}
        if(!output.empty() && output.back()!='\n')output+='\n';output+=replacement+"\n";
    }
    if(output.size()>Limit)return 0;e->edited.raw=std::move(output);return 1;
}
extern "C" int PortZwNative_DonorAllowed(int field,uint32_t name) {
    if(!name)return 0;
    if(field==ZW_VOICE || field==ZW_STYLE)return library.roster.count(static_cast<const char *>(Port_Ptr(name)))!=0;
    if(!library.donors.empty()) {
        const char *value=static_cast<const char *>(Port_Ptr(name));
        for(const auto &entry:library.donors)if(entry.name==value)return 1;
        return 0;
    }
    return !WarriorNativeSources::available(library.root);
}
extern "C" int PortZwNative_DonorName(int index,uint32_t out,int capacity) {
    if(library.donors.empty())return WarriorNativeSources::available(library.root)?0:-1;
    if(index<0 || index>=int(library.donors.size()))return 0;
    return copy_out(library.donors[index].name,out,capacity);
}
extern "C" int PortZwNative_AttackName(uint32_t donor,int slot,uint32_t out,int capacity) {
    if(!donor || slot<0 || slot>=5 || !out || capacity<1 || capacity>1024)return 0;
    copy_out("",out,capacity);
    const char *source=static_cast<const char *>(Port_Ptr(donor));size_t length=0;
    while(length<=128 && source[length])length++;
    if(length>128 || !value_ok(std::string(source,length)))return 0;
    std::string key(source,length);auto found=attackNames.find(key);
    if(found==attackNames.end()) {
        WarriorNativeSources::Model model;std::array<std::string,5> names;std::string error;
        if(!WarriorNativeSources::model(library.root,key,model,error) || !WarriorNativeAttackNames::extract(model.pack,names,error))return 0;
        found=attackNames.emplace(key,std::move(names)).first;
    }
    if(found->second[slot].empty())return 0;
    return copy_out(found->second[slot],out,capacity);
}
extern "C" int PortZwNative_Write(int field,uint32_t value) {
    auto e=selected();if(!e || library.process || field<0 || field>=ZW_FIELD_COUNT || !value)return 0;
    const auto text=static_cast<const char *>(Port_Ptr(value));size_t n=0;while(n<=512 && text[n])n++;
    if(n>512)return 0;std::string v(text,n);
    const bool sourceChoice=zwFields[field].kind==ZW_PART || zwFields[field].kind==ZW_DONOR || zwFields[field].kind==ZW_ATTACK;
    if(!v.empty() && (!zw_native_editable(field) || (sourceChoice && (v=="none" || v.find(", costume ")!=std::string::npos)))) {
        library.status="This option is not supported by the native creator yet. Your draft was kept.";return 0;
    }
    if((field==ZW_NAME || field==ZW_TITLE) && !value_ok(v))return 0;
    // Reference attack labels are optional overrides, stored in 32-byte slots.
    // Reject overlong edits instead of letting the builder truncate UTF-8.
    if(field>=ZW_BLAST11NAME && field<=ZW_ULTIMATENAME &&
        (!v.empty() && (!value_ok(v) || v.size()>31)))return 0;
    if((field==ZW_BODY || field==ZW_MOVES || field==ZW_VOICE || field==ZW_STYLE) && !value_ok(v))return 0;
    if((field==ZW_VOICE || field==ZW_STYLE) && !library.roster.count(v))return 0;
    if(zwFields[field].kind==ZW_NUMBER && !v.empty()) {
        char *end=nullptr;long number=std::strtol(v.c_str(),&end,10);
        if(!end || *end || number<50 || number>180)return 0;
        if(number==100)v.clear();
    }
    if(zwFields[field].kind==ZW_RGB && !v.empty() && (v.size()!=6 || v.find_first_not_of("0123456789abcdefABCDEF")!=std::string::npos))return 0;
    return patch_choice(e->edited,field,v);
}
static int new_warrior(int copyForm,bool linkForm) {
    auto &l=library;if(l.process)return -1;
    if(l.entries.size()>=4){l.status="The current prototype supports four custom forms at a time.";return -1;}
    Draft entry;
    if(copyForm>=0 && copyForm<int(l.entries.size())) {
        entry.edited=l.entries[copyForm].edited;entry.source=l.entries[copyForm].source;
        for(int field=ZW_FORM1;field<=ZW_FORM4;field++)patch_field(entry.edited,zwFields[field].key,"");
        if(linkForm) {
            patch_field(entry.edited,"name",entry.edited.get("name").substr(0,165)+" (Form)");
            patch_field(entry.edited,"form1","1,0,"+l.entries[copyForm].folder);
        }else {
            auto base=entry.edited.get("name");
            if(base.size()>165){base.resize(165);while(!base.empty() && (static_cast<unsigned char>(base.back())&0xc0)==0x80)base.pop_back();
                if(!base.empty() && static_cast<unsigned char>(base.back())>=0xc0)base.pop_back();}
            std::string candidate=base+" Copy";int suffix=2;
            for(;;) {
                bool exists=false;for(auto &e:l.entries)if(e.edited.get("name")==candidate){exists=true;break;}
                if(!exists)break;candidate=base+" Copy "+std::to_string(suffix++);
            }
            patch_field(entry.edited,"name",candidate);
        }
    }else entry.edited.raw="name=New Warrior\ntitle=Z Warrior\nbody=Goku (End)\nmoves=Goku (End)\nvoice=Goku (End)\nstyle=Goku (End)\n";
    entry.folder="Warrior - Custom "+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    entry.fresh=true;l.entries.push_back(std::move(entry));int added=int(l.entries.size())-1;
    if(linkForm && copyForm>=0 && copyForm<added) {
        auto &base=l.entries[copyForm];bool linked=false;
        for(int field=ZW_FORM1;field<=ZW_FORM4;field++)if(base.edited.get(zwFields[field].key).empty()) {
            patch_field(base.edited,zwFields[field].key,"1,0,"+l.entries[added].folder);linked=true;break;
        }
        if(!linked){l.entries.pop_back();l.status="This warrior already has four transformation links.";return -1;}
    }
    l.selected=added;return added;
}
extern "C" int PortZwNative_New(int copyForm){return new_warrior(copyForm,true);}
extern "C" int PortZwNative_Duplicate(int source) {
    if(source<0 || source>=int(library.entries.size()))return -1;
    return new_warrior(source,false);
}
static std::string native_source(const Document &document,const char *key) {
    auto value=document.get(key);
    if(value.empty())value=document.get(!std::strcmp(key,"voice")?"body":"moves");
    if(value.empty())value=document.get("body");return value;
}
static int save_native_stock() {
    auto &l=library;Draft *fresh=nullptr;
    for(auto &e:l.entries) {
        if(e.fresh) {
            if(fresh || !WarriorNativeStock::supports(e.edited))return 0;
            fresh=&e;
        }else if(!WarriorNativeBuild::metadata_only(WarriorNativeForms::without_links(e.original),
                                                   WarriorNativeForms::without_links(e.edited)))return 0;
    }
    if(!fresh || !WarriorNativeSources::generation_available(l.root))return 0;
    if(!WarriorNativeBuild::ensure_library(l.root,l.status))return -1;
    if(l.entries.size()>4){l.status="The prototype supports four custom forms.";return -1;}
    for(const auto &e:l.entries)if(!e.fresh) {
        std::string current;
        if(!read(e.source/"warrior.txt",current) || current!=e.original.raw) {
            l.status="A warrior changed on disk. Reopen the creator before saving.";return -1;
        }
    }
    for(const char *key:{"voice","style"})if(!l.roster.count(native_source(fresh->edited,key))) {
        l.status="Choose a style and voice present in this BT4 roster.";return -1;
    }
    const auto family=native_family();std::vector<WarriorNativeBuild::FileEdit> links;
    for(const auto &e:l.entries)if(!e.fresh && e.original.raw!=e.edited.raw) {
        Document manifest;
        if(!read(e.source/"mod.txt",manifest.raw)){l.status="The generated form manifest is unavailable.";return -1;}
        auto before=manifest.raw;
        if(!e.edited.valid(l.status) || !WarriorNativeForms::update(e.original,e.edited,manifest,family,e.folder,l.status))return -1;
        if(e.original.get("name")!=e.edited.get("name"))patch_field(manifest,"white",e.edited.get("name"));
        if(e.original.get("title")!=e.edited.get("title"))patch_field(manifest,"yellow",e.edited.get("title"));
        links.push_back({e.source/"warrior.txt",e.original.raw,e.edited.raw});
        if(before!=manifest.raw)links.push_back({e.source/"mod.txt",std::move(before),std::move(manifest.raw)});
    }
    auto staged=l.root/".warrior-builds"/("native-create-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))/"Warrior";
    if(!WarriorNativeStock::generate(l.root,fresh->edited,staged,l.status,family,fresh->folder))return -1;
    Port_FileWarriorClose();
    if(!WarriorNativeBuild::publish_new_linked(l.root,staged,fresh->folder,l.enabled,links,l.status))return -1;
    std::fprintf(stderr,"warrior native stock save: helper=none\n");
    PortWarriors_Reload();PortZwNative_Open();l.nativeResult=1;
    l.status="Saved in BT4 without the creator helper. Your character is ready for the next fight.";return 1;
}
static int save_native_stock_edits() {
    auto &l=library;bool changed=false;
    for(const auto &e:l.entries) {
        if(e.fresh)return 0;
        if(e.original.raw==e.edited.raw)continue;
        if(!WarriorNativeStock::supports(e.original) || !WarriorNativeStock::supports(e.edited))return 0;
        changed=true;
    }
    if(!changed || !WarriorNativeSources::generation_available(l.root))return 0;
    std::vector<WarriorNativeBuild::FileEdit> files;
    for(const auto &e:l.entries) {
        if(e.original.raw==e.edited.raw)continue;
        auto staged=l.root/".warrior-builds"/("native-edit-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))/"Warrior";
        if(!WarriorNativeStock::generate(l.root,e.edited,staged,l.status,native_family(),e.folder))return -1;
        std::string current;
        if(!read(e.source/"warrior.txt",current) || current!=e.original.raw) {
            l.status="A warrior changed on disk. Reopen the creator before saving.";return -1;
        }
        if(!WarriorNativeRegenerate::plan(e.source,staged,files,l.status))return -1;
    }
    Port_FileWarriorClose();
    if(!WarriorNativeBuild::publish(l.root,files,l.status))return -1;
    std::fprintf(stderr,"warrior native stock edits: files=%u helper=none\n",unsigned(files.size()));
    PortWarriors_Reload();PortZwNative_Open();l.nativeResult=1;
    l.status="Saved in BT4 without the creator helper. Your characters are ready for the next fight.";return 1;
}
static int save_native_metadata() {
    auto &l=library;std::vector<WarriorNativeBuild::FileEdit> files;
    bool resources=false,forms=false,effects=false;std::set<std::string> family;
    for(const auto &e:l.entries) {
        if(e.fresh || !WarriorNativeBuild::nonvisual_only(e.original,e.edited))return 0;
        family.insert(e.folder);
        for(int i=1;i<=4;i++)forms|=e.original.get("form"+std::to_string(i))!=e.edited.get("form"+std::to_string(i));
        effects|=WarriorNativeEffects::changed(e.original,e.edited);
        resources|=native_source(e.original,"voice")!=native_source(e.edited,"voice") || native_source(e.original,"style")!=native_source(e.edited,"style");
        resources|=e.original.get("aurafrom")!=e.edited.get("aurafrom");
    }
    // The bridge remains available during migration. A source cache contains
    // donor data only: loading it never executes the reference game.
    if(resources && !WarriorNativeSources::available(l.root))return 0;
    if(effects && !WarriorNativeSources::generation_available(l.root))return 0;
    if(forms) {
        std::string enabled;
        if(!read(l.root/"mods/enabled.txt",enabled) || enabled!=l.enabled || family.size()>4) {
            l.status="The warrior library changed. Reopen the creator before saving form links.";return -1;
        }
    }
    for(const auto &e:l.entries) {
        std::string error;
        if(!e.edited.valid(error)){l.status=error;return -1;}
        auto effectiveStyle=native_source(e.edited,"style"),effectiveVoice=native_source(e.edited,"voice");
        if(!l.roster.count(effectiveStyle) || !l.roster.count(effectiveVoice)) {
            l.status="Choose a style and voice present in this BT4 roster.";return -1;
        }
        if(e.original.raw==e.edited.raw)continue;
        files.push_back({e.source/"warrior.txt",e.original.raw,e.edited.raw});
        bool effectsChanged=WarriorNativeEffects::changed(e.original,e.edited);
        if(effectsChanged) {
            std::string built;if(!read(e.source/".warrior_built",built)){l.status="The generated effect list is unavailable.";return -1;}
            std::istringstream lines(built);std::string name;unsigned count=0;
            while(std::getline(lines,name)) {
                if(!name.empty() && name.back()=='\r')name.pop_back();
                if(!WarriorNativeBuild::effect_name(name))continue;
                if(++count>1){l.status="The warrior has duplicate effect resources.";return -1;}
                auto path=e.source/fs::u8path(name);std::error_code ec;
                if(fs::is_symlink(path,ec) || ec || fs::canonical(path,ec).parent_path()!=e.source || ec) {
                    l.status="The generated effect path is invalid.";return -1;
                }
                std::string before,after;WarriorNativePack::Pack pack;
                if(!WarriorNativeBuild::read_file(path,before,true) || !WarriorNativePack::decode(before,pack,l.status) ||
                   !WarriorNativeEffects::update(l.root,pack,e.original,e.edited,l.status) || !WarriorNativePack::encode(pack,after,l.status))return -1;
                files.push_back({path,std::move(before),std::move(after),false,true});
            }
            if(count!=1){l.status="The generated effect resource is missing.";return -1;}
        }
        bool voiceChanged=native_source(e.original,"voice")!=effectiveVoice;
        bool styleChanged=native_source(e.original,"style")!=effectiveStyle;
        bool auraChanged=e.original.get("aurafrom")!=e.edited.get("aurafrom");
        WarriorNativeSources::Model voiceSource,styleSource,auraSource;
        if(voiceChanged && !WarriorNativeSources::model(l.root,effectiveVoice,voiceSource,l.status))return -1;
        if(styleChanged && !WarriorNativeSources::model(l.root,effectiveStyle,styleSource,l.status))return -1;
        if(auraChanged && !WarriorNativeSources::model(l.root,WarriorNativeAura::source(e.edited),auraSource,l.status))return -1;
        if(voiceChanged || styleChanged || auraChanged) {
            std::string built;
            if(!read(e.source/".warrior_built",built) || built.compare(0,9,"#warrior ")) {
                l.status="The generated warrior resource list is unavailable.";return -1;
            }
            std::istringstream lines(built);std::string name;std::set<std::string> models;
            while(std::getline(lines,name)) {
                if(!name.empty() && name.back()=='\r')name.pop_back();
                if(!WarriorNativeBuild::model_name(name))continue;
                auto path=e.source/fs::u8path(name);std::error_code ec;
                if(!models.insert(name).second || fs::is_symlink(path,ec) || ec || fs::canonical(path,ec).parent_path()!=e.source || ec) {
                    l.status="A generated model path is invalid.";return -1;
                }
                std::string original,updated;WarriorNativePack::Pack pack;
                if(!WarriorNativeBuild::read_file(path,original,true) || !WarriorNativePack::decode(original,pack,l.status))return -1;
                if(voiceChanged && !WarriorNativePack::copy_entries(pack,voiceSource.pack,53,252,l.status))return -1;
                if(styleChanged && !WarriorNativePack::copy_style(pack,styleSource.pack,l.status))return -1;
                if(auraChanged && !WarriorNativeAura::apply(pack,auraSource.pack,l.status))return -1;
                if(!WarriorNativePack::encode(pack,updated,l.status))return -1;
                files.push_back({path,std::move(original),std::move(updated),true});
            }
            if(models.size()<2){l.status="The native save is missing its intact or damaged model.";return -1;}
        }
        // Keep generated manifest captions consistent with the recipe.
        // Technique labels are resolved directly from the recipe by BT4.
        Document manifest;
        if(!read(e.source/"mod.txt",manifest.raw)){l.status="The generated warrior manifest is unavailable.";return -1;}
        auto original=manifest.raw;
        if(e.original.get("name")!=e.edited.get("name"))patch_field(manifest,"white",e.edited.get("name"));
        if(e.original.get("title")!=e.edited.get("title"))patch_field(manifest,"yellow",e.edited.get("title"));
        if(voiceChanged)patch_field(manifest,"voice",std::to_string(voiceSource.id));
        if(styleChanged)patch_field(manifest,"template",std::to_string(styleSource.id));
        if(!WarriorNativeForms::update(e.original,e.edited,manifest,family,e.folder,l.status))return -1;
        if(manifest.raw!=original)files.push_back({e.source/"mod.txt",original,manifest.raw});
    }
    Port_FileWarriorClose();
    if(!WarriorNativeBuild::publish(l.root,files,l.status))return -1;
    std::fprintf(stderr,"warrior native %s: files=%u helper=none\n",resources || effects?"resources":"metadata",unsigned(files.size()));
    PortWarriors_Reload();PortZwNative_Open();
    l.status="Saved in BT4 without the creator helper. Your characters are ready for the next fight.";
    l.nativeResult=1;return 1;
}
extern "C" int PortZwNative_Build() {
    auto &l=library;if(l.process || !selected())return 0;
    if(previewJob.process){previewJob.saveRequested=true;return 1;}
    struct RestoreRejectedPreview {
        bool ready=previewJob.ready;
        std::string applied=previewJob.applied,wanted=previewJob.wanted;
        ~RestoreRejectedPreview() {
            if(!ready || library.nativeResult || library.process)return;
            for(const auto &cached:previewJob.cache)if(cached.first==applied && preview_apply(cached.second)) {
                previewJob.ready=true;previewJob.applied=applied;previewJob.wanted=wanted;break;
            }
        }
    } restoreRejectedPreview;
    PortZwNative_PreviewStop();
    int stock=save_native_stock();if(stock)return stock>0;
    int native=save_native_metadata();
    if(native)return native>0;
    int edits=save_native_stock_edits();if(edits)return edits>0;
    const char *python=std::getenv("BT3_WARRIOR_PYTHON"),*tool=std::getenv("BT3_WARRIOR_BUILD_TOOL"),*helper=std::getenv("BT3_WARRIOR_BUILDER_COPY");
    if(!python || !tool || !helper) {
        l.status="The native creator cannot save this draft. Your edits are still in the editor.";
        if(!WarriorNativeSources::generation_available(l.root))l.status="Creator source data is missing. Restart through Play to prepare it. Your draft is still in the editor.";
        else for(const auto &entry:l.entries) {
            if(!entry.fresh && entry.edited.raw==entry.original.raw)continue;
            for(int field=0;field<ZW_FIELD_COUNT;field++) {
                if(field==ZW_HEADSET || field==ZW_BODYSET)continue;
                const auto value=entry.edited.get(zwFields[field].key);
                if(!value.empty() && (!zw_native_editable(field) || value=="none" || value.find(", costume ")!=std::string::npos)) {
                    l.status=std::string("Native saving does not yet support: ")+zwFields[field].label+". Reset this setting to Default; your draft was kept.";break;
                }
            }
        }
        std::fprintf(stderr,"warrior native save rejected: %s\n",l.status.c_str());return 0;
    }
    for(auto &e:l.entries)if(!e.fresh) {
        std::string current;
        if(!read(e.source/"warrior.txt",current) || current!=e.original.raw){l.status="A warrior changed on disk. Reopen the creator before saving.";return 0;}
    }
    std::error_code ec;
    fs::path drafts=l.root/".warrior-drafts"/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    fs::create_directories(drafts,ec);if(ec){l.status="Could not prepare the character drafts.";return 0;}
    std::vector<std::string> command={python,tool,"--builder-copy",helper,"--data",l.root.u8string(),"--drafts",drafts.u8string(),"--enable"};
    for(auto &e:l.entries) {
        auto dest=drafts/fs::u8path(e.folder);fs::create_directory(dest,ec);
        if(ec || !write(dest/"warrior.txt",e.edited.raw) || !copy_assets(e.source,dest)){l.status="Could not stage the character's editor assets.";return 0;}
        if(e.fresh || e.edited.raw!=e.original.raw){command.push_back("--folder");command.push_back(e.folder);}
    }
    if(!PortZwNative_Dirty()){command.push_back("--folder");command.push_back(selected()->folder);}
    Port_FileWarriorClose();l.process=start_worker(command);
    if(!l.process){l.status="The local character builder could not be started.";return 0;}
    l.status="Building and validating your characters...";return 1;
}
extern "C" int PortZwNative_Poll() {
    if(library.nativeResult){int result=library.nativeResult;library.nativeResult=0;return result;}
    if(previewJob.saveRequested) {
        preview_poll();if(previewJob.process)return 0;
        previewJob.saveRequested=false;
        if(!PortZwNative_Build())return 2;
        return 0;
    }
    auto &l=library;if(!l.process)return 2;int code=0;
    if(!SDL_WaitProcess(l.process,false,&code))return 0;
    size_t size=0;int readCode=code;void *output=SDL_ReadProcess(l.process,&size,&readCode);
    std::string details=output?std::string(static_cast<char *>(output),std::min(size,size_t(8192))):"";
    if(output){std::fprintf(stderr,"warrior builder: %.*s\n",int(std::min(size,size_t(4096))),static_cast<char *>(output));SDL_free(output);}
    SDL_DestroyProcess(l.process);l.process=nullptr;
    if(code){
        if(details.find("[WinError 5]")!=std::string::npos || details.find("[WinError 32]")!=std::string::npos)
            l.status="Another game or tool is using this character's files. Close it and save again. Your playable characters were kept.";
        else l.status="The draft could not be built or saved. Your playable characters were kept. Check the builder log before trying other parts or moves.";
        return 2;
    }
    Port_FileWarriorClose();PortWarriors_Reload();PortZwNative_Open();
    l.status="Built and saved. Your characters are ready for the next fight.";return 1;
}
extern "C" void PortZwNative_Trace(int page,int row,int tab) {
    if(std::getenv("BT3_TAG_TRACE"))std::fprintf(stderr,"warriorui page=%d row=%d tab=%d\n",page,row,tab);
}
