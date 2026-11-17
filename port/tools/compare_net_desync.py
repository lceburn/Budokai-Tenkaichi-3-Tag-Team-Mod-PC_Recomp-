"""Compare private native network captures without uploading them anywhere."""
import argparse
import json
from pathlib import Path
import struct

FIELDS=('chara costume hp maxHp ki blast action target player lockedOff clash fused controller next seconds swap revive helper assist power posX posY posZ posW rotX rotY rotZ rotW').split()


def pairs(line):
    return dict(token.split('=',1) for token in line.split() if '=' in token)


def read(path):
    lines=Path(path).read_text(encoding='utf-8').splitlines()
    if not lines or not lines[0].startswith('net-desync-v1 '):raise ValueError('Not a network diagnostic capture: '+str(path))
    result={'meta':pairs(lines[0]),'frames':{},'config':[]}
    frame=None
    for line in lines[1:]:
        if line.startswith(('config ','roster=','options')):result['config'].append(line)
        elif line.startswith('frame='):
            if 'unavailable' in line:frame=None;continue
            frame={'meta':pairs(line),'actors':{},'inputs':{}}
            result['frames'][int(frame['meta']['frame'])]=frame
        elif frame is not None:
            if line.startswith('actor='):
                parts=line.split();frame['actors'][int(parts[0].split('=')[1])]=[int(w,16) for w in parts[2:]]
            elif line.startswith('input='):frame['inputs'][int(pairs(line)['input'])]=pairs(line)
            else:
                parts=line.split()
                if parts and parts[0] in ('header','rng','lobby','mt','extra_rng'):frame[parts[0]]=[int(w,16) for w in parts[1:]]
    return result


def recompute(frame):
    if len(frame.get('header',[]))!=4 or len(frame.get('rng',[]))!=2 or len(frame.get('lobby',[]))!=2:return None
    words=list(frame['header'])
    for actor in range(frame['header'][0]):
        if len(frame['actors'].get(actor,[]))!=28:return None
        words+=frame['actors'][actor]
    words+=frame['rng'];h=14695981039346656037
    for word in words:
        for b in struct.pack('<I',word):h=((h^b)*1099511628211)&((1<<64)-1)
    return h^(frame['lobby'][0]|frame['lobby'][1]<<32)


def compare(a,b):
    meta=('build','data','round','mismatch_frame')
    result={'metadata_match':all(a['meta'].get(k)==b['meta'].get(k) for k in meta),
            'configuration_match':a['config']==b['config'],
            'paired_checksums':a['meta'].get('local')==b['meta'].get('remote') and b['meta'].get('local')==a['meta'].get('remote'),
            'capture_hash_errors':[], 'first_state_difference':None,'first_extra_rng_difference':None,'first_input_difference':None}
    if not result['metadata_match']:
        result.update(common_frames=0,note='Captures differ in build, data, round or mismatch frame. Pair reports from the same event before comparing states.')
        return result
    for name,report in (('first',a),('second',b)):
        for tick,frame in report['frames'].items():
            h=recompute(frame)
            if h is not None and h!=int(frame['meta']['hash'],16):result['capture_hash_errors'].append({'report':name,'frame':tick})
    common=sorted(a['frames'].keys()&b['frames'].keys());result['common_frames']=len(common)
    for tick in common:
        x,y=a['frames'][tick],b['frames'][tick]
        differences=[]
        for key in ('header','rng','lobby'):
            if x.get(key)!=y.get(key):differences.append({'component':key,'first':x.get(key),'second':y.get(key)})
        for actor in sorted(x['actors'].keys()|y['actors'].keys()):
            left,right=x['actors'].get(actor),y['actors'].get(actor)
            if left is None or right is None:differences.append({'actor':actor,'component':'missing-actor'});continue
            for i in range(max(len(left),len(right))):
                u=left[i] if i<len(left) else None;v=right[i] if i<len(right) else None
                if u!=v:
                    entry={'actor':actor,'field':FIELDS[i] if i<len(FIELDS) else 'word'+str(i),'first_bits':u,'second_bits':v}
                    if 20<=i<28 and u is not None and v is not None:
                        entry.update(first_float=struct.unpack('<f',struct.pack('<I',u))[0],second_float=struct.unpack('<f',struct.pack('<I',v))[0])
                    differences.append(entry)
        if differences and result['first_state_difference'] is None:result['first_state_difference']={'frame':tick,'differences':differences}
        if result['first_extra_rng_difference'] is None and any(x.get(k)!=y.get(k) for k in ('mt','extra_rng')):
            result['first_extra_rng_difference']={'frame':tick,'components':[k for k in ('mt','extra_rng') if x.get(k)!=y.get(k)]}
        for player in sorted(x['inputs'].keys()&y['inputs'].keys()):
            left,right=x['inputs'][player],y['inputs'][player]
            for kind in ('actual','used'):
                if left.get(kind+'_valid')=='1' and right.get(kind+'_valid')=='1' and left[kind]!=right[kind] and result['first_input_difference'] is None:
                    result['first_input_difference']={'frame':tick,'player':player,'kind':kind,'first':left[kind],'second':right[kind]}
    result['note']='First differences within the retained window, not proof of the original cause. Extra RNG is recorded for investigation and is not part of the existing wire checksum. Tick-entry state precedes that tick\'s input.'
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('first');parser.add_argument('second')
    args=parser.parse_args()
    print(json.dumps(compare(read(args.first),read(args.second)),indent=2))


if __name__=='__main__':main()
