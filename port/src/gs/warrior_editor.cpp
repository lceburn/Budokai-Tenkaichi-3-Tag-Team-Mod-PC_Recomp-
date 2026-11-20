// First native editor pass: identity and voice edits need no model rebuilding.
#include "imgui.h"
#include "warrior_recipe.h"
#include <cstdio>
#include <cstdlib>

namespace {
struct Editor {
    bool loaded=false,ready=false;
    int selected=-1;
    std::vector<std::string> folders,voices;
    WarriorRecipe::fs::path root,path;
    WarriorRecipe::Document original;
    char name[192]={},voice[192]={},filter[192]={};
    std::string status;
    void load() {
        ready=false;status.clear();
        if(selected<0 || selected>=int(folders.size()) || !WarriorRecipe::recipe_path(root,folders[selected],path) ||
           !WarriorRecipe::read(path,original.raw) || !original.valid(status)) {
            if(status.empty())status="Could not open this warrior's recipe.";return;
        }
        std::snprintf(name,sizeof(name),"%s",original.get("name").c_str());
        auto v=original.get("voice");if(v.empty())v=original.get("body");
        std::snprintf(voice,sizeof(voice),"%s",v.c_str());ready=true;
    }
    void scan() {
        loaded=true;folders.clear();voices.clear();selected=-1;ready=false;
        const char *data=std::getenv("BT3_DATA");root=WarriorRecipe::fs::u8path(data?data:"gamedata");
        std::string list;
        if(!WarriorRecipe::read(root/"mods/enabled.txt",list)){status="No imported warriors are enabled.";return;}
        for(size_t at=0;at<list.size() && folders.size()<32;) {
            size_t end=list.find('\n',at);if(end==std::string::npos)end=list.size();
            auto folder=list.substr(at,end-at);if(!folder.empty() && folder.back()=='\r')folder.pop_back();
            WarriorRecipe::fs::path recipe;
            if(!folder.empty() && folder[0]!='#' && WarriorRecipe::recipe_path(root,folder,recipe))folders.push_back(folder);
            at=end+1;
        }
        std::string roster;
        if(WarriorRecipe::read(root/"native-roster.tsv",roster)) {
            for(size_t at=0;at<roster.size();) {
                size_t end=roster.find('\n',at);if(end==std::string::npos)end=roster.size();
                auto line=roster.substr(at,end-at);if(!line.empty() && line.back()=='\r')line.pop_back();
                size_t first=line.find('\t'),second=first==std::string::npos?first:line.find('\t',first+1);
                if(first!=std::string::npos && second!=std::string::npos && !line.empty() && line[0]>='0' && line[0]<='9') {
                    auto v=line.substr(first+1,second-first-1),form=line.substr(second+1);
                    if(!form.empty())v+=" "+form;
                    if(WarriorRecipe::value_ok(v))voices.push_back(v);
                }
                at=end+1;
            }
        }
        if(!folders.empty()){selected=0;load();}else status="No editable imported recipes found.";
    }
} editor;
}
void WarriorEditor_Tab() {
    auto &e=editor;if(!e.loaded)e.scan();
    ImGui::TextWrapped("Warrior editor: name and voice. Save, then restart the game to apply changes.");
    bool dirty=e.ready && (e.original.get("name")!=e.name ||
        (e.original.get("voice").empty()?e.original.get("body"):e.original.get("voice"))!=e.voice);
    ImGui::BeginDisabled(dirty);
    if(ImGui::Button("Refresh warriors"))e.scan();
    if(ImGui::BeginCombo("Warrior",e.selected>=0?e.folders[e.selected].c_str():"Choose a warrior")) {
        for(int i=0;i<int(e.folders.size());i++)if(ImGui::Selectable(e.folders[i].c_str(),i==e.selected)){e.selected=i;e.load();}
        ImGui::EndCombo();
    }
    ImGui::EndDisabled();
    if(e.ready) {
        ImGui::InputText("Name",e.name,sizeof(e.name));
        if(ImGui::BeginCombo("Voice",e.voice)) {
            ImGui::InputText("Find voice",e.filter,sizeof(e.filter));
            for(const auto &v:e.voices)if(!e.filter[0] || v.find(e.filter)!=std::string::npos)
                if(ImGui::Selectable(v.c_str(),v==e.voice))std::snprintf(e.voice,sizeof(e.voice),"%s",v.c_str());
            ImGui::EndCombo();
        }
        bool known=false;for(const auto &v:e.voices)if(v==e.voice)known=true;
        ImGui::BeginDisabled(!dirty || !known || !WarriorRecipe::value_ok(e.name));
        if(ImGui::Button("Save changes")) {
            auto edited=e.original;
            if(edited.set("name",e.name) && edited.set("voice",e.voice) && WarriorRecipe::save(e.path,e.original,edited,e.status)) {
                e.original=edited;e.status="Saved. Restart the game to use the edited warrior. A recipe backup was kept.";
            }else if(e.status.empty())e.status="Could not save these values.";
        }
        ImGui::EndDisabled();ImGui::SameLine();
        if(ImGui::Button("Discard and reload"))e.load();
        ImGui::Separator();
        ImGui::Text("Body: %s",e.original.get("body").c_str());
        ImGui::Text("Moves: %s",e.original.get("moves").c_str());
        ImGui::TextWrapped("Body, parts, moves and transformation editing are not available in this first editor pass.");
    }
    if(!e.status.empty())ImGui::TextWrapped("%s",e.status.c_str());
}
