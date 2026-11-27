// Rebuild existing characters without changing the router's resource filenames.
#pragma once
#include "warrior_native_build.h"
#include <iterator>
#include <sstream>
namespace WarriorNativeRegenerate {
using namespace WarriorNativeBuild;
inline int role(const std::string &name) {
    if(name.find_first_of("\\:")!=std::string::npos)return -1;
    if(model_name(name)) {
        for(int i=1;i<=4;i++)for(int damage=0;damage<2;damage++) {
            auto suffix="_"+std::to_string(i)+"p"+(damage?"_dmg":"")+".pak";
            if(name.size()>suffix.size() && name.compare(name.size()-suffix.size(),suffix.size(),suffix)==0)
                return i+damage*4;
        }
    }
    if(animation_name(name))return 9;
    if(effect_name(name))return 10;
    const std::string suffix="_img.cdbt";
    if(name.find('/')==std::string::npos && name.size()>suffix.size() &&
       name.compare(name.size()-suffix.size(),suffix.size(),suffix)==0)return 11;
    if(name.size()==17 && name.compare(0,8,"pzs3us1/")==0 &&
       name.compare(13,4,".bin")==0 && name.substr(8,5).find_first_not_of("0123456789")==std::string::npos)return 12;
    return -1;
}
inline bool manifest(const fs::path &folder,std::vector<std::string> &out) {
    std::string raw;if(!read_file(folder/".warrior_built",raw))return false;
    std::istringstream lines(raw);std::string line;std::set<std::string> seen;
    while(std::getline(lines,line)) {
        if(!line.empty() && line.back()=='\r')line.pop_back();
        if(line.empty() || line[0]=='#')continue;
        if(role(line)<0 || !seen.insert(line).second || out.size()>=13)return false;
        out.push_back(line);
    }
    return out.size()>=7;
}
inline bool plan(const fs::path &source,const fs::path &staged,std::vector<FileEdit> &out,std::string &error) {
    std::vector<std::string> oldNames,newNames;
    if(!manifest(source,oldNames) || !manifest(staged,newNames) || oldNames.size()!=newNames.size()) {
        error="This edit changes the costume layout, which is not supported by native replacement yet.";return false;
    }
    std::vector<FileEdit> pending;
    for(size_t i=0;i<oldNames.size();i++) {
        int kind=role(oldNames[i]);
        if(kind!=role(newNames[i])) {error="The rebuilt resource order is incompatible.";return false;}
        std::string before,after;
        if(!read_file(source/fs::u8path(oldNames[i]),before,true) ||
           !read_file(staged/fs::u8path(newNames[i]),after,true)) {
            error="The native warrior is missing a generated resource.";return false;
        }
        if(before==after)continue;
        pending.push_back({source/fs::u8path(oldNames[i]),std::move(before),std::move(after),kind<=8,kind==9 || kind==10,kind==11 || kind==12});
    }
    for(const char *name:{"warrior.txt","mod.txt","native-colours.tsv"}) {
        std::string before,after;
        if(!read_file(source/name,before) || !read_file(staged/name,after)) {
            error="The native warrior is missing authoring metadata.";return false;
        }
        if(before!=after)pending.push_back({source/name,std::move(before),std::move(after)});
    }
    // Keep routed filenames while recording that an older generated model
    // has been refreshed. Publish this marker in the same transaction.
    std::string built;
    if(read_file(source/".warrior_built",built) && built.compare(0,12,"#warrior 47\n")==0) {
        auto updated=built;updated.replace(0,12,"#warrior 48\n");
        pending.push_back({source/".warrior_built",std::move(built),std::move(updated)});
    }
    out.insert(out.end(),std::make_move_iterator(pending.begin()),std::make_move_iterator(pending.end()));
    error.clear();return true;
}
}
