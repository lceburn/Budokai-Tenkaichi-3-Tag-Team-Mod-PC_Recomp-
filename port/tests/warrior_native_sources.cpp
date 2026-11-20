#include "../src/gs/warrior_native_sources.h"
#include <cassert>
#include <iomanip>
#include <iostream>
using namespace WarriorNativeSources;
int main(int argc,char **argv) {
    assert(argc==2);auto root=fs::absolute(fs::u8path(argv[1]));
    fs::create_directories(root/"warrior-sources/models");
    WarriorNativePack::Pack pack;pack.entries.resize(252);pack.entries[17]=std::string(0x58,'x');
    std::string data,error;assert(WarriorNativePack::encode(pack,data,error));
    auto path=root/"warrior-sources/models/80.pak";assert(WarriorRecipe::write(path,data));
    std::ostringstream row;row<<"80\tNappa\t"<<data.size()<<'\t'<<std::hex<<std::setfill('0')<<std::setw(16)<<fingerprint(data)<<"\r\n";
    auto index=root/"warrior-sources/index.tsv";auto header=std::string("# BT4 native warrior sources v1\r\n");
    assert(WarriorRecipe::write(index,header+row.str()));Model loaded;
    assert(available(root) && model(root,"Nappa",loaded,error) && loaded.id==80);
    assert(loaded.pack.entries[17].substr(0,0x58)==std::string(0x58,'x'));
    std::vector<CatalogueEntry> catalogueRows;
    assert(catalogue(root,catalogueRows,error) && catalogueRows.size()==1 && catalogueRows[0].id==80);
    auto finalRow=row.str();finalRow.replace(0,2,"249");
    finalRow.replace(finalRow.find("Nappa"),5,"Last BT4 fighter");
    assert(WarriorRecipe::write(root/"warrior-sources/models/249.pak",data));
    assert(WarriorRecipe::write(index,header+row.str()+finalRow));
    assert(catalogue(root,catalogueRows,error) && catalogueRows.size()==2 && catalogueRows[1].id==249);
    assert(model(root,"Last BT4 fighter",loaded,error) && loaded.id==249);
    assert(model(root,"Last-BT4 fighter",loaded,error) && loaded.id==249);
    assert(identity_key("Chichi")==identity_key("Chi-Chi"));
    assert(identity_key("Android 18")==identity_key("Android #18"));
    assert(identity_key("Goku (Med)")==identity_key("Goku (Mid)"));
    assert(identity_key("Goku (Mid)")!=identity_key("Goku (End)"));
    assert(!model(root,"Unknown",loaded,error));
    assert(WarriorRecipe::write(index,header+row.str()+row.str()));assert(!model(root,"Nappa",loaded,error));
    auto unchanged=catalogueRows.size();assert(!catalogue(root,catalogueRows,error) && catalogueRows.size()==unchanged);
    assert(WarriorRecipe::write(index,header+row.str()));data.back()='y';assert(WarriorRecipe::write(path,data));
    assert(!model(root,"Nappa",loaded,error));
    fs::create_directory(root/"warrior-sources/files");auto assetPath=root/"warrior-sources/files/01461.bin";
    assert(WarriorRecipe::write(assetPath,"ANIMATION DATA"));
    std::ostringstream assetRow;assetRow<<"# BT4 native warrior generation assets v1\n1461\t14\t"<<std::hex<<std::setfill('0')<<std::setw(16)<<fingerprint("ANIMATION DATA")<<"\n";
    assert(WarriorRecipe::write(root/"warrior-sources/assets.tsv",assetRow.str()));std::string assetData;
    assert(generation_available(root) && asset(root,1461,assetData,error) && assetData=="ANIMATION DATA");
    assert(!asset(root,1462,assetData,error));assert(!asset(root,-1,assetData,error));
    assert(WarriorRecipe::write(assetPath,"CHANGED ASSETS"));assert(!asset(root,1461,assetData,error));
    assert(WarriorRecipe::write(index,header+"../outside\tNappa\t1000\t0000000000000000\n"));
    assert(!model(root,"Nappa",loaded,error));
    std::cout<<"PASS: native source integrity, CRLF, missing donors, duplicate catalogue rows and path rejection\n";
}
