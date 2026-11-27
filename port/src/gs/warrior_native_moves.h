// Common technique resources when a body borrows a complete moves donor.
// Per-attack overrides and motion-pack assembly are separate operations.
#pragma once
#include "warrior_native_pack.h"
namespace WarriorNativeMoves {
inline bool motions(const WarriorNativePack::Pack &body,const WarriorNativePack::Pack &moves,
                    WarriorNativePack::Pack &out,std::string &error) {
    if(body.entries.size()!=440 || moves.entries.size()!=440 ||
       (!body.flags.empty() && body.flags.size()!=440) ||
       (!moves.flags.empty() && moves.flags.size()!=440)) {
        error="Unsupported native motion pack layout.";return false;
    }
    auto result=moves;
    // Default means the complete Move Set, including stance. Independent
    // animation group selections are applied after this baseline.
    out=std::move(result);error.clear();return true;
}
inline bool common(WarriorNativePack::Pack &model,const WarriorNativePack::Pack &moves,std::string &error) {
    if(model.entries.size()<252 || moves.entries.size()<252 ||
       model.entries[17].size()!=256 || moves.entries[17].size()!=256) {
        error="Incomplete native common-move source.";return false;
    }
    auto result=model;
    result.entries[17]=moves.entries[17];
    // The body owns both size/attachment parameters. The remaining parameters come from
    // the moves donor before any independent voice/style/attack overrides.
    result.entries[17].replace(4,8,model.entries[17].substr(4,8));
    for(size_t i=18;i<52;i++) {
        // Secondary chains and animation scaling remain body-owned.
        if(i==22 || i==25)continue;
        result.entries[i]=moves.entries[i];
    }
    model=std::move(result);error.clear();return true;
}
}
