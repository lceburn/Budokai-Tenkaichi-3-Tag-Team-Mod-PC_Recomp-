#include "../src/gs/warrior_native_parts.h"
#include <cassert>
#include <iostream>
int main() {
    using namespace WarriorNativeParts;
    std::vector<Record> rows;
    auto add=[&](unsigned node,int parent) {
        rows.push_back({0,64,0,0,node,0,parent});return int(rows.size()-1);
    };
    auto root=add(3,-1),waist=add(16,root),hips=add(8,waist);
    auto cloth=add(71,waist),child=add(72,cloth),loose=add(87,root);
    auto skirt=add(88,hips),head=add(46,root),hair=add(79,head);
    auto special=add(111,root),specialChild=add(112,special);
    assert(attachment_slot(rows,cloth)==4 && attachment_slot(rows,child)==4);
    assert(attachment_slot(rows,loose)==4 && attachment_slot(rows,skirt)==8);
    assert(attachment_slot(rows,hair)==-1 && attachment_slot(rows,special)==-1);
    assert(attachment_slot(rows,specialChild)==-1 && attachment_slot(rows,waist)==-1);
    assert(attachment_slot(rows,rows.size())==-1);
    rows[cloth].parent=child;assert(attachment_slot(rows,child)==-1);
    std::cout<<"PASS native attachment ownership: waist, root cloth, hips, nested chains, hair exclusions and invalid ancestry\n";
}
