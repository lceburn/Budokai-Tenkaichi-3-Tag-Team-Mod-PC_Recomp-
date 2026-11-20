// Lossless recipe edits. Generated models and manifests are never rewritten here.
#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <chrono>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace WarriorRecipe {
namespace fs = std::filesystem;
constexpr size_t Limit = 128 * 1024;
inline bool read(const fs::path &path, std::string &out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    out.clear(); char block[4096];
    while (f.read(block, sizeof(block)) || f.gcount()) {
        out.append(block, size_t(f.gcount()));
        if (out.size() > Limit) return false;
    }
    return !f.bad();
}
inline bool value_ok(const std::string &v) {
    if (v.empty() || v.size() >= 192 || v.find_first_not_of(" \t") == std::string::npos) return false;
    for (unsigned char c : v) if (c < 32 || c == 127) return false;
    return true;
}
struct Document {
    std::string raw;
    bool valid(std::string &error) const {
        if (raw.size() > Limit || raw.find('\0') != std::string::npos) { error="Recipe is too large or contains invalid text."; return false; }
        int name=0,voice=0;
        for (size_t at=0; at<raw.size();) {
            size_t end=raw.find('\n',at); if(end==std::string::npos)end=raw.size();
            std::string line=raw.substr(at,end-at); if(!line.empty() && line.back()=='\r')line.pop_back();
            if(!line.empty() && line[0]!='#' && line.find('=')==std::string::npos) {error="Malformed recipe line.";return false;}
            if(line.compare(0,5,"name=")==0)name++;
            if(line.compare(0,6,"voice=")==0)voice++;
            at=end+1;
        }
        if(name!=1 || voice>1 || !value_ok(get("name"))) {error="Recipe has missing or duplicate identity fields.";return false;}
        return true;
    }
    std::string get(const std::string &key) const {
        const std::string prefix=key+"=";
        for(size_t at=0;at<raw.size();) {
            size_t end=raw.find('\n',at);if(end==std::string::npos)end=raw.size();
            if(raw.compare(at,prefix.size(),prefix)==0) {
                std::string v=raw.substr(at+prefix.size(),end-at-prefix.size());
                if(!v.empty() && v.back()=='\r')v.pop_back();return v;
            }
            at=end+1;
        }
        return {};
    }
    bool set(const std::string &key,const std::string &value) {
        if((key!="name" && key!="voice") || !value_ok(value))return false;
        const std::string prefix=key+"=";
        for(size_t at=0;at<raw.size();) {
            size_t end=raw.find('\n',at);if(end==std::string::npos)end=raw.size();
            if(raw.compare(at,prefix.size(),prefix)==0) {
                size_t last=end;if(last>at && raw[last-1]=='\r')last--;
                raw.replace(at,last-at,prefix+value);return raw.size()<=Limit;
            }
            at=end+1;
        }
        std::string nl=raw.find("\r\n")!=std::string::npos?"\r\n":"\n";
        if(!raw.empty() && raw.back()!='\n')raw+=nl;
        raw+=prefix+value+nl;return raw.size()<=Limit;
    }
};
inline bool recipe_path(const fs::path &root,const std::string &folder,fs::path &out) {
    if(folder.empty() || folder=="." || folder==".." || folder.find_first_of("/\\:")!=std::string::npos || folder.back()=='.' || folder.back()==' ')return false;
    for(unsigned char c:folder)if(c<32)return false;
    std::error_code ec;
    auto mods=fs::canonical(root/"mods",ec);if(ec)return false;
    auto dir=fs::canonical(mods/fs::u8path(folder),ec);if(ec || dir.parent_path()!=mods)return false;
    out=dir/"warrior.txt";
    auto actual=fs::canonical(out,ec);return !ec && actual.parent_path()==dir && actual==out;
}
inline bool write(const fs::path &path,const std::string &bytes) {
    std::ofstream f(path,std::ios::binary|std::ios::trunc);
    if(!f)return false;f.write(bytes.data(),bytes.size());f.flush();bool ok=bool(f);f.close();return ok && bool(f);
}
inline bool save(const fs::path &path,const Document &original,const Document &edited,std::string &error) {
    if(!edited.valid(error))return false;
    std::string current;
    if(!read(path,current) || current!=original.raw){error="Recipe changed on disk. Reload before saving.";return false;}
    std::error_code ec;
    fs::path backup=path;backup += ".editor-backup";
    if(!fs::exists(backup,ec)) {
        if(ec || !fs::copy_file(path,backup,fs::copy_options::none,ec) || ec){error="Could not create the recipe backup.";return false;}
    }
    fs::path temp=path;temp += ".editor-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".tmp";
    if(!write(temp,edited.raw)){fs::remove(temp,ec);error="Could not write the edited recipe.";return false;}
#ifdef _WIN32
    bool replaced=MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
    fs::rename(temp,path,ec);bool replaced=!ec;
#endif
    if(!replaced){fs::remove(temp,ec);error="Could not replace the recipe; the original is unchanged.";return false;}
    return true;
}
}
