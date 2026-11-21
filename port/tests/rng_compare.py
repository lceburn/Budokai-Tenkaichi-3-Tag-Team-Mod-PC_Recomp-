"""Exercise v2 attribution without needing game assets or private captures."""
import copy
import json
from pathlib import Path
import sys
import tempfile
import zipfile

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'port/tools'))
from compare_net_desync import read,compare,load_symbols,resolve

def main():
    with tempfile.TemporaryDirectory() as temp:
        p=Path(temp)/'capture.txt'
        p.write_text('net-desync-v2 build=123456789abcdef0 data=1 round=1 mismatch_frame=10 current_frame=11 local=1 remote=1\n'
                     'initial_rng 00000001 00000000\nframe=10 hash=0\n'
                     'rng_counts frame=10 overflow=0 k1=1\n'
                     'rng_site frame=10 kind=1 phase=15 view=0 caller=0000000020010005 count=1 before=1 after=2\n'
                     'rng_coverage frame=10 overwritten=0\n'
                     'rng_event frame=10 seq=1 branch=0 kind=1 ordinal=1 phase=15 view=0 caller=0000000020010005 before=1 after=2 result=3\n')
        a=read(p);b=copy.deepcopy(a)
        b['frames'][10]['rng_sites'][0]['view']='1'
        b['frames'][10]['rng_events'][0].update(view='1',branch='2',seq='17')
        assert compare(a,b)['first_rng_event_difference'] is None
        assert compare(a,b)['first_rng_caller_difference'] is None
        assert not compare(a,b)['ai_capture_available']
        a['frames'][10]['ai']={(3,0):[0]*32}
        b['frames'][10]['ai']={(3,0):[0]*32}
        b['frames'][10]['ai'][3,0][21]=123
        delta=compare(a,b)['first_ai_difference']
        assert delta['actor']==3 and delta['stage']==0 and delta['differences'][0]['field']=='statusHash'
        b['frames'][10]['ai'].clear()
        assert compare(a,b)['first_ai_difference']['component']=='missing-ai-pass'
        a['frames'][10]['ai'].clear()
        a['frames'][10]['rng_events'].append(dict(kind='9',phase='4',caller='20010005',before='1',after='2',result='2f'))
        b['frames'][10]['rng_events'].append(dict(kind='9',phase='4',caller='20010005',before='1',after='2',result='61'))
        assert compare(a,b)['first_rng_event_difference'] is None
        assert compare(a,b)['first_ai_random_result_difference']['frame']==10
        a['frames'][10]['ai_check35']={3:[2,0x3f800000,1,0,0,1,0]}
        b['frames'][10]['ai_check35']={3:[2,0xbf800000,1,0,0x80000000,0,3]}
        check=compare(a,b)['first_ai_check35_difference']
        assert check['actor']==3 and check['differences'][0]['field']=='progressBits'
        a['frames'][10]['ai_check35'].clear();b['frames'][10]['ai_check35'].clear()
        b['frames'][10]['rng_counts']['k1']='2'
        b['frames'][10]['rng_sites'][0]['count']='2'
        b['frames'][10]['rng_events'][0]['caller']='0000000020010010'
        symbols=[(0x20010000,'EffectDraw')]
        result=compare(a,b,symbols)
        assert result['first_rng_count_difference']['frame']==10
        assert result['first_rng_caller_difference']['sites'][0]['nearest_symbol']=='EffectDraw+0x5'
        assert result['first_rng_event_difference']['second_symbol']=='EffectDraw+0x10'
        b['frames'][10]['rng_coverage']['overwritten']='1'
        assert compare(a,b)['first_rng_event_difference'] is None
        b['meta']['current_frame']='10'
        assert compare(a,b)['first_rng_count_difference'] is None
        b['meta']['build']='other'
        assert not compare(a,b)['metadata_match']
        z=Path(temp)/'symbols.zip'
        with zipfile.ZipFile(z,'w') as archive:
            archive.writestr('build.json',json.dumps({'build':{'identity':'123456789abcdef0extra'}}))
            archive.writestr('TagTeam.exe.map',' 0x20010000 EffectDraw\n')
        assert resolve(load_symbols(z,'123456789abcdef0'),0x20010005)=='EffectDraw+0x5'
        try:load_symbols(z,'wrong')
        except ValueError:pass
        else:raise AssertionError('mismatched symbols accepted')
    print('PASS: v2 parsing, caller/phase count and ordered event attribution, view/branch exclusion, overflow/current-tick limits, symbol resolution and build pairing.')

if __name__=='__main__':main()
