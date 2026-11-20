// The reference's head record contains both skin and hair draw batches.
// Auxiliary hair materials, excluding materials also used by the body/face,
// identify the hair batches. The remaining batches follow the head source.
#pragma once
#include "warrior_native_head_chains.h"
namespace WarriorNativeHeadPieces {
struct Batch {size_t offset,size;uint32_t material;};
inline bool batches(const std::string &mesh,const WarriorNativeParts::Record &record,
                    std::vector<Batch> &out,std::string &error) {
    if(record.offset>mesh.size() || record.size>mesh.size()-record.offset || record.size<64 ||
       (record.size>64 && record.size<96)){error="Invalid native head batch extent.";return false;}
    std::vector<Batch> result;size_t end=record.offset+record.size,total=0;
    for(size_t dma=record.offset+96;dma+16<=end;) {
        auto tag=WarriorNativePack::word(mesh,dma);size_t bytes=(tag&65535)*16;
        if(bytes>end-dma-16){error="Truncated native head DMA packet.";return false;}
        size_t words=bytes/4+2;
        for(size_t j=0;j+23<=words;) {
            size_t at=dma+8+j*4;if(WarriorNativePack::word(mesh,at)!=0x6c058000)break;
            size_t count=WarriorNativePack::word(mesh,at+52)&0x7fff;
            if(count>(words-j-23)/12 || count>4096-total){error="Invalid native head batch vertices.";return false;}
            total+=count;
            uint32_t low=WarriorNativePack::word(mesh,at+20),high=WarriorNativePack::word(mesh,at+24);
            result.push_back({at,92+count*48,(low&0x3fff)|((high<<11)&0x3fff0000)});
            j+=23+count*12;
        }
        dma+=16+bytes;if((tag&0xf0000000)==0x60000000)break;
    }
    out=std::move(result);error.clear();return true;
}
inline bool hair_materials(const std::string &mesh,std::set<uint32_t> &out,std::string &error) {
    std::vector<WarriorNativeParts::Record> rows;
    if(!WarriorNativeParts::records(mesh,rows,error))return false;
    std::set<uint32_t> hair,skin;
    for(size_t i=0;i<rows.size();i++) {
        bool owned=WarriorNativeHeadChains::owned(rows,i);
        if(!owned && rows[i].node>=46 && rows[i].node!=48)continue;
        std::vector<Batch> draws;if(!batches(mesh,rows[i],draws,error))return false;
        auto &keys=owned?hair:skin;
        for(const auto &batch:draws)keys.insert(batch.material);
        if(keys.size()>32){error="The native head material classification exceeds the reference limit.";return false;}
    }
    for(auto key:skin)hair.erase(key);
    out=std::move(hair);error.clear();return true;
}
inline bool mixed_record(const std::string &originalBody,const std::string &originalDonor,
                         const std::string &body,const std::string &donor,
                         const WarriorNativeParts::Record &base,const WarriorNativeParts::Record &source,
                         unsigned mask,std::string &out,std::string &error) {
    std::set<uint32_t> bodyHair,donorHair;
    std::vector<Batch> bodyDraws,donorDraws;
    if(!hair_materials(originalBody,bodyHair,error) || !hair_materials(originalDonor,donorHair,error) ||
       !batches(originalBody,base,bodyDraws,error) || !batches(originalDonor,source,donorDraws,error))return false;
    std::string payload;
    for(unsigned part:{1u,4u}) {
        bool imported=mask&part;const auto &draws=imported?donorDraws:bodyDraws;
        const auto &keys=imported?donorHair:bodyHair;const auto &data=imported?donor:body;
        for(const auto &batch:draws)if(bool(keys.count(batch.material))==(part==4)) {
            if(batch.offset>data.size() || batch.size>data.size()-batch.offset ||
               batch.size>WarriorNativePack::Limit-payload.size()){error="Invalid native head piece payload.";return false;}
            payload+=data.substr(batch.offset,batch.size);
        }
    }
    auto result=body.substr(base.offset,std::min(size_t(96),base.size));
    if(payload.empty())result.resize(64);
    else {
        if(result.size()!=96){error="A native mixed head has no drawing bounds.";return false;}
        size_t packet=(8+payload.size()+15)&~size_t(15),qwc=(packet-16)/16;
        if(qwc>65535 || packet>WarriorNativePack::Limit-96){error="The native mixed head packet is too large.";return false;}
        result.resize(96+packet,'\0');WarriorNativePack::put(result,96,0x60000000u|unsigned(qwc));
        result.replace(104,payload.size(),payload);
    }
    WarriorNativePack::put(result,0,unsigned(result.size()));out=std::move(result);error.clear();return true;
}
}
