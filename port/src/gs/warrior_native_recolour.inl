// Included inside WarriorNativeMaterials after the palette/HSV helpers.
// Every transformation reads the donor snapshot, never a previously painted
// shared palette. Meshes receive private shading bindings when edits differ.
inline bool recolour(WarriorNativePack::Pack &model,const std::vector<Colour> &colours,std::string &error) {
    if(colours.empty())return true;
    std::vector<Material> materials;
    if(!catalogue(model,materials,error) || model.entries[2].size()<0x54){error="Missing material bindings.";return false;}
    const auto &original=model.entries[11];auto data=original;
    std::vector<uint32_t> keys;if(!material_keys(original,keys,error))return false;
    std::vector<WarriorNativeTextureBindings::Texture> textures;
    size_t table=size_t(WarriorNativePack::word(original,4))*4;
    unsigned first=(unsigned char)model.entries[2][0x4e],normal=(unsigned char)model.entries[2][0x4f];
    unsigned shadeFirst=(unsigned char)model.entries[2][0x50],shades=(unsigned char)model.entries[2][0x51];
    if(first>keys.size() || normal>keys.size()-first || shadeFirst>keys.size() || shades>keys.size()-shadeFirst || shades>16){error="Invalid material binding counts.";return false;}
    std::map<uint32_t,Colour> requested;
    for(const auto &c:colours) {
        if((c.key&0xe000001f)!=0x40000010 || !requested.emplace(c.key,c).second || c.r<0 || c.r>255 || c.g<0 || c.g>255 || c.b<0 || c.b>255){error="Invalid native material colour.";return false;}
    }
    std::map<unsigned,Material> bases;std::map<unsigned,Colour> changes;
    std::map<size_t,std::string> palettesPainted;
    for(unsigned i=first;i<first+normal;i++)if(keys[i]) {
        auto c=requested.find(keys[i]);if(c==requested.end())continue;
        size_t e=table+i*64;Material m{keys[i],int(i)};
        if(!dominant(original,e,m)){error="Could not read the native material's base colour.";return false;}
        bases[i]=m;
        // An explicit assignment of the original color is a byte-exact no-op.
        if(m.r==c->second.r && m.g==c->second.g && m.b==c->second.b)continue;
        changes[i]=c->second;
        size_t p=size_t(WarriorNativePack::word(original,e+4))*4;
        unsigned bytes=WarriorNativePack::word(original,e+12),psm=(wide(original,e+48)>>20)&63,n=psm==19?256:16;
        if(bytes<96+n*4 || p>original.size() || bytes>original.size()-p){error="The material palette is truncated.";return false;}
        auto painted=original.substr(p+96,n*4);auto from=hsv(m.r,m.g,m.b),to=hsv(c->second.r,c->second.g,c->second.b);
        for(unsigned k=0;k<n;k++)turn(from,to,painted.data()+k*4);
        auto existing=palettesPainted.find(p);
        if(existing!=palettesPainted.end() && existing->second!=painted){error="Conflicting edits to a shared native palette.";return false;}
        palettesPainted[p]=painted;data.replace(p+96,painted.size(),painted);
    }
    if(changes.empty()){error.clear();return true;}
    if(!WarriorNativeTextureBindings::textures(original,textures,error))return false;
    struct Binding {unsigned entry,material,shade;size_t offset;};std::vector<Binding> bindings;
    // The facial-animation streams use the same material blocks as the mesh.
    for(unsigned entry=2;entry<=10 && entry<model.entries.size();entry++) {
        const auto &mesh=model.entries[entry];
        for(size_t at=0;at+88<=mesh.size();at+=4)if(WarriorNativePack::word(mesh,at)==0x6c058000) {
            size_t vertices=WarriorNativePack::word(mesh,at+52)&0x7fff;if(vertices>(mesh.size()-at-88)/48)continue;
            unsigned tbp=WarriorNativePack::word(mesh,at+20)&0x3fff,cbp=(WarriorNativePack::word(mesh,at+24)>>5)&0x3fff;
            unsigned shadow=(WarriorNativePack::word(mesh,at+40)>>5)&0x3fff;
            for(unsigned i=first;i<first+normal;i++)if(textures[i].tbp==tbp && textures[i].cbp==cbp) {
                for(unsigned j=shadeFirst;j<shadeFirst+shades;j++)if(textures[j].tbp==shadow)bindings.push_back({entry,i,j,at+40});
                break;
            }
        }
    }
    auto next=model;
    struct Shade {std::string upload;unsigned tbp;};
    std::map<unsigned,std::vector<Shade>> groups;
    std::map<std::pair<unsigned,unsigned>,std::string> transformed;
    for(const auto &binding:bindings) {
        auto pair=std::make_pair(binding.shade,binding.material);if(transformed.count(pair))continue;
        auto upload=textures[binding.shade].pixelUpload;
        auto change=changes.find(binding.material);
        if(change!=changes.end()) {
            if(upload.size()<1120){error="A shading upload leaves the material resource.";return false;}
            const auto &m=bases.at(binding.material);const auto &c=change->second;
            auto from=hsv(m.r,m.g,m.b),to=hsv(c.r,c.g,c.b);
            for(unsigned n=0;n<256;n++) {
                char *pixel=upload.data()+96+n*4;if(!(pixel[0] || pixel[1] || pixel[2]))continue;
                char lit[3];int old[]={m.r,m.g,m.b},target[]={c.r,c.g,c.b};
                for(unsigned k=0;k<3;k++)lit[k]=char(std::max(0,old[k]-int((unsigned char)pixel[k])));
                turn(from,to,lit);
                for(unsigned k=0;k<3;k++)pixel[k]=char(std::max(0,target[k]-int((unsigned char)lit[k])));
            }
        }
        transformed[pair]=std::move(upload);
    }
    unsigned cursor=0;
    for(unsigned j=shadeFirst;j<shadeFirst+shades;j++)cursor=std::max(cursor,textures[j].tbp+WarriorNativePack::word(textures[j].header,16));
    std::vector<WarriorNativeTextureBindings::Texture> extras;
    for(unsigned j=shadeFirst;j<shadeFirst+shades;j++) {
        auto &list=groups[j];const auto originalUpload=textures[j].pixelUpload;
        bool preserve=false;
        for(const auto &pair:transformed)if(pair.first.first==j && pair.second==originalUpload)preserve=true;
        // Fade resources keep their original colors even if no visible surface does.
        unsigned fade=(unsigned char)model.entries[2][0x4d],fadeFirst=(unsigned char)model.entries[2][0x52],fadeCount=(unsigned char)model.entries[2][0x53];
        if(j==fade || (j>=fadeFirst && j<fadeFirst+fadeCount))preserve=true;
        if(preserve)list.push_back({originalUpload,textures[j].tbp});
        for(const auto &binding:bindings)if(binding.shade==j) {
            const auto &upload=transformed.at({j,binding.material});
            auto found=std::find_if(list.begin(),list.end(),[&](const Shade &s){return s.upload==upload;});
            if(found==list.end()) {
                if(list.empty()) {
                    list.push_back({upload,textures[j].tbp});textures[j].pixelUpload=upload;
                }else {
                    auto clone=textures[j];unsigned extent=WarriorNativePack::word(clone.header,16);
                    if(!extent || cursor>0x3fff || extent>0x4000-cursor){error="Native recolored shading exceeds GS address limits.";return false;}
                    clone.tbp=cursor;clone.pixelUpload=upload;extras.push_back(std::move(clone));list.push_back({upload,cursor});cursor+=extent;
                }
                found=std::prev(list.end());
            }
            auto &mesh=next.entries[binding.entry];auto word=WarriorNativePack::word(mesh,binding.offset);
            WarriorNativePack::put(mesh,binding.offset,(word&~(0x3fffu<<5))|(found->tbp<<5));
        }
    }
    if(shades+extras.size()>16 || textures.size()+extras.size()>128){error="These independent surface colors exceed the native shading capacity.";return false;}
    unsigned insert=shadeFirst+shades;
    if(extras.empty()) {
        for(unsigned j=shadeFirst;j<shadeFirst+shades;j++) {
            size_t e=table+j*64,p=size_t(WarriorNativePack::word(original,e))*4;
            data.replace(p,textures[j].pixelUpload.size(),textures[j].pixelUpload);
        }
    }else {
        // Indexed palettes were painted in the original layout; retain those
        // payloads when inserting the new shading rows before auxiliary uploads.
        for(unsigned i=0;i<textures.size();i++)if(!textures[i].paletteUpload.empty()) {
            size_t p=size_t(WarriorNativePack::word(original,table+i*64+4))*4;
            textures[i].paletteUpload=data.substr(p,textures[i].paletteUpload.size());
        }
        textures.insert(textures.begin()+insert,extras.begin(),extras.end());
        data.assign(32+textures.size()*64,'\0');data.replace(0,32,original.substr(0,32));
        WarriorNativePack::put(data,0,unsigned(textures.size()));WarriorNativePack::put(data,4,8);
        for(unsigned i=0;i<textures.size();i++) {
            const auto &t=textures[i];size_t e=32+i*64;data.replace(e,64,t.header);
            WarriorNativePack::put(data,e,unsigned(data.size()/4));data+=t.pixelUpload;
            WarriorNativePack::put(data,e+4,t.paletteUpload.empty()?0:unsigned(data.size()/4));data+=t.paletteUpload;
            WarriorNativePack::put(data,e+32,t.tbp);WarriorNativePack::put(data,e+36,t.cbp);
            if(data.size()>WarriorNativePack::Limit){error="The recolored texture resource exceeds its limit.";return false;}
        }
        auto &mesh=next.entries[2];
        for(unsigned field:{0x4cu,0x4du,0x52u})if((unsigned char)mesh[field]>=insert)mesh[field]=char((unsigned char)mesh[field]+extras.size());
        mesh[0x51]=char(shades+extras.size());
    }
    for(size_t i=12;i<17 && i<next.entries.size();i++)follow_colours(next.entries[i],original,data);
    next.entries[11]=std::move(data);model=std::move(next);error.clear();return true;
}
