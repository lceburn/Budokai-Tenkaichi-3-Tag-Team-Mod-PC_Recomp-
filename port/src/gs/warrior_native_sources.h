// Local donor assets, independent of the reference game's executable.
#pragma once
#include "warrior_native_build.h"
#include "warrior_native_pack.h"
#include <cstdlib>
#include <sstream>
#include <iomanip>

namespace WarriorNativeSources {
namespace fs=std::filesystem;
struct Model {int id=-1;WarriorNativePack::Pack pack;};
inline bool bt4_origin(const WarriorNativePack::Pack &pack) {
    return pack.entries.size()==253 && pack.entries[252].compare(0,8,"BT4WARRI")==0;
}
inline uint64_t fingerprint(const std::string &bytes) {
    uint64_t value=UINT64_C(0xcbf29ce484222325);
    for(unsigned char byte:bytes)value=(value^byte)*UINT64_C(0x100000001b3);
    return value;
}
inline bool available(const fs::path &root) {
    std::error_code ec;return fs::is_regular_file(root/"warrior-sources/index.tsv",ec);
}
inline bool generation_available(const fs::path &root) {
    std::error_code ec;return fs::is_regular_file(root/"warrior-sources/assets.tsv",ec);
}
inline bool asset(const fs::path &root,int id,std::string &result,std::string &error) {
    if(id<0 || id>=4000){error="Invalid native source file number.";return false;}
    auto cache=root/"warrior-sources";std::string index;
    if(!WarriorNativeBuild::read_file(cache/"assets.tsv",index)) {
        error="Native generation assets are unavailable.";return false;
    }
    std::istringstream lines(index);std::string line;std::set<int> seen;
    if(std::getline(lines,line) && !line.empty() && line.back()=='\r')line.pop_back();
    if(line!="# BT4 native warrior generation assets v1") {error="Unknown native generation catalogue.";return false;}
    size_t expectedSize=0;uint64_t expectedHash=0;
    while(std::getline(lines,line)) {
        if(!line.empty() && line.back()=='\r')line.pop_back();
        if(line.empty() || line[0]=='#')continue;
        auto first=line.find('\t'),second=first==std::string::npos?first:line.find('\t',first+1);
        if(first==std::string::npos || second==std::string::npos || line.find('\t',second+1)!=std::string::npos) {
            error="Malformed native generation catalogue.";return false;
        }
        auto file=line.substr(0,first),size=line.substr(first+1,second-first-1),hash=line.substr(second+1);
        if(file.empty() || file.size()>4 || file.find_first_not_of("0123456789")!=std::string::npos || size.empty() || size.size()>8 ||
           size.find_first_not_of("0123456789")!=std::string::npos || hash.size()!=16 || hash.find_first_not_of("0123456789abcdefABCDEF")!=std::string::npos) {
            error="Invalid native generation asset record.";return false;
        }
        int number=std::atoi(file.c_str());size_t bytes=std::strtoul(size.c_str(),nullptr,10);
        if(number>=4000 || !bytes || bytes>WarriorNativePack::Limit || !seen.insert(number).second) {
            error="Invalid or duplicate native generation source.";return false;
        }
        if(number==id){expectedSize=bytes;expectedHash=std::strtoull(hash.c_str(),nullptr,16);}
    }
    if(!expectedSize){error="A required native generation asset is missing.";return false;}
    std::ostringstream filename;filename<<std::setfill('0')<<std::setw(5)<<id<<".bin";
    auto path=cache/"files"/filename.str();std::error_code ec;auto parent=fs::canonical(cache/"files",ec);
    if(ec || fs::is_symlink(path,ec) || ec || fs::canonical(path,ec).parent_path()!=parent || ec ||
       !WarriorNativeBuild::read_file(path,result,true) || result.size()!=expectedSize || fingerprint(result)!=expectedHash) {
        error="A native generation asset is missing, unsafe or has changed.";return false;
    }
    return true;
}
struct CatalogueEntry {int id;std::string name;size_t size;uint64_t hash;};
inline std::string identity_key(std::string value) {
    // Legacy recipes spell the same donors differently. This does not map
    // legacy numeric roster positions onto BT4's replaced characters.
    for(size_t at=0;(at=value.find("(Med)",at))!=std::string::npos;at+=5)value.replace(at,5,"(Mid)");
    std::string key;
    for(unsigned char c:value) {
        if(c==' ' || c=='-' || c=='#' || c=='(' || c==')')continue;
        key+=char(c>='A' && c<='Z'?c-'A'+'a':c);
    }
    return key;
}
inline bool catalogue(const fs::path &root,std::vector<CatalogueEntry> &result,std::string &error) {
    auto cache=root/"warrior-sources";std::string index;
    if(!WarriorNativeBuild::read_file(cache/"index.tsv",index)) {
        error="Native donor assets are unavailable.";return false;
    }
    std::istringstream lines(index);std::string line;std::set<int> ids;std::set<std::string> names;
    std::vector<CatalogueEntry> entries;
    if(std::getline(lines,line) && !line.empty() && line.back()=='\r')line.pop_back();
    if(line!="# BT4 native warrior sources v1") {
        error="Unknown native donor asset catalogue.";return false;
    }
    while(std::getline(lines,line)) {
        if(!line.empty() && line.back()=='\r')line.pop_back();
        if(line.empty() || line[0]=='#')continue;
        std::vector<std::string> fields;size_t at=0;
        for(;;) {auto tab=line.find('\t',at);fields.push_back(line.substr(at,tab-at));if(tab==std::string::npos)break;at=tab+1;}
        if(fields.size()!=4 || fields[0].empty() || fields[0].size()>3 || fields[0].find_first_not_of("0123456789")!=std::string::npos ||
           !WarriorRecipe::value_ok(fields[1]) || fields[2].empty() || fields[2].size()>8 || fields[2].find_first_not_of("0123456789")!=std::string::npos ||
           fields[3].size()!=16 || fields[3].find_first_not_of("0123456789abcdefABCDEF")!=std::string::npos) {
            error="Malformed native donor asset catalogue.";return false;
        }
        int id=std::atoi(fields[0].c_str());auto bytes=std::strtoul(fields[2].c_str(),nullptr,10);
        if(id<0 || id>=250 || bytes<12 || bytes>WarriorNativePack::Limit || !ids.insert(id).second || !names.insert(fields[1]).second) {
            error="Invalid or duplicate native donor asset.";return false;
        }
        entries.push_back({id,fields[1],bytes,std::strtoull(fields[3].c_str(),nullptr,16)});
    }
    if(entries.empty()){error="The native donor catalogue is empty.";return false;}
    result=std::move(entries);error.clear();return true;
}
inline bool model(const fs::path &root,const std::string &name,Model &result,std::string &error) {
    std::vector<CatalogueEntry> entries;if(!catalogue(root,entries,error))return false;
    int found=-1;size_t expectedSize=0;uint64_t expectedHash=0;
    for(const auto &entry:entries)if(entry.name==name){found=entry.id;expectedSize=entry.size;expectedHash=entry.hash;}
    if(found<0) {
        const auto key=identity_key(name);
        for(const auto &entry:entries)if(identity_key(entry.name)==key) {
            if(found>=0){error="The legacy donor name is ambiguous in this BT4 catalogue.";return false;}
            found=entry.id;expectedSize=entry.size;expectedHash=entry.hash;
        }
    }
    if(found<0){error="This source fighter is not in the native donor cache.";return false;}
    auto cache=root/"warrior-sources";
    auto path=cache/"models"/(std::to_string(found)+".pak");std::error_code ec;
    auto parent=fs::canonical(cache/"models",ec);
    if(ec || fs::is_symlink(path,ec) || ec || fs::canonical(path,ec).parent_path()!=parent || ec) {
        error="The native donor model path is unavailable or unsafe.";return false;
    }
    std::string data;
    if(!WarriorNativeBuild::read_file(path,data,true) || data.size()!=expectedSize || fingerprint(data)!=expectedHash) {
        error="The native donor asset is incomplete or has changed.";return false;
    }
    Model loaded;loaded.id=found;
    if(!WarriorNativePack::decode(data,loaded.pack,error))return false;
    if(loaded.pack.entries.size()<252 || loaded.pack.entries[17].size()<0x58) {
        error="The native donor model is missing required metadata.";return false;
    }
    result=std::move(loaded);return true;
}
}
