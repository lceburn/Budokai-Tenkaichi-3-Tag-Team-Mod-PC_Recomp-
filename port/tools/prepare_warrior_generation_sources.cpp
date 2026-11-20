#include "../src/gs/warrior_native_source_import.h"
#include <iostream>
int main(int argc,char **argv) {
    if(argc!=3){std::cerr<<"Usage: prepare_warrior_generation_sources SOURCE_DATA EXISTING_NATIVE_CACHE\n";return 2;}
    std::string error;uint64_t bytes=0;
    if(!WarriorNativeSourceImport::prepare(std::filesystem::u8path(argv[1]),std::filesystem::u8path(argv[2]),error,bytes)) {
        std::cerr<<error<<"\n";return 1;
    }
    std::cout<<"Prepared native generation data: "<<bytes<<" bytes; no game executable used\n";
}
