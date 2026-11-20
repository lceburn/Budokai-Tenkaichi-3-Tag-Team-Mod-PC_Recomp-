#include "../src/gs/warrior_native_materials.h"
#include "../src/gs/warrior_native_build.h"
#include <cassert>
#include <iostream>
int main(int argc,char **argv) {
    using namespace WarriorNativeMaterials;
    WarriorNativePack::Pack empty;std::vector<Material> rows;std::string error;
    assert(!catalogue(empty,rows,error));empty.entries.resize(12);assert(!catalogue(empty,rows,error));
    empty.entries[11]=std::string(16,'\0');WarriorNativePack::put(empty.entries[11],0,129);
    assert(!catalogue(empty,rows,error));
    if(argc==3) {
        namespace fs=std::filesystem;auto source=fs::u8path(argv[1]),out=fs::u8path(argv[2]);
        assert(fs::is_directory(source) && fs::create_directory(out));unsigned count=0;
        for(const auto &file:fs::directory_iterator(source))if(file.path().extension()==".pak") {
            std::string data;WarriorNativePack::Pack pack;
            assert(WarriorNativeBuild::read_file(file.path(),data,true));assert(WarriorNativePack::decode(data,pack,error));
            assert(catalogue(pack,rows,error));assert(WarriorRecipe::write(out/(file.path().stem().string()+".tsv"),table(rows)));count++;
        }
        std::cout<<"PASS: native catalogues generated for "<<count<<" donor models\n";
    }else assert(argc==1);
    std::cout<<"PASS: material catalogue bounds and invalid-resource rejection\n";
}
