// Native assembly of the editor's default, unmodified stock-based warrior.
#pragma once
#include "warrior_native_sources.h"
#include "warrior_native_materials.h"
#include "warrior_native_head.h"
#include "warrior_native_moves.h"
#include "warrior_native_hands.h"
#include "warrior_native_body_model.h"
#include "warrior_native_effects.h"
#include "warrior_native_aura.h"
#include "warrior_native_motion_groups.h"
#include "warrior_native_attacks.h"
#include <cmath>
#include <cstring>

namespace WarriorNativeStock {
namespace fs=std::filesystem;
using WarriorRecipe::Document;
inline bool head_edit(const Document &recipe) {
    return !recipe.get("hair").empty() || !recipe.get("head").empty() || !recipe.get("face").empty();
}
inline bool experimental_head(const Document &recipe) {
    const char *enabled=std::getenv("BT3_WARRIOR_NATIVE_HEAD_TEST");
    if(!enabled || std::string(enabled)!="1")return false;
    bool any=false;
    for(const char *key:{"head","face","hair"}) {
        const auto source=recipe.get(key);if(source.empty())continue;
        if(source=="none" || source.find(", costume ")!=std::string::npos)return false;
        any=true;
    }
    return any;
}
inline bool hand_edit(const Document &recipe) {return !recipe.get("hands").empty();}
inline bool experimental_hands(const Document &recipe) {
    const char *enabled=std::getenv("BT3_WARRIOR_NATIVE_HAND_TEST");
    const auto donor=recipe.get("hands");
    return enabled && std::string(enabled)=="1" && !donor.empty() && donor!="none" &&
        donor.find(", costume ")==std::string::npos;
}
inline unsigned body_slot(const std::string &key) {
    if(key=="chest")return 3;if(key=="waist")return 4;if(key=="upperarms")return 5;
    if(key=="forearms")return 6;if(key=="hips")return 8;if(key=="shins")return 9;if(key=="feet")return 10;
    return 0;
}
inline bool body_edit(const Document &recipe) {
    for(const char *key:{"chest","waist","upperarms","forearms","hips","shins","feet"})if(!recipe.get(key).empty())return true;
    return false;
}
inline bool experimental_body(const Document &recipe) {
    const char *enabled=std::getenv("BT3_WARRIOR_NATIVE_BODY_TEST");
    if(!enabled || std::string(enabled)!="1" || recipe.get("body").empty() ||
       (head_edit(recipe) && !experimental_head(recipe)))return false;
    if(hand_edit(recipe) && !experimental_hands(recipe))return false;
    unsigned count=0;
    for(const char *key:{"chest","waist","upperarms","forearms","hips","shins","feet"})if(!recipe.get(key).empty()) {
        const auto selected=recipe.get(key);
        // Source identity is not a capability test. Generation validates the
        // selected hierarchy, fitting records, material bindings and size.
        if(selected=="none" || selected.find(", costume ")!=std::string::npos)return false;
        count++;
    }
    return count>=1;
}
inline bool recipe_colours(const Document &recipe,std::vector<WarriorNativeMaterials::Colour> &out,const std::string &field="colour") {
    std::istringstream lines(recipe.raw);std::string line;std::set<uint32_t> keys;
    while(std::getline(lines,line)) {
        if(!line.empty() && line.back()=='\r')line.pop_back();
        auto prefix=field+"=";if(line.compare(0,prefix.size(),prefix))continue;
        std::istringstream values(line.substr(prefix.size()));uint64_t key;int r,g,b;char a,c,d,tail;
        if(!(values>>key>>a>>r>>c>>g>>d>>b) || values.get(tail) || a!=',' || c!=',' || d!=',' ||
           key>UINT32_MAX || (key&0xe000001f)!=0x40000010 || !keys.insert(uint32_t(key)).second ||
           r<0 || r>255 || g<0 || g>255 || b<0 || b>255 || out.size()>=512)return false;
        out.push_back({uint32_t(key),r,g,b});
    }
    return true;
}
inline bool supports(const Document &recipe) {
    // Broaden this only when the corresponding assembly paths are verified.
    if(recipe.get("moves").empty())return false;
    if(recipe.get("body").empty())return false;
    if(head_edit(recipe) && !experimental_head(recipe))return false;
    if(hand_edit(recipe) && !experimental_hands(recipe))return false;
    if(body_edit(recipe) && !experimental_body(recipe))return false;
    std::istringstream lines(recipe.raw);std::string line;
    while(std::getline(lines,line)) {
        if(!line.empty() && line.back()=='\r')line.pop_back();
        if(line.empty() || line[0]=='#')continue;
        auto equals=line.find('=');if(equals==std::string::npos)return false;
        auto key=line.substr(0,equals);
        auto category=WarriorNativeMotionGroups::group(key);
        if(category==2 || (category>=0 && line.substr(equals+1).find(", costume ")!=std::string::npos))return false;
        if(key!="body" && key!="moves" && key!="voice" && key!="style" && key!="portrait" && key!="colour" && key!="damagecolour" &&
           key!="hair" && key!="head" && key!="face" && key!="hands" && !body_slot(key) && category<0 && !WarriorNativeAttacks::key(key) && !WarriorNativeEffects::key(key) && !WarriorNativeBuild::metadata_key(key) && !WarriorNativeForms::key(key))return false;
        auto value=line.substr(equals+1);
        if(!WarriorRecipe::value_ok(value) || (key.size()>=4 && key.compare(key.size()-4,4,"name")==0 && key!="name" && value.size()>31))return false;
    }
    std::string error;std::vector<WarriorNativeMaterials::Colour> colours,damaged;
    return recipe.valid(error) && recipe_colours(recipe,colours) && recipe_colours(recipe,damaged,"damagecolour");
}
inline bool valid_model(const WarriorNativePack::Pack &pack) {
    return pack.entries.size()>=252 && !pack.entries[1].empty() && !pack.entries[2].empty() &&
        !pack.entries[11].empty() && pack.entries[17].size()>=0x58;
}
inline bool part_model(const fs::path &root,const WarriorNativeSources::Model &source,int outfit,bool damage,
                       WarriorNativePack::Pack &out,std::string &error,int *resolvedIndex=nullptr) {
    if(outfit<0 || outfit>3){error="Invalid native part costume.";return false;}
    // An absent requested costume/damage slot falls back to the first intact
    // donor model, matching the installed creator's part selection.
    int index=outfit+(damage?4:0);
    if(index==0){out=source.pack;if(resolvedIndex)*resolvedIndex=0;error.clear();return true;}
    std::string raw;WarriorNativePack::Pack candidate;
    if(!WarriorNativeSources::asset(root,1423+source.id*10+index,raw,error))return false;
    if(WarriorNativePack::decode(raw,candidate,error) && valid_model(candidate))out=std::move(candidate);
    else {out=source.pack;index=0;}
    if(resolvedIndex)*resolvedIndex=index;
    error.clear();return true;
}
// Root translation of skeleton node 3, used by the reference animation scaler.
inline bool skeleton_height(const WarriorNativePack::Pack &pack,float &height) {
    if(pack.entries.size()<3)return false;
    const auto &nodes=pack.entries[2];
    if(nodes.size()<0x70)return false;
    size_t at=WarriorNativePack::word(nodes,0x6c);
    for(unsigned n=0;n<512;n++) {
        if(at>nodes.size() || nodes.size()-at<0x40)return false;
        auto size=WarriorNativePack::word(nodes,at);
        auto word=[&](size_t off){return unsigned(static_cast<unsigned char>(nodes[at+off])) |
            (unsigned(static_cast<unsigned char>(nodes[at+off+1]))<<8);};
        if(word(10)==3) {
            auto bits=WarriorNativePack::word(nodes,at+20);std::memcpy(&height,&bits,4);
            return std::isfinite(height) && height < -0.5f;
        }
        if(word(6) || !size || size>nodes.size()-at)return false;
        at+=size;
    }
    return false;
}
inline bool stock_animation_scale(WarriorNativePack::Pack &model,const WarriorNativePack::Pack &body,
                                  const WarriorNativePack::Pack &moves,std::string &error,
                                  const std::vector<WarriorNativeMotionGroups::Selection> &groups={},bool sameBodyMoves=false,
                                  const std::vector<WarriorNativeMotionGroups::RangeScale> &attackScales={}) {
    float bodyHeight,movesHeight,outfitHeight;
    if(!skeleton_height(body,bodyHeight) || !skeleton_height(moves,movesHeight) || !skeleton_height(model,outfitHeight)) {
        error="The native stock skeleton has no valid animation height.";return false;
    }
    return WarriorNativeMotionGroups::scales(model,bodyHeight,movesHeight,outfitHeight,groups,error,sameBodyMoves,attackScales);
}
inline bool generate(const fs::path &root,const Document &recipe,const fs::path &output,std::string &error,
                     const std::set<std::string> &family={},const std::string &self={},
                     std::vector<WarriorNativeMaterials::Material> *catalogueOnly=nullptr) {
    if(!supports(recipe)){error="This stock recipe needs an assembly path that is not implemented yet.";return false;}
    std::vector<std::string> formRecords(4);
    for(unsigned i=0;i<4;i++)if(!WarriorNativeForms::record(recipe.get("form"+std::to_string(i+1)),i,family,self,formRecords[i],error))return false;
    WarriorNativeSources::Model body,moves,voice,style,hands,aura;
    std::vector<std::pair<unsigned,WarriorNativeSources::Model>> headParts;
    std::vector<std::pair<unsigned,WarriorNativeSources::Model>> bodyParts;
    std::vector<WarriorNativeMotionGroups::Selection> motionGroups;
    std::vector<WarriorNativeAttacks::Selection> attacks;
    std::vector<WarriorNativeAttacks::CameraSelection> cameras;
    std::vector<WarriorNativeMaterials::Colour> colours,damageColours;
    if(!recipe_colours(recipe,colours) || !recipe_colours(recipe,damageColours,"damagecolour")){error="Invalid native material colours.";return false;}
    if(!WarriorNativeMotionGroups::prepare(root,recipe,motionGroups,error))return false;
    if(!WarriorNativeAttacks::prepare(root,recipe,attacks,error))return false;
    if(!WarriorNativeAttacks::prepare_cameras(root,recipe,cameras,error))return false;
    const auto attackScales=WarriorNativeAttacks::scale_ranges(attacks);
    auto voiceName=recipe.get("voice"),styleName=recipe.get("style");
    if(voiceName.empty())voiceName=recipe.get("body");if(styleName.empty())styleName=recipe.get("moves");
    if(!WarriorNativeSources::model(root,recipe.get("body"),body,error) ||
       !WarriorNativeSources::model(root,recipe.get("moves"),moves,error) || !WarriorNativeSources::model(root,voiceName,voice,error) ||
       !WarriorNativeSources::model(root,styleName,style,error))return false;
    const bool bt4=WarriorNativeSources::bt4_origin(body.pack);
    if(WarriorNativeSources::bt4_origin(moves.pack)!=bt4 || WarriorNativeSources::bt4_origin(voice.pack)!=bt4 ||
       WarriorNativeSources::bt4_origin(style.pack)!=bt4) {
        error="The native body, moves, voice and style must use the same source-data profile.";return false;
    }
    int portraitId=body.id;std::string portraitHud;
    if(!recipe.get("portrait").empty()) {
        WarriorNativeSources::Model portrait;
        if(!WarriorNativeSources::model(root,recipe.get("portrait"),portrait,error))return false;
        if(WarriorNativeSources::bt4_origin(portrait.pack)!=bt4){error="The portrait must use the body's source-data profile.";return false;}
        std::vector<WarriorNativeTextureBindings::Texture> textures;
        if(portrait.pack.entries.empty() || !WarriorNativeTextureBindings::textures(portrait.pack.entries[0],textures,error))return false;
        portraitHud=portrait.pack.entries[0];
        portraitId=portrait.id;
    }
    for(const auto &choice:std::vector<std::pair<const char *,unsigned>>{{"head",1},{"face",2},{"hair",4}}) {
        auto name=recipe.get(choice.first);if(name.empty())continue;
        WarriorNativeSources::Model part;
        if(!WarriorNativeSources::model(root,name,part,error))return false;
        if(WarriorNativeSources::bt4_origin(part.pack)!=bt4){error="Native head parts must use the body's data profile.";return false;}
        bool combined=false;
        for(auto &existing:headParts)if(existing.second.id==part.id){existing.first|=choice.second;combined=true;break;}
        if(!combined)headParts.emplace_back(choice.second,std::move(part));
    }
    if(hand_edit(recipe) && !WarriorNativeSources::model(root,recipe.get("hands"),hands,error))return false;
    if(!recipe.get("aurafrom").empty() && !WarriorNativeSources::model(root,recipe.get("aurafrom"),aura,error))return false;
    for(const char *key:{"chest","waist","upperarms","forearms","hips","shins","feet"})if(!recipe.get(key).empty()) {
        WarriorNativeSources::Model bodyPart;
        if(!WarriorNativeSources::model(root,recipe.get(key),bodyPart,error))return false;
        bodyParts.emplace_back(body_slot(key),std::move(bodyPart));
    }
    std::error_code ec;
    if(!catalogueOnly && (fs::exists(output,ec) || ec || !fs::create_directories(output/"pzs3us1",ec) || ec)) {
        error="Could not prepare a new native warrior folder.";return false;
    }
    std::vector<std::string> intact,damaged;std::vector<WarriorNativeMaterials::Material> materials;
    bool gap=false;
    for(int outfit=0;outfit<4;outfit++) {
        std::string raw;WarriorNativePack::Pack model;
        if(!WarriorNativeSources::asset(root,1423+body.id*10+outfit,raw,error))return false;
        if(!WarriorNativePack::decode(raw,model,error) || !valid_model(model)) {gap=true;continue;}
        if(gap){error="Non-contiguous native stock costumes are not supported yet.";return false;}
        auto original=model;WarriorNativePack::Pack normalModel;
        std::vector<WarriorNativeMaterials::Material> normalMaterials;
        for(int damage=0;damage<2;damage++) {
            if(damage) {
                if(!WarriorNativeSources::asset(root,1427+body.id*10+outfit,raw,error))return false;
                if(!WarriorNativePack::decode(raw,model,error) || !valid_model(model))model=original;
            }
            if(!WarriorNativeMoves::common(model,moves.pack,error))return false;
            for(const auto &attack:attacks)if(!WarriorNativeAttacks::parameters(model,attack.source.pack,attack.slot,attack.sourceSlot,error) ||
                !WarriorNativeAttacks::names(model,attack.source.pack,attack.slot,attack.sourceSlot,error))return false;
            for(const auto &camera:cameras)if(!WarriorNativeAttacks::camera(model,camera,error))return false;
            if(!stock_animation_scale(model,body.pack,moves.pack,error,motionGroups,body.id==moves.id,attackScales))return false;
            if(!recipe.get("aurafrom").empty() && !WarriorNativeAura::apply(model,aura.pack,error))return false;
            for(const auto &source:headParts) {
                WarriorNativePack::Pack assembled,part;
                if(!part_model(root,source.second,outfit,damage!=0,part,error) ||
                   !WarriorNativeHead::compose(model,part,assembled,error,source.first,true))return false;
                model=std::move(assembled);
            }
            // Fit all independent surfaces before stitching their shared
            // boundaries. Stitching a face against an intermediate head can
            // select different neighboring vertices than the final assembly.
            if(!headParts.empty()) {
                unsigned headId=body.id,faceId=body.id;
                for(const auto &source:headParts) {
                    if(source.first&1u)headId=source.second.id;
                    if(source.first&2u)faceId=source.second.id;
                }
                std::vector<WarriorNativeParts::Record> rows;
                if(!WarriorNativeParts::records(model.entries[2],rows,error))return false;
                auto identity=[&](unsigned node){return node==46 || node==47?headId:node==48?faceId:body.id;};
                for(const auto &row:rows)if(row.node>=46 && row.node<=48 && row.parent>=0 &&
                    identity(row.node)!=identity(rows[row.parent].node))
                    if(!WarriorNativeSeams::stitch(model.entries[2],row,rows[row.parent],nullptr,error))return false;
            }
            if(hand_edit(recipe) && bodyParts.empty()) {
                WarriorNativePack::Pack assembled,part;
                if(!part_model(root,hands,outfit,damage!=0,part,error) || !WarriorNativeHands::compose(model,part,assembled,error))return false;
                model=std::move(assembled);
            }
            if(!bodyParts.empty()) {
                std::vector<WarriorNativePack::Pack> parts;parts.reserve(bodyParts.size()+1);
                std::vector<WarriorNativeBodyModel::Selection> selections;
                for(const auto &source:bodyParts) {
                    parts.emplace_back();int resolved;
                    if(!part_model(root,source.second,outfit,damage!=0,parts.back(),error,&resolved))return false;
                    selections.push_back({&parts.back(),{source.first},uint64_t(1+source.second.id*8+resolved)});
                }
                if(hand_edit(recipe)) {
                    parts.emplace_back();int resolved;
                    if(!part_model(root,hands,outfit,damage!=0,parts.back(),error,&resolved))return false;
                    selections.push_back({&parts.back(),{7},uint64_t(1+hands.id*8+resolved)});
                }
                WarriorNativePack::Pack assembled;
                if(!WarriorNativeBodyModel::compose(model,selections,assembled,error))return false;
                model=std::move(assembled);
            }
            // The active health-bar portrait is a texture sheet in section 0,
            // separate from mesh/skin/face-animation data. Selection follows
            // only the Portrait field, for every costume and damage variant.
            if(!portraitHud.empty())model.entries[0]=portraitHud;
            if(!WarriorNativePack::copy_entries(model,voice.pack,53,252,error) || !WarriorNativePack::copy_style(model,style.pack,error))return false;
            std::vector<WarriorNativeMaterials::Material> current;
            if(!WarriorNativeMaterials::catalogue(model,current,error))return false;
            if(!damage){normalModel=model;normalMaterials=current;}
            else if(!WarriorNativeMaterials::link_damage(normalModel,model,normalMaterials,current,error))return false;
            for(auto m:current) {
                m.damaged=damage;m.costume=outfit;
                if(std::none_of(materials.begin(),materials.end(),[&](const WarriorNativeMaterials::Material &n){return n.key==m.key && n.damaged==damage;}))materials.push_back(m);
            }
            if(catalogueOnly)continue;
            auto changes=WarriorNativeMaterials::variant_colours(current,colours,damageColours,damage!=0);
            if(!WarriorNativeMaterials::recolour(model,changes,error))return false;
            std::string packed;
            if(!WarriorNativePack::encode(model,packed,error) || ((packed.size()+2047)&~size_t(2047))>WarriorNativePack::ModelLimit) {
                error="The native stock model exceeds the BT4 model buffer.";return false;
            }
            auto name="Goku_"+std::to_string(moves.id)+"_"+std::to_string(outfit+1)+"p"+(damage?"_dmg":"")+".pak";
            if(!WarriorRecipe::write(output/name,packed)){error="Could not write a native stock model.";return false;}
            (damage?damaged:intact).push_back(name);
        }
    }
    if(catalogueOnly){*catalogueOnly=std::move(materials);error.clear();return true;}
    if(intact.empty() || intact.size()!=damaged.size()){error="The stock source has no complete model family.";return false;}
    std::string built="#warrior 50\n";
    for(const auto &name:intact)built+=name+"\n";for(const auto &name:damaged)built+=name+"\n";
    const int ids[]={1431+moves.id*10,1432+moves.id*10,760+portraitId,7+portraitId*2,8+portraitId*2};
    const std::string prefix="Goku_"+std::to_string(moves.id);
    // All three portrait resources follow the selected donor, published under
    // the existing template filenames so saved and network routing stay stable.
    std::ostringstream memberA,memberB;memberA<<"pzs3us1/"<<std::setfill('0')<<std::setw(5)<<(7+moves.id*2)<<".bin";memberB<<"pzs3us1/"<<std::setfill('0')<<std::setw(5)<<(8+moves.id*2)<<".bin";
    const std::string names[]={prefix+"_anm.pak",prefix+"_eff.pak",prefix+"_img.cdbt",memberA.str(),memberB.str()};
    for(int i=0;i<5;i++) {
        std::string data;if(!WarriorNativeSources::asset(root,ids[i],data,error))return false;
        if(i<2){WarriorNativePack::Pack pack;if(!WarriorNativePack::decode(data,pack,error))return false;}
        if(i==1) {
            WarriorNativePack::Pack effects;
            if(!WarriorNativeEffects::assemble(root,recipe,effects,error) || !WarriorNativePack::encode(effects,data,error))return false;
        }
        if(i==0 && body.id!=moves.id) {
            std::string bodyData;WarriorNativePack::Pack bodyMotions,moveMotions,mixed;
            if(!WarriorNativeSources::asset(root,1431+body.id*10,bodyData,error) ||
               !WarriorNativePack::decode(bodyData,bodyMotions,error) || !WarriorNativePack::decode(data,moveMotions,error) ||
               !WarriorNativeMoves::motions(bodyMotions,moveMotions,mixed,error) ||
               !WarriorNativePack::encode(mixed,data,error))return false;
        }
        if(i==0 && !motionGroups.empty()) {
            WarriorNativePack::Pack motions;
            if(!WarriorNativePack::decode(data,motions,error) || !WarriorNativeMotionGroups::apply(motions,motionGroups,error) || !WarriorNativePack::encode(motions,data,error))return false;
        }
        if(i==0 && !attacks.empty()) {
            WarriorNativePack::Pack motions;
            if(!WarriorNativePack::decode(data,motions,error))return false;
            for(const auto &attack:attacks)if(!WarriorNativeAttacks::motions(motions,attack.motions,attack.slot,attack.sourceSlot,error))return false;
            if(!WarriorNativePack::encode(motions,data,error))return false;
        }
        if(!WarriorRecipe::write(output/fs::u8path(names[i]),data)){error="Could not write a native warrior resource.";return false;}
        built+=names[i]+"\n";
    }
    auto title=recipe.get("title");if(title.empty())title="Custom Warrior";
    std::string manifest="mode=new\ntemplate="+std::to_string(style.id)+"\nvoice="+std::to_string(voice.id)+"\nwhite="+recipe.get("name")+"\nyellow="+title+"\n";
    if(moves.pack.entries.size()==253 && moves.pack.entries[252].compare(0,8,"BT4WARRI")==0)
        manifest+="rules=bt4\n";
    manifest+="trans0=1,0,2,-1,0,-1\ntrans1=1,0,2,-1,0,-1\ntrans2=1,0,2,-1,0,-1\ntrans3=0,1,1,-1,0,-1\nnext=-1\naura=-1,-1,-1,-1\nfuse0=3,1,-1,-1,-1,-1,-1,-1\nfuse1=3,1,-1,-1,-1,-1,-1,-1\nfuse2=3,1,-1,-1,-1,-1,-1,-1\nfuseextra=-1,-1,-1,0,0,0\n";
    Document linked{manifest};
    for(unsigned i=0;i<4;i++)if(!WarriorNativeForms::replace(linked,"trans"+std::to_string(i),formRecords[i])) {
        error="Could not generate native transformation records.";return false;
    }
    manifest=std::move(linked.raw);
    if(!WarriorRecipe::write(output/"warrior.txt",recipe.raw) || !WarriorRecipe::write(output/"mod.txt",manifest) ||
       !WarriorRecipe::write(output/".warrior_built",built) || !WarriorRecipe::write(output/"native-colours.tsv",WarriorNativeMaterials::table(materials))) {
        error="Could not finish the native warrior manifests.";return false;
    }
    error.clear();return true;
}
}
