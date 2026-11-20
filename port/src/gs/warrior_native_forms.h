// Native links between generated custom forms. No model rebuild is required.
#pragma once
#include "warrior_recipe.h"
#include <set>
#include <sstream>
namespace WarriorNativeForms {
inline bool key(const std::string &name) {
    return name.size()==5 && name.compare(0,4,"form")==0 && name[4]>='1' && name[4]<='4';
}
inline WarriorRecipe::Document without_links(const WarriorRecipe::Document &document) {
    WarriorRecipe::Document result;
    for(size_t at=0;at<document.raw.size();) {
        auto end=document.raw.find('\n',at);if(end==std::string::npos)end=document.raw.size();
        auto next=end<document.raw.size()?end+1:end,equals=document.raw.find('=',at);
        if(equals>=end || !key(document.raw.substr(at,equals-at)))result.raw+=document.raw.substr(at,next-at);
        at=next;
    }
    return result;
}
inline bool folder_ok(const std::string &folder) {
    if(!WarriorRecipe::value_ok(folder) || folder=="." || folder==".." ||
       folder.find_first_of("/\\:")!=std::string::npos || folder.back()=='.' || folder.back()==' ')return false;
    return true;
}
inline bool record(const std::string &value,unsigned slot,const std::set<std::string> &family,
                   const std::string &self,std::string &out,std::string &error) {
    if(slot>=4){error="Invalid transformation slot.";return false;}
    if(value.empty()){out=slot==3?"0,1,1,-1,0,-1":"1,0,2,-1,0,-1";return true;}
    auto first=value.find(','),second=first==std::string::npos?first:value.find(',',first+1);
    if(first==std::string::npos || second==std::string::npos) {error="Invalid native form link.";return false;}
    auto cost=value.substr(0,first),mode=value.substr(first+1,second-first-1),target=value.substr(second+1);
    if(cost.size()!=1 || cost[0]<'0' || cost[0]>'5' || mode!="0" ||
       !folder_ok(target) || target==self || !family.count(target)) {
        error="Choose another saved form and a stock cost from zero to five.";return false;
    }
    out=cost+",0,2,-1,0,@"+target;return true;
}
inline bool replace(WarriorRecipe::Document &document,const std::string &key,const std::string &value) {
    std::string prefix=key+"=";size_t found=std::string::npos;unsigned count=0;
    for(size_t at=0;at<document.raw.size();) {
        auto end=document.raw.find('\n',at);if(end==std::string::npos)end=document.raw.size();
        if(document.raw.compare(at,prefix.size(),prefix)==0){found=at;count++;}
        at=end+1;
    }
    if(count>1)return false;
    if(found!=std::string::npos) {
        auto end=document.raw.find('\n',found);if(end==std::string::npos)end=document.raw.size();
        if(end>found && document.raw[end-1]=='\r')end--;
        document.raw.replace(found,end-found,prefix+value);
    }else {
        if(!document.raw.empty() && document.raw.back()!='\n')document.raw+='\n';
        document.raw+=prefix+value+"\n";
    }
    return document.raw.size()<=WarriorRecipe::Limit;
}
inline bool update(const WarriorRecipe::Document &before,const WarriorRecipe::Document &edited,
                   WarriorRecipe::Document &manifest,const std::set<std::string> &family,
                   const std::string &self,std::string &error) {
    auto next=manifest;
    for(unsigned i=0;i<4;i++) {
        auto k="form"+std::to_string(i+1);
        if(before.get(k)==edited.get(k))continue;
        std::string value;
        if(!record(edited.get(k),i,family,self,value,error))return false;
        if(!replace(next,"trans"+std::to_string(i),value)){error="Invalid native form manifest.";return false;}
    }
    manifest=std::move(next);return true;
}
}
