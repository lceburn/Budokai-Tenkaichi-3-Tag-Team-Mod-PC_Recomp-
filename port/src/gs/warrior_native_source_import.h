// One-time asset import for native creation. Reads data, never executable code.
#pragma once
#include "warrior_native_sources.h"
#include <iomanip>

namespace WarriorNativeSourceImport {
namespace fs=std::filesystem;
inline bool inside(const fs::path &path,const fs::path &parent) {
    auto a=path.begin();
    for(auto b=parent.begin();b!=parent.end();++b,++a)if(a==path.end() || *a!=*b)return false;
    return true;
}
inline bool prepare(const fs::path &sourceData,const fs::path &cache,std::string &error,uint64_t &bytes) {
    std::error_code ec;auto source=fs::canonical(sourceData/"pzs3us1",ec);
    if(ec){error="The extracted BT3 source archive is unavailable.";return false;}
    auto output=fs::canonical(cache,ec);
    if(ec || inside(output,fs::canonical(sourceData,ec)) || ec) {
        error="Choose a native cache outside the source installation.";return false;
    }
    if(!fs::is_regular_file(output/"index.tsv",ec) || ec) {
        error="Prepare the native donor catalogue before generation assets.";return false;
    }
    if(fs::exists(output/"assets.tsv",ec) || ec || !fs::create_directory(output/"files",ec) || ec) {
        error="The generation-asset destination must be new.";return false;
    }
    std::set<int> wanted;
    for(int i=0;i<161;i++) {
        for(int part=0;part<10;part++)wanted.insert(1423+i*10+part);
        wanted.insert(760+i);wanted.insert(7+i*2);wanted.insert(8+i*2);
    }
    std::ostringstream index;index<<"# BT4 native warrior generation assets v1\n";bytes=0;
    for(int id:wanted) {
        std::ostringstream file;file<<std::setfill('0')<<std::setw(5)<<id<<".bin";
        auto path=source/file.str();std::string data;
        if(fs::is_symlink(path,ec) || ec || fs::canonical(path,ec).parent_path()!=source || ec ||
           !WarriorNativeBuild::read_file(path,data,true) || data.empty()) {
            error="A native generation source file is missing, linked or too large.";return false;
        }
        if(id>=1423 && (id-1423)%10==0) {
            std::string previous;
            if(!WarriorNativeBuild::read_file(output/"models"/(std::to_string((id-1423)/10)+".pak"),previous,true) || previous!=data) {
                error="The generation assets do not match the prepared donor catalogue.";return false;
            }
        }
        if(!WarriorRecipe::write(output/"files"/file.str(),data)) {
            error="Could not write the native generation assets.";return false;
        }
        index<<std::dec<<id<<'\t'<<data.size()<<'\t'<<std::hex<<std::setfill('0')<<std::setw(16)
             <<WarriorNativeSources::fingerprint(data)<<'\n';bytes+=data.size();
    }
    if(!WarriorRecipe::write(output/"assets.ready",index.str()) || !WarriorNativeBuild::atomic_replace(output/"assets.ready",output/"assets.tsv")) {
        error="Could not publish the native generation catalogue.";return false;
    }
    return true;
}
}
