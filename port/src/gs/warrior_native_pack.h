// Bounds-checked offset-table resources used by the native character builder.
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "../../include/warrior_limits.h"

namespace WarriorNativePack {
constexpr size_t Limit=64*1024*1024;
constexpr size_t ModelLimit=PORT_WARRIOR_MODEL_CAPACITY;
inline uint32_t word(const std::string &data,size_t at) {
    const auto *p=reinterpret_cast<const unsigned char *>(data.data()+at);
    return p[0]|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;
}
inline void put(std::string &data,size_t at,uint32_t value) {
    for(unsigned i=0;i<4;i++)data[at+i]=char(value>>(i*8));
}
struct Pack {
    std::vector<std::string> entries;
    std::vector<unsigned char> flags;
    std::string headerPadding,tail;
    unsigned char endFlags=0;
};
inline bool decode(const std::string &data,Pack &result,std::string &error) {
    if(data.size()<12 || data.size()>Limit){error="Invalid resource pack size.";return false;}
    auto count=word(data,0);size_t table=(size_t(count)+2)*4;
    if(!count || count>4096 || table>data.size()){error="Invalid resource pack table.";return false;}
    std::vector<uint32_t> offsets;
    for(size_t i=0;i<=count;i++) {
        auto value=word(data,4+i*4)&~3u;
        if(value<table || value>data.size() || (!offsets.empty() && value<offsets.back())) {
            error="Resource pack offsets are outside their payload or out of order.";return false;
        }
        offsets.push_back(value);
    }
    Pack parsed;
    parsed.headerPadding=data.substr(table,offsets[0]-table);
    for(size_t i=0;i<count;i++) {
        parsed.entries.push_back(data.substr(offsets[i],offsets[i+1]-offsets[i]));
        parsed.flags.push_back(word(data,4+i*4)&3u);
    }
    parsed.endFlags=word(data,4+count*4)&3u;
    parsed.tail=data.substr(offsets.back());result=std::move(parsed);return true;
}
inline bool encode(const Pack &pack,std::string &result,std::string &error) {
    size_t count=pack.entries.size(),table=(count+2)*4;
    if(!count || count>4096 || (!pack.flags.empty() && pack.flags.size()!=count)) {
        error="Invalid native pack entry count.";return false;
    }
    size_t header=pack.headerPadding.empty()?(table+63)&~size_t(63):table+pack.headerPadding.size();
    if(header>Limit || (header&3)){error="Invalid native pack header alignment.";return false;}
    size_t total=header;
    for(const auto &entry:pack.entries) {
        if(entry.size()>Limit || ((entry.size()+63)&~size_t(63))>Limit-total) {
            error="Native resource pack exceeds its size limit.";return false;
        }
        total+=(entry.size()+63)&~size_t(63);
    }
    if(pack.tail.size()>Limit-total){error="Native resource pack tail exceeds its size limit.";return false;}
    std::string out(total+pack.tail.size(),'\0');put(out,0,uint32_t(count));
    if(!pack.headerPadding.empty())out.replace(table,pack.headerPadding.size(),pack.headerPadding);
    size_t at=header;
    for(size_t i=0;i<count;i++) {
        put(out,4+i*4,uint32_t(at)|(pack.flags.empty()?0:pack.flags[i]&3u));
        out.replace(at,pack.entries[i].size(),pack.entries[i]);
        at+=(pack.entries[i].size()+63)&~size_t(63);
    }
    put(out,4+count*4,uint32_t(at)|(pack.endFlags&3u));
    if(!pack.tail.empty())out.replace(at,pack.tail.size(),pack.tail);
    result=std::move(out);return true;
}
// Entry numbers follow the game's one-based table. An empty source entry is
// meaningful (for example, an unused lip track) and must replace the old one.
inline bool copy_entries(Pack &target,const Pack &source,unsigned first,unsigned last,std::string &error) {
    if(!first || last<first || last>target.entries.size() || last>source.entries.size()) {
        error="Donor resource pack is missing required entries.";return false;
    }
    for(size_t i=first-1;i<last;i++) {
        target.entries[i]=source.entries[i];
        if(!target.flags.empty())target.flags[i]=source.flags.empty()?0:source.flags[i];
    }
    return true;
}
inline bool copy_style(Pack &target,const Pack &source,std::string &error) {
    // Style is the reference creator's stats donor. Keep the chosen moves,
    // body parameters and technique rules outside this 48-byte stats range.
    if(target.entries.size()<18 || source.entries.size()<18 ||
       target.entries[17].size()<0x58 || source.entries[17].size()<0x58) {
        error="The style donor is missing its fighter stats.";return false;
    }
    target.entries[17].replace(0x28,0x30,source.entries[17],0x28,0x30);return true;
}
}
