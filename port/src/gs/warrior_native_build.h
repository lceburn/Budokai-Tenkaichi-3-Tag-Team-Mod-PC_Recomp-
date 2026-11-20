// Native authoring primitives. This backend never launches another program.
#pragma once
#include "warrior_recipe.h"
#include "warrior_native_forms.h"
#include "warrior_native_pack.h"
#include <functional>
#include <cstdio>
#include <set>

namespace WarriorNativeBuild {
using WarriorRecipe::Document;
namespace fs=std::filesystem;

inline bool metadata_key(const std::string &key) {
    return key=="name" || key=="title" ||
        key=="blast1_1name" || key=="blast1_2name" || key=="blast2_1name" ||
        key=="blast2_2name" || key=="ultimatename";
}
inline std::string resource_recipe(const Document &document) {
    std::string result;
    for(size_t at=0;at<document.raw.size();) {
        auto end=document.raw.find('\n',at);if(end==std::string::npos)end=document.raw.size();
        auto next=end<document.raw.size()?end+1:end;
        auto equals=document.raw.find('=',at);
        if(equals>=end || !metadata_key(document.raw.substr(at,equals-at)))
            result+=document.raw.substr(at,next-at);
        at=next;
    }
    return result;
}
inline bool metadata_only(const Document &original,const Document &edited) {
    return resource_recipe(original)==resource_recipe(edited);
}
inline bool nonvisual_only(Document original,Document edited) {
    // Voice lip tracks and fighter stats need native resource edits on save,
    // but do not change the idle assembled geometry in the editor preview.
    auto remove=[](Document &document) {
        std::string result;
        for(size_t at=0;at<document.raw.size();) {
            auto end=document.raw.find('\n',at);if(end==std::string::npos)end=document.raw.size();
            auto next=end<document.raw.size()?end+1:end;
            auto equals=document.raw.find('=',at);
            auto key=equals<end?document.raw.substr(at,equals-at):std::string();
            bool form=WarriorNativeForms::key(key);
            bool effect=key=="effects" || key=="aura" || key=="aurafrom" || key=="blast2_1colour" || key=="blast2_2colour" || key=="ultimatecolour" ||
                key=="blast1_1fx" || key=="blast1_2fx" || key=="blast2_1fx" || key=="blast2_2fx" || key=="ultimatefx";
            if(!form && !effect && document.raw.compare(at,6,"voice=") && document.raw.compare(at,6,"style="))result+=document.raw.substr(at,next-at);
            at=next;
        }
        document.raw=std::move(result);
    };
    remove(original);remove(edited);return metadata_only(original,edited);
}
struct FileEdit {fs::path path;std::string original,edited;bool model=false,resource=false;};
// A normal installation has no personal character folders until its first
// save. Initialize only an empty library; never replace an existing index or
// silently orphan character folders whose activation index is missing.
inline bool ensure_library(const fs::path &root,std::string &error) {
    std::error_code ec;auto base=fs::canonical(root,ec);
    if(ec){error="The character data folder is unavailable.";return false;}
    auto mods=base/"mods";
    if(fs::is_symlink(mods,ec)){error="The character library cannot be a linked folder.";return false;}
    ec.clear();fs::create_directory(mods,ec);
    if(ec || fs::canonical(mods,ec)!=mods || ec) {
        error="Could not open the local character library.";return false;
    }
    auto index=mods/"enabled.txt";
    if(fs::is_symlink(index,ec)){error="The character index cannot be a linked file.";return false;}
    ec.clear();
    if(fs::exists(index,ec)) {
        if(!ec && fs::is_regular_file(index,ec) && !ec)return true;
        error="The character index is unavailable.";return false;
    }
    if(ec || !fs::is_empty(mods,ec) || ec) {
        error="Character folders exist without enabled.txt. Keep them for library recovery.";return false;
    }
    // Exclusive creation avoids truncating an index written by another editor.
#ifdef _WIN32
    auto file=CreateFileW(index.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE){error="Could not initialize the character index.";return false;}
    if(!CloseHandle(file)){error="Could not finish the character index.";return false;}
#else
    std::FILE *file=std::fopen(index.c_str(),"wx");
    if(!file){error="Could not initialize the character index.";return false;}
    if(std::fclose(file)){error="Could not finish the character index.";return false;}
#endif
    error.clear();return true;
}
inline bool effect_name(const std::string &name) {
    const std::string suffix="_eff.pak";
    return name.size()>suffix.size() && name.find_first_of("/\\:")==std::string::npos &&
        name.compare(name.size()-suffix.size(),suffix.size(),suffix)==0;
}
inline bool animation_name(const std::string &name) {
    const std::string suffix="_anm.pak";
    return name.size()>suffix.size() && name.find_first_of("/\\:")==std::string::npos && name.compare(name.size()-suffix.size(),suffix.size(),suffix)==0;
}
inline bool resource_name(const std::string &name) {return effect_name(name) || animation_name(name);}
inline bool model_name(const std::string &name) {
    if(name.find_first_of("/\\:")!=std::string::npos)return false;
    for(int i=1;i<=4;i++)for(const char *damage:{"","_dmg"}) {
        auto suffix="_"+std::to_string(i)+"p"+damage+".pak";
        if(name.size()>suffix.size() && name.compare(name.size()-suffix.size(),suffix.size(),suffix)==0)return true;
    }
    return false;
}
inline bool read_file(const fs::path &path,std::string &out,bool binary=false) {
    if(!binary)return WarriorRecipe::read(path,out);
    std::ifstream file(path,std::ios::binary|std::ios::ate);if(!file)return false;
    auto size=file.tellg();if(size<0 || size>64*1024*1024)return false;
    out.resize(size_t(size));file.seekg(0);return bool(file.read(out.data(),std::streamsize(out.size())));
}

inline bool atomic_replace(const fs::path &source,const fs::path &target) {
#ifdef _WIN32
    return MoveFileExW(source.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
    std::error_code ec;fs::rename(source,target,ec);return !ec;
#endif
}
// The same filesystem operation is injectable so partial-publication rollback
// can be exercised with a real failure after at least one committed file.
using Replace=std::function<bool(const fs::path &,const fs::path &)>;
inline bool publish(const fs::path &root,const std::vector<FileEdit> &files,
                             std::string &error,Replace replace=atomic_replace) {
    std::error_code ec;
    auto canonicalRoot=fs::canonical(root,ec);
    if(ec){error="The native save root is unavailable.";return false;}
    auto mods=fs::canonical(root/"mods",ec);if(ec){error="The warrior library is unavailable.";return false;}
    if(mods.parent_path()!=canonicalRoot){error="The warrior library leaves the native save root.";return false;}
    std::set<fs::path> seen;
    for(const auto &file:files) {
        auto actual=fs::canonical(file.path,ec);
        if(ec || fs::is_symlink(file.path,ec) || ec || actual!=file.path ||
           actual.parent_path().parent_path()!=mods || !seen.insert(actual).second ||
           (file.model?!model_name(actual.filename().u8string()):file.resource?!resource_name(actual.filename().u8string()):actual.filename()!="warrior.txt" && actual.filename()!="mod.txt" && actual.filename()!="native-colours.tsv")) {
            error="A native save path leaves its warrior folder.";return false;
        }
        std::string current;
        if(!read_file(actual,current,file.model || file.resource) || current!=file.original) {
            error="A warrior changed on disk. Reopen the creator before saving.";return false;
        }
        if(file.model?file.edited.empty() || ((file.edited.size()+2047)&~size_t(2047))>WarriorNativePack::ModelLimit:file.resource?file.edited.empty() || file.edited.size()>64*1024*1024:
           file.edited.size()>WarriorRecipe::Limit || file.edited.find('\0')!=std::string::npos) {
            error="The native draft is invalid.";return false;
        }
    }
    if(files.empty())return true;
    auto builds=root/".warrior-builds";
    fs::create_directory(builds,ec);
    if(ec || fs::canonical(builds,ec).parent_path()!=canonicalRoot || ec) {
        error="The native save staging path leaves the game data.";return false;
    }
    auto job=builds/("native-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if(!fs::create_directories(job/"previous",ec) || ec){error="Could not prepare a native save.";return false;}
    std::vector<fs::path> pending,previous;
    for(size_t i=0;i<files.size();i++) {
        auto ready=job/(std::to_string(i)+".ready");
        auto backup=job/"previous"/std::to_string(i);
        if(!WarriorRecipe::write(ready,files[i].edited) || !WarriorRecipe::write(backup,files[i].original)) {
            error="Could not stage the native save. Your playable files were kept.";return false;
        }
        pending.push_back(ready);previous.push_back(backup);
    }
    size_t committed=0;
    for(;committed<files.size();committed++) {
        // Recheck each file immediately before replacement as well as before
        // staging. Another editor can change it while backups are written.
        std::string current;
        if(!read_file(files[committed].path,current,files[committed].model || files[committed].resource) || current!=files[committed].original ||
           !replace(pending[committed],files[committed].path))break;
    }
    if(committed!=files.size()) {
        bool restored=true;
        while(committed) {
            committed--;auto rollback=job/(std::to_string(committed)+".rollback");
            std::string current;
            if(!read_file(files[committed].path,current,files[committed].model || files[committed].resource) || current!=files[committed].edited) {
                restored=false;continue; // Do not destroy another editor's newer changes.
            }
            if(!WarriorRecipe::write(rollback,files[committed].original) || !atomic_replace(rollback,files[committed].path))restored=false;
        }
        error=restored?"The native save failed. Your previous playable files were restored.":
            "The native save needs recovery from its previous-files backup.";
        return false;
    }
    WarriorRecipe::write(job/"result.txt","backend=native\nfiles="+std::to_string(files.size())+"\n");
    return true;
}
inline bool publish_metadata(const fs::path &root,const std::vector<FileEdit> &files,std::string &error,Replace replace=atomic_replace) {
    for(const auto &file:files)if(file.model || file.resource){error="A metadata transaction cannot replace generated resources.";return false;}
    return publish(root,files,error,replace);
}
// Publish a newly assembled warrior, then activate it. A failed activation
// returns the untouched folder to staging; existing characters are never replaced.
inline bool publish_new(const fs::path &root,const fs::path &staged,const std::string &folder,
                        const std::string &enabledBefore,std::string &error,Replace replace=atomic_replace) {
    std::error_code ec;auto base=fs::canonical(root,ec);
    if(ec || folder.empty() || folder=="." || folder==".." || folder.find_first_of("/\\:\r\n\t")!=std::string::npos) {
        error="Invalid native warrior destination.";return false;
    }
    auto mods=fs::canonical(base/"mods",ec);
    if(ec || mods.parent_path()!=base){error="Invalid native warrior library.";return false;}
    auto builds=fs::canonical(base/".warrior-builds",ec);
    if(ec || builds.parent_path()!=base){error="Invalid native warrior staging area.";return false;}
    auto source=fs::canonical(staged,ec);
    auto relative=source.lexically_relative(builds);
    if(ec || relative.empty() || *relative.begin()==".." || source==builds || fs::is_symlink(staged,ec) || ec) {
        error="The new warrior is outside native staging.";return false;
    }
    for(fs::recursive_directory_iterator i(source,ec),end;i!=end && !ec;i.increment(ec)) {
        if(i->is_symlink(ec) || ec || (!i->is_directory(ec) && !i->is_regular_file(ec))) {
            error="The native warrior contains an unsupported file.";return false;
        }
    }
    if(ec){error="Could not validate the staged warrior.";return false;}
    for(const char *required:{"warrior.txt","mod.txt",".warrior_built"})if(!fs::is_regular_file(source/required,ec) || ec) {
        error="The native warrior is incomplete.";return false;
    }
    auto target=mods/fs::u8path(folder),enabled=mods/"enabled.txt";
    std::string current;
    if(fs::exists(target,ec) || ec || fs::is_symlink(enabled,ec) || ec ||
       !read_file(enabled,current) || current!=enabledBefore) {
        error="The warrior library changed. Reopen the creator before saving.";return false;
    }
    auto updated=enabledBefore;
    if(!updated.empty() && updated.back()!='\n')updated+='\n';
    updated+=folder+"\n";
    if(updated.size()>WarriorRecipe::Limit){error="The warrior library is full.";return false;}
    auto ready=source.parent_path()/"enabled.ready",backup=source.parent_path()/"enabled.previous";
    if(fs::exists(ready,ec) || ec || fs::exists(backup,ec) || ec ||
       !WarriorRecipe::write(ready,updated) || !WarriorRecipe::write(backup,enabledBefore)) {
        error="Could not stage native warrior activation.";return false;
    }
#ifdef _WIN32
    bool moved=MoveFileExW(source.c_str(),target.c_str(),MOVEFILE_WRITE_THROUGH)!=0;
#else
    // Both directories are beneath the same validated data root.
    fs::rename(source,target,ec);bool moved=!ec;
#endif
    if(!moved){error="Could not publish the native warrior.";return false;}
    if(!read_file(enabled,current) || current!=enabledBefore || !replace(ready,enabled)) {
        fs::rename(target,source,ec);
        error=ec?"Native activation failed; the unactivated warrior remains in the library.":
            "Native activation failed. Your previous library was kept.";
        return false;
    }
    return true;
}
// New form activation is last. Restore existing recipes/manifests if activation
// fails; refuse to overwrite a newer external edit during that restoration.
inline bool publish_new_linked(const fs::path &root,const fs::path &staged,const std::string &folder,
                              const std::string &enabledBefore,const std::vector<FileEdit> &links,
                              std::string &error,Replace replace=atomic_replace) {
    if(!publish_metadata(root,links,error,replace))return false;
    if(publish_new(root,staged,folder,enabledBefore,error,replace))return true;
    auto activationError=error;std::vector<FileEdit> reverse;
    for(auto i=links.rbegin();i!=links.rend();++i)reverse.push_back({i->path,i->edited,i->original});
    if(!publish_metadata(root,reverse,error)) {
        error="Native form activation failed; existing links need recovery from their previous-files backup.";return false;
    }
    error=activationError;return false;
}
}
