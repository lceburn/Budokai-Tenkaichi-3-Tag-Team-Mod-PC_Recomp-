"""Compare private native network captures without uploading them anywhere."""
import argparse
import json
from pathlib import Path
import struct
import bisect
import re
import zipfile

FIELDS=('chara costume hp maxHp ki blast action target player lockedOff clash fused controller next seconds swap revive helper assist power posX posY posZ posW rotX rotY rotZ rotW').split()
TEAM_FIELDS=FIELDS[:20]+['team']+FIELDS[20:]
AI_FIELDS=('objId target type level managerFrame dist radiusSum sight seqFlags depth phase step timer ruleSet topId statusLo statusHi react oppClass oppAction seqHash statusHash moveHash planHash outHash timer18 timer1C timer20 buttons stickX stickY ruleNo').split()


def pairs(line):
    return dict(token.split('=',1) for token in line.split() if '=' in token)


def read(path):
    lines=Path(path).read_text(encoding='utf-8').splitlines()
    if not lines or not lines[0].startswith(('net-desync-v1 ','net-desync-v2 ','net-desync-v3 ')):raise ValueError('Not a network diagnostic capture: '+str(path))
    result={'meta':pairs(lines[0]),'frames':{},'config':[],'initial':{}}
    frame=None
    for line in lines[1:]:
        if line.startswith('initial_'):
            parts=line.split()
            if len(parts)>1:result['initial'][parts[0]]=[int(w,16) for w in parts[1:]]
        elif line.startswith(('config ','roster=','options')):result['config'].append(line)
        elif line.startswith('frame='):
            if 'unavailable' in line:frame=None;continue
            frame={'meta':pairs(line),'actors':{},'inputs':{},'rng_sites':[],'rng_events':[],'ai':{}}
            result['frames'][int(frame['meta']['frame'])]=frame
        elif frame is not None:
            if line.startswith('actor='):
                parts=line.split();frame['actors'][int(parts[0].split('=')[1])]=[int(w,16) for w in parts[2:]]
            elif line.startswith('input='):frame['inputs'][int(pairs(line)['input'])]=pairs(line)
            elif line.startswith('rng_site '):frame['rng_sites'].append(pairs(line))
            elif line.startswith('rng_event '):frame['rng_events'].append(pairs(line))
            elif line.startswith('rng_counts '):frame['rng_counts']=pairs(line)
            elif line.startswith('rng_coverage '):frame['rng_coverage']=pairs(line)
            elif line.startswith('ai actor='):
                parts=line.split();key=pairs(line)
                frame['ai'][int(key['actor']),int(key['stage'])]=[int(w,16) for w in parts[3:]]
            else:
                parts=line.split()
                if parts and parts[0] in ('header','rng','lobby','mt','extra_rng'):frame[parts[0]]=[int(w,16) for w in parts[1:]]
    return result


def recompute(frame):
    header_words=len(frame.get('header',[]))
    if header_words not in (4,6) or len(frame.get('rng',[]))!=2 or len(frame.get('lobby',[]))!=2:return None
    words=list(frame['header'])
    for actor in range(frame['header'][0]):
        if len(frame['actors'].get(actor,[]))!=(29 if header_words==6 else 28):return None
        words+=frame['actors'][actor]
    words+=frame['rng'];h=14695981039346656037
    for word in words:
        for b in struct.pack('<I',word):h=((h^b)*1099511628211)&((1<<64)-1)
    return h^(frame['lobby'][0]|frame['lobby'][1]<<32)


def compare(a,b,symbols=None):
    meta=('build','data','round','mismatch_frame')
    result={'metadata_match':all(a['meta'].get(k)==b['meta'].get(k) for k in meta),
            'configuration_match':a['config']==b['config'],
            'paired_checksums':a['meta'].get('local')==b['meta'].get('remote') and b['meta'].get('local')==a['meta'].get('remote'),
            'capture_hash_errors':[], 'first_state_difference':None,'first_extra_rng_difference':None,'first_input_difference':None,
            'initial_rng_difference':[k for k in a.get('initial',{}) if a['initial'].get(k)!=b.get('initial',{}).get(k)],
            'first_rng_count_difference':None,'first_rng_caller_difference':None,'first_rng_event_difference':None,
            'first_ai_random_result_difference':None,
            'ai_capture_available':any(f.get('ai') for f in a['frames'].values()) and any(f.get('ai') for f in b['frames'].values()),
            'first_ai_difference':None}
    if not result['metadata_match']:
        result.update(common_frames=0,note='Captures differ in build, data, round or mismatch frame. Pair reports from the same event before comparing states.')
        return result
    for name,report in (('first',a),('second',b)):
        for tick,frame in report['frames'].items():
            h=recompute(frame)
            if h is not None and h!=int(frame['meta']['hash'],16):result['capture_hash_errors'].append({'report':name,'frame':tick})
    common=sorted(a['frames'].keys()&b['frames'].keys());result['common_frames']=len(common)
    complete=min(int(a['meta']['current_frame']),int(b['meta']['current_frame']))
    for tick in common:
        x,y=a['frames'][tick],b['frames'][tick]
        if tick<complete:
            if result['first_ai_random_result_difference'] is None and not any(int(z.get('rng_coverage',{}).get('overwritten','0')) for z in (x,y)):
                # Equal generator advances do not imply equal AI decisions:
                # Rand_Range can apply different bounds to the same raw draw.
                ex=[e for e in x['rng_events'] if e['kind']=='9' and e['phase']=='4']
                ey=[e for e in y['rng_events'] if e['kind']=='9' and e['phase']=='4']
                for n,(u,v) in enumerate(zip(ex,ey)):
                    if any(u.get(k)!=v.get(k) for k in ('caller','before','after','result','actor')):
                        item={'frame':tick,'index':n,'first':u,'second':v}
                        if symbols:
                            item['first_symbol']=resolve(symbols,int(u['caller'],16))
                            item['second_symbol']=resolve(symbols,int(v['caller'],16))
                        result['first_ai_random_result_difference']=item;break
            if result['first_ai_difference'] is None:
                # Actor processing order is deterministic; stages are within
                # that actor's pass, after the frame-entry state observation.
                for actor,stage in sorted(x.get('ai',{}).keys()|y.get('ai',{}).keys()):
                    left=x.get('ai',{}).get((actor,stage));right=y.get('ai',{}).get((actor,stage))
                    if left is None or right is None:
                        result['first_ai_difference']={'frame':tick,'actor':actor,'stage':stage,'component':'missing-ai-pass'};break
                    changed=[{'field':AI_FIELDS[i] if i<len(AI_FIELDS) else str(i),'first_bits':u,'second_bits':v}
                             for i,(u,v) in enumerate(zip(left,right)) if u!=v]
                    if changed:
                        result['first_ai_difference']={'frame':tick,'actor':actor,'stage':stage,'differences':changed};break
            cx,cy=x.get('rng_counts',{}),y.get('rng_counts',{})
            if result['first_rng_count_difference'] is None:
                counts={k:{'first':cx.get(k),'second':cy.get(k)} for k in sorted(cx.keys()|cy.keys()) if k.startswith('k') and cx.get(k)!=cy.get(k)}
                if counts:result['first_rng_count_difference']={'frame':tick,'counts':counts}
            def sites(frame):
                output={}
                for s in frame['rng_sites']:
                    key=(s['kind'],s['phase'],s['caller'],s.get('actor','-1'));output[key]=output.get(key,0)+int(s['count'])
                return output
            sx,sy=sites(x),sites(y)
            if result['first_rng_caller_difference'] is None:
                changed=[]
                for key in sorted(sx.keys()|sy.keys()):
                    if sx.get(key,0)!=sy.get(key,0):
                        item=dict(zip(('kind','phase','caller','actor'),key));item.update(first_count=sx.get(key,0),second_count=sy.get(key,0))
                        if symbols:item['nearest_symbol']=resolve(symbols,int(key[2],16))
                        changed.append(item)
                if changed:result['first_rng_caller_difference']={'frame':tick,'sites':changed,'site_overflow_first':cx.get('overflow'),'site_overflow_second':cy.get('overflow')}
            if result['first_rng_event_difference'] is None and not any(int(z.get('rng_coverage',{}).get('overwritten','0')) for z in (x,y)):
                # Compare generator advances, not their wrapper annotations. View
                # numbers may legitimately differ with the local player's view.
                for kind in ('1','3','5'):
                    ex=[e for e in x['rng_events'] if e['kind']==kind];ey=[e for e in y['rng_events'] if e['kind']==kind]
                    if not ex and not ey:continue
                    for n in range(max(len(ex),len(ey))):
                        u=ex[n] if n<len(ex) else None;v=ey[n] if n<len(ey) else None
                        if u is None or v is None or any(u.get(k)!=v.get(k) for k in ('caller','phase','before','after','result','actor')):
                            item={'frame':tick,'kind':kind,'index':n,'first':u,'second':v}
                            if symbols:
                                item['first_symbol']=resolve(symbols,int(u['caller'],16)) if u else None
                                item['second_symbol']=resolve(symbols,int(v['caller'],16)) if v else None
                            result['first_rng_event_difference']=item;break
                    if result['first_rng_event_difference'] is not None:break
        differences=[]
        for key in ('header','rng','lobby'):
            if x.get(key)!=y.get(key):differences.append({'component':key,'first':x.get(key),'second':y.get(key)})
        for actor in sorted(x['actors'].keys()|y['actors'].keys()):
            left,right=x['actors'].get(actor),y['actors'].get(actor)
            if left is None or right is None:differences.append({'actor':actor,'component':'missing-actor'});continue
            fields=TEAM_FIELDS if len(left)==29 and len(right)==29 else FIELDS
            for i in range(max(len(left),len(right))):
                u=left[i] if i<len(left) else None;v=right[i] if i<len(right) else None
                if u!=v:
                    entry={'actor':actor,'field':fields[i] if i<len(fields) else 'word'+str(i),'first_bits':u,'second_bits':v}
                    if len(fields)-8<=i<len(fields) and u is not None and v is not None:
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
    result['note']='First differences within the retained window, not proof of the original cause. Extra RNG is not in the wire checksum. State is at tick entry; RNG calls are AFTER that entry and can explain a later state difference. Incomplete current-tick call intervals are excluded. Wrapper kinds are caller annotations, not additional RNG advances. Site/event overflow limits attribution.'
    return result


def load_symbols(path,build):
    with zipfile.ZipFile(path) as archive:
        meta=json.loads(archive.read('build.json'))
        if meta['build']['identity'][:16]!=build:raise ValueError('The symbol archive does not match this capture build.')
        text=archive.read('TagTeam.exe.map').decode('utf-8',errors='replace')
    entries=[]
    for line in text.splitlines():
        m=re.match(r'^\s*0x([0-9a-fA-F]+)\s+([A-Za-z_][\w]*)\s*$',line)
        if m:entries.append((int(m[1],16),m[2]))
    return sorted(set(entries))


def resolve(symbols,address):
    index=bisect.bisect_right(symbols,(address,chr(0x10ffff)))-1
    if index<0 or address-symbols[index][0]>=0x10000:return 'outside retained map range'
    base,name=symbols[index];return name+'+0x'+format(address-base,'x')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('first');parser.add_argument('second')
    parser.add_argument('--symbols',help='Matching diagnostic symbols ZIP; build identity is checked before resolving callers.')
    args=parser.parse_args()
    a,b=read(args.first),read(args.second)
    symbols=load_symbols(args.symbols,a['meta']['build']) if args.symbols else None
    print(json.dumps(compare(a,b,symbols),indent=2))


if __name__=='__main__':main()
