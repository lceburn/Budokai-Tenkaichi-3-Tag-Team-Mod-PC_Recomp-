// Remove a saved fighter from the active library, retaining a recoverable copy.
#pragma once
#include "warrior_native_build.h"

namespace WarriorNativeDelete {
inline bool linked(const WarriorRecipe::Document &document,const std::string &folder,bool manifest=false) {
    for(int i=0;i<4;i++) {
        auto value=document.get((manifest?"trans":"form")+std::to_string(i+(manifest?0:1)));
        auto comma=value.rfind(',');
        if(comma!=std::string::npos && value.substr(comma+1)==(manifest?"@":"")+folder)return true;
    }
    return false;
}
inline bool archive(const WarriorRecipe::fs::path &root,const std::string &folder,
                    const std::string &enabledBefore,std::string &error,
                    WarriorNativeBuild::Replace replace=WarriorNativeBuild::atomic_replace) {
    namespace fs=std::filesystem;
    using namespace WarriorRecipe;
    std::error_code ec;
    auto base=fs::canonical(root,ec);
    if(ec || !WarriorNativeForms::folder_ok(folder)){error="The character folder is invalid.";return false;}
    auto mods=base/"mods",source=mods/fs::u8path(folder);
    if(fs::canonical(mods,ec)!=mods || ec || fs::canonical(source,ec)!=source || ec ||
       !fs::is_directory(source,ec) || ec) {error="The character folder leaves the saved library.";return false;}
    std::string current;
    if(!read(mods/"enabled.txt",current) || current!=enabledBefore) {
        error="The character library changed. Reopen the creator before deleting.";return false;
    }
    std::string next;unsigned removed=0;
    for(size_t at=0;at<current.size();) {
        auto end=current.find('\n',at);if(end==std::string::npos)end=current.size();
        auto after=end<current.size()?end+1:end;
        auto name=current.substr(at,end-at);if(!name.empty() && name.back()=='\r')name.pop_back();
        if(name==folder)removed++;
        else {
            next+=current.substr(at,after-at);
            if(!name.empty() && name[0]!='#') {
                fs::path path;Document recipe,manifest;
                if(!recipe_path(base,name,path) || !read(path,recipe.raw) ||
                   !read(path.parent_path()/"mod.txt",manifest.raw)) {
                    error="Another character could not be checked for transformation links.";return false;
                }
                if(linked(recipe,folder) || linked(manifest,folder,true)) {
                    error="Another character links to this form. Remove that transformation link before deleting.";return false;
                }
            }
        }
        at=after;
    }
    if(removed!=1){error="The character is not uniquely listed in the active library.";return false;}
    auto deleted=base/".warrior-deleted";
    fs::create_directory(deleted,ec);
    if(ec || fs::canonical(deleted,ec)!=deleted || ec) {
        error="The deleted-character backup folder is unavailable.";return false;
    }
    auto job=deleted/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    if(!fs::create_directory(job,ec) || ec){error="Could not prepare a character backup.";return false;}
    if(!write(job/"enabled-before.txt",current) || !write(job/"enabled-next.txt",next) ||
       !write(job/"folder.txt",folder)) {error="Could not back up the character library.";return false;}
    fs::rename(source,job/"character",ec);
    if(ec){error="Could not remove the character folder; the library is unchanged.";return false;}
    if(!replace(job/"enabled-next.txt",mods/"enabled.txt")) {
        fs::rename(job/"character",source,ec);
        error=ec?"Deletion needs recovery from the deleted-character backup.":
                 "Could not update the character library; the character was restored.";
        return false;
    }
    return true;
}
}
