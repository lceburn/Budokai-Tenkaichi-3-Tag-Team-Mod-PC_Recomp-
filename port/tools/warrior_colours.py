"""Read material colour keys from a generated model, without modifying game data.

The key matches the reference creator's texture-content identity: FNV-1a over
the GS pixel upload payload, folded to 24 bits, with the whole-material tag.
Palettes are not part of the identity, so recolouring keeps the key stable.
"""
import argparse
from collections import Counter
import json
from pathlib import Path
import struct


def word(data, offset):
    if offset<0 or offset+4>len(data):raise ValueError("Truncated model texture data")
    return struct.unpack_from("<I",data,offset)[0]


def texture_file(model):
    count=word(model,0)
    if count<12 or count>4096 or (count+2)*4>len(model):raise ValueError("Invalid model pack")
    offsets=[word(model,i*4)&~3 for i in range(1,count+2)]
    start=offsets[11]
    if start<(count+2)*4 or start>=len(model):raise ValueError("Missing model textures")
    end=min((o for o in offsets if o>start),default=len(model))
    if end>len(model):raise ValueError("Invalid texture boundary")
    return model[start:end]


def material_key(payload):
    h=0x811c9dc5
    for value in payload:h=((h^value)*0x1000193)&0xffffffff
    folded=(h&0xffffff)^(h>>24)
    return 0x40000010|((folded<<5)&0x1fffffe0)


def catalog(model, decode=False):
    data=texture_file(model);count=word(data,0);table=word(data,4)*4
    if not 1<=count<=128 or table<8 or table+count*64>len(data):raise ValueError("Invalid texture table")
    result=[];seen=set()
    for index in range(count):
        entry=table+index*64
        tex0=struct.unpack_from("<Q",data,entry+48)[0]
        psm=(tex0>>20)&63;width=1<<((tex0>>26)&15);height=1<<((tex0>>30)&15)
        start=word(data,entry)*4;size=word(data,entry+8)
        palette=word(data,entry+4)*4;pal_size=word(data,entry+12)
        if psm not in (19,20) or not palette or not start:continue
        if size<96 or start+size>len(data) or palette+pal_size>len(data):raise ValueError("Texture upload leaves its file")
        key=material_key(data[start+96:start+size])
        if key in seen:continue
        seen.add(key)
        item=dict(key=key,texture=index,width=width,height=height,rgb=[255,255,255])
        if decode:
            import menu_font
            rgba=menu_font.texture(data,index)
            if rgba:
                colours=Counter(tuple(rgba[i:i+3]) for i in range(0,len(rgba),4) if rgba[i+3]>4 and max(rgba[i:i+3])>20)
                if colours:item["rgb"]=list(colours.most_common(1)[0][0])
        result.append(item)
    return result


if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("model",type=Path);parser.add_argument("--decode",action="store_true")
    args=parser.parse_args();print(json.dumps(catalog(args.model.read_bytes(),args.decode),indent=2))
