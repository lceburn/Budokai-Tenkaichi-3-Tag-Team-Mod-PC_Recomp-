#include "../src/gs/warrior_recipe.h"
#include <cassert>
#include <iostream>
int main(int argc,char **argv) {
    using namespace WarriorRecipe;
    assert(argc==2);fs::path root=fs::u8path(argv[1]);
    std::error_code ec;fs::create_directories(root/"mods/Example",ec);assert(!ec);
    fs::path path=root/"mods/Example/warrior.txt",resolved;
    Document original{"# keep this\r\nname=Example\r\nbody=Future Gohan\r\ncolour=1,2,3,4\r\ncolour=5,6,7,8\r\nform1=1,0,Other\r\n"};
    std::string error,bytes;
    assert(original.valid(error));assert(write(path,original.raw));
    assert(recipe_path(root,"Example",resolved));
    assert(!recipe_path(root,"../Example",resolved));
    assert(!recipe_path(root,"Example/..",resolved));
    auto edited=original;
    assert(edited.set("name","Updated"));assert(edited.set("voice","Nappa"));
    assert(!edited.set("body","Goku (End)"));assert(!edited.set("name","bad\nvoice=x"));
    assert(edited.get("body")=="Future Gohan");
    assert(edited.raw.find("colour=1,2,3,4\r\ncolour=5,6,7,8\r\nform1=1,0,Other\r\n")!=std::string::npos);
    assert(save(path,original,edited,error));assert(read(path,bytes) && bytes==edited.raw);
    assert(read(path.string()+".editor-backup",bytes) && bytes==original.raw);
    auto conflict=edited;assert(conflict.set("name","External"));assert(write(path,conflict.raw));
    assert(!save(path,edited,original,error));assert(read(path,bytes) && bytes==conflict.raw);
    Document dup{"name=one\nname=two\n"};assert(!dup.valid(error));
    Document huge{std::string(Limit+1,'x')};assert(!huge.valid(error));
    Document missing{"body=Goku (End)\n"};assert(!missing.valid(error));
    Document eof{"name=One"};assert(eof.set("voice","Nappa"));assert(eof.get("voice")=="Nappa");
    fs::create_directories(root/"outside",ec);assert(!ec);
    fs::create_directory_symlink(root/"outside",root/"mods/Escape",ec);
    if(!ec){assert(write(root/"outside/warrior.txt",original.raw));assert(!recipe_path(root,"Escape",resolved));}
    std::cout<<"PASS: recipe edits preserve unknown fields, backups and conflicts\n";
}
