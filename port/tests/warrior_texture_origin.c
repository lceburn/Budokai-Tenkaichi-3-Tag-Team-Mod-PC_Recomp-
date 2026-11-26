/* Exercise the production model binder and VIF rebaser, without game assets. */
#include "../../src/sys/gfxm_e_b.c"
#include "../../src/sys/gfxm_e_c.c"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
void Res_RelocateOffsets(TexFile **out,u8 *base,TexFile *hdr) { (void)base;*out=hdr; }
s32 BtlObjVis_AllocFreeBit(void) { return 0; }
s32 BtlObjVis_AllocBit(s32 bit) { return bit; }
static void check(u32 offset,u32 palette) {
    static unsigned char raw[512],clut[2][160];
    ObjMdlFile *model=(ObjMdlFile *)raw;
    ObjMdlMesh *mesh=(ObjMdlMesh *)(raw+128);
    TexEntry entries[2]={0};TexFile tex={0};ObjMdlObj obj={0};
    u32 *chain=mesh->vif;
    memset(raw,0,sizeof(raw));
    model->meshOfs=128;model->texCount=2;mesh->enabled=1;mesh->last=1;
    chain[0]=0x6C058000;chain[5]=offset;chain[6]=palette<<5;chain[25]=0x70000000;
    entries[0].clut=clut[0];entries[1].clut=clut[1];
    entries[1].tbpOfs=offset;entries[1].cbpOfs=palette;
    tex.count=2;tex.ent=entries;obj.mdl.file=model;obj.mdl.texFile=&tex;
    BtlObjMdl_BindFiles(&obj);
    assert((chain[5]&0x3FFF)==0x3480+offset);
    assert(((chain[6]>>5)&0x3FFF)==0x3C00+palette);
    assert(model->bound);
    BtlObjMdl_BindFiles(&obj); // Existing bindings must not be rebased twice.
    assert((chain[5]&0x3FFF)==0x3480+offset);
}
int main(void) {
    check(128,1); /* Zangya with Goku head: first uploaded texture is not drawn. */
    check(64,1);  /* Broly / Caulifla with Goku head. */
    check(0,0);   /* Ordinary stock model. */
    puts("PASS production texture binding: uploads and materials share origin, stock and repeat bind");
    return 0;
}
