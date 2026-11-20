"""Generate local native roster chips from supplied portraits and a disc template.

Game artwork stays in the selected private data folder. This does not change
models, recipes, palettes, animations or the source archives. Existing icons
are preserved unless --replace is explicitly selected.
"""
import argparse
import json
from pathlib import Path
import struct
from PIL import Image
import menu_font
from make_fingerprint import bpe_decode


def word(data,at):return struct.unpack_from("<I",data,at)[0]


def part(data,n):
    start,end=(word(data,j*4)&~3 for j in (n,n+1))
    if start<0 or end<start or end>len(data):raise ValueError("Invalid native pack entry")
    return data[start:end]


def build_icon(portrait,template):
    """Keep the native chip's upload layout and silhouette; replace its art."""
    if len(template)!=5504:raise ValueError("Expected the native 5,504-byte chip")
    decoded=bpe_decode(portrait)
    if decoded is None:raise ValueError("Invalid supplied portrait")
    entry=word(decoded,4)*4;tex=struct.unpack_from("<Q",decoded,entry+48)[0]
    width,height=1<<((tex>>26)&15),1<<((tex>>30)&15)
    rgba=menu_font.texture(decoded,0)
    mask_rgba=menu_font.texture(template,0)
    if rgba is None or mask_rgba is None:raise ValueError("Unsupported portrait or template texture")
    source=Image.frombytes("RGBA",(width,height),rgba)
    mask=Image.frombytes("RGBA",(64,64),mask_rgba).getchannel("A")
    bounds=mask.getbbox()
    if not bounds:raise ValueError("The native icon has no visible pixels")
    # The supplied full-length portraits place the head in their upper centre.
    crop=source.crop((width//4,height*70//512,width*3//4,height*214//512))
    crop=crop.resize((bounds[2]-bounds[0],bounds[3]-bounds[1]),Image.Resampling.LANCZOS)
    image=Image.new("RGBA",(64,64));image.paste(crop,bounds[:2]);image.putalpha(mask)
    indexed=image.quantize(colors=256,method=Image.Quantize.FASTOCTREE)
    palette=bytes(indexed.getpalette("RGBA"));pixels=indexed.tobytes()
    result=bytearray(template);entry=word(result,4)*4
    tex=struct.unpack_from("<Q",result,entry+48)[0]
    pixel_at=word(result,entry)*4+96;palette_at=word(result,entry+4)*4+96
    reg=struct.unpack_from("<Q",result,pixel_at-48)[0]
    key=(64,64,(tex>>20)&63,reg&4095,(reg>>32)&4095,word(result,entry+24)&63,(tex>>14)&63)
    mapping=menu_font._MAPPING[key]
    if len(mapping)!=4096 or len({a for a,shift in mapping})!=4096 or any(shift for a,shift in mapping):
        raise ValueError("Unsupported native upload layout")
    for index,(address,shift) in enumerate(mapping):result[pixel_at+address]=pixels[index]
    for index in range(256):
        native=(index&0xE7)|((index&8)<<1)|((index&16)>>1)
        result[palette_at+native*4:palette_at+native*4+4]=palette[index*4:index*4+4]
    # Independently decode the finished packet, rather than trusting the writes.
    if menu_font.texture(result,0)!=indexed.convert("RGBA").tobytes():
        raise ValueError("Generated chip failed its native-format round trip")
    return bytes(result)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--data",type=Path,required=True)
    p.add_argument("--template-character",type=int,default=52)
    p.add_argument("--replace",action="store_true")
    a=p.parse_args()
    if json.loads((a.data/"native-disc.json").read_text()).get("game")!="bt4":
        p.error("This prototype icon builder expects the verified BT4 data layout")
    archive=(a.data/"pzs4us1/00448.bin").read_bytes()
    menu=bpe_decode(part(archive,2))
    if menu is None:raise ValueError("Invalid native team picker")
    template=part(part(menu,45),a.template_character+1)
    enabled=a.data/"mods/enabled.txt"
    for name in enabled.read_text(encoding="utf-8-sig").splitlines():
        if not name or name.startswith("#"):continue
        folder=a.data/"mods"/name
        if folder.resolve().parent!=(a.data/"mods").resolve():raise ValueError("Unsafe warrior folder")
        target=folder/"chip.bin"
        if target.exists() and not a.replace:
            print("Kept existing icon:",name);continue
        files=(folder/".warrior_built").read_text(encoding="utf-8-sig").splitlines()
        portraits=[f for f in files if f.endswith("_img.cdbt")]
        if len(portraits)!=1:raise ValueError("Expected one supplied portrait")
        portrait=folder/portraits[0]
        if portrait.resolve().parent!=folder.resolve():raise ValueError("Unsafe portrait path")
        result=build_icon(portrait.read_bytes(),template)
        temp=folder/".chip-building.tmp";temp.write_bytes(result);temp.replace(target)
        print("Built native roster icon:",name)


if __name__=="__main__":main()
