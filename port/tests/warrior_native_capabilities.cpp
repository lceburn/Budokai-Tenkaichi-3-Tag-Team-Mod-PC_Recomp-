#include "../src/gs/warrior_native_stock.h"
#include "../../include/menu/warrior_fields.h"
#include <cassert>
#include <iostream>
int main() {
    setenv("BT3_WARRIOR_NATIVE_HEAD_TEST","1",1);
    setenv("BT3_WARRIOR_NATIVE_HAND_TEST","1",1);
    setenv("BT3_WARRIOR_NATIVE_BODY_TEST","1",1);
    unsigned checked=0;
    for(int field=0;field<ZW_FIELD_COUNT;field++) {
        if(!zw_native_editable(field))continue;
        WarriorRecipe::Document recipe{"name=Test\nbody=Goku (End)\nmoves=Goku (End)\n"};
        auto key=std::string(zwFields[field].key);
        std::string value="Vegeta (Scouter)";
        if(zwFields[field].kind==ZW_RGB)value="00ff00";
        if(zwFields[field].kind==ZW_ATTACK)value+=":1";
        if(zwFields[field].kind==ZW_FORM)value="1,0,Other";
        if(field==ZW_HEADSET)recipe.raw+="head="+value+"\nhair="+value+"\nface="+value+"\n";
        else if(field==ZW_NAME)recipe.raw="name=New name\nbody=Goku (End)\nmoves=Goku (End)\n";
        else if(field!=ZW_BODY && field!=ZW_MOVES)recipe.raw+=key+"="+value+"\n";
        if(!WarriorNativeStock::supports(recipe)){std::cerr<<"Exposed unsupported control: "<<key<<"\n";return 1;}
        checked++;
    }
    assert(!zw_native_editable(ZW_ANIM_MELEE));
    assert(!zw_native_editable(ZW_EXTRA1));
    assert(!zw_native_editable(ZW_BODYSET));
    assert(!zw_native_editable(ZW_LOOK2));
    assert(!zw_native_editable(ZW_BLAST11COLOUR));
    std::cout<<"PASS: "<<checked<<" exposed native controls pass the recipe capability gate\n";
}
