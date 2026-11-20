"""Prepare private native-builder donor assets without running a game executable.

This development importer reads already-extracted BT3 data. Its generated cache
contains game assets and must stay out of source control and public releases.
"""
import argparse
from pathlib import Path
import re
import struct

ROOT=Path(__file__).resolve().parents[2]
LIMIT=64*1024*1024

def donor_names():
    source=(ROOT/'src/menu/port_tbo.c').read_text(encoding='utf-8')
    table=source.split('sTbNames[] = {',1)[1].split('};',1)[0]
    names=re.findall(r'"([^"]+)"',table)
    if len(names)!=161 or len(set(names))!=161:raise ValueError('Unknown native donor catalogue')
    return names

def fingerprint(data):
    value=0xcbf29ce484222325
    for byte in data:value=((value^byte)*0x100000001b3)&0xffffffffffffffff
    return value

def validate_model(data):
    if len(data)<12 or len(data)>LIMIT:raise ValueError('Invalid donor model size')
    count=struct.unpack_from('<I',data)[0]
    if count<252 or count>4096 or (count+2)*4>len(data):raise ValueError('Donor model lacks required metadata')
    offsets=[n&~3 for n in struct.unpack_from(f'<{count+1}I',data,4)]
    if offsets[0]<(count+2)*4 or offsets[-1]>len(data) or offsets!=sorted(offsets):raise ValueError('Invalid donor model offsets')
    if offsets[18]-offsets[17]<0x58:raise ValueError('Donor model lacks fighter stats')

def prepare(source,output):
    if not (source/'pzs3us1').is_dir():raise ValueError('Choose extracted BT3 source data')
    if output.exists() and any(output.iterdir()):raise ValueError('Choose an empty native source-cache folder')
    models=output/'models';models.mkdir(parents=True)
    rows=['# BT4 native warrior sources v1\n']
    total=0
    for index,name in enumerate(donor_names()):
        path=source/'pzs3us1'/f'{1424+index*10-1:05d}.bin'
        if not path.is_file() or path.stat().st_size>LIMIT:raise ValueError(f'Missing or oversized donor model: {name}')
        data=path.read_bytes();validate_model(data)
        (models/f'{index}.pak').write_bytes(data)
        rows.append(f'{index}\t{name}\t{len(data)}\t{fingerprint(data):016x}\n');total+=len(data)
    staged=output/'index.ready';staged.write_text(''.join(rows),encoding='utf-8');staged.replace(output/'index.tsv')
    return dict(donors=len(rows)-1,bytes=total,executables_used=False)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-data',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    try:print(prepare(args.source_data.resolve(),args.output.resolve()))
    except (OSError,ValueError) as error:parser.exit(1,str(error)+'\n')

if __name__=='__main__':main()
