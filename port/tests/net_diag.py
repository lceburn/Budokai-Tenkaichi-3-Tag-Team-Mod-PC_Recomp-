"""Run asset-free diagnostic history tests and produce paired sample captures."""
from pathlib import Path
import os
import subprocess
import tempfile
import sys

ROOT=Path(__file__).resolve().parents[2]


def main():
    out=ROOT/'port/build/net-diag-tests';out.mkdir(exist_ok=True)
    exe=out/'capture'
    subprocess.run(['cc','-std=gnu99','-O2','port/tests/net_diag.c','-o',str(exe)],cwd=ROOT,check=True)
    for name in ('host','client'):
        folder=out/name;folder.mkdir(exist_ok=True)
        subprocess.run([exe,name],cwd=folder,check=True)
    sys.path.insert(0,str(ROOT/'port/tools'))
    from compare_net_desync import read,compare
    a=read(out/'host/net-desync.txt');b=read(out/'client/net-desync.txt')
    a['meta']['remote']=b['meta']['local'];b['meta']['remote']=a['meta']['local']
    result=compare(a,b)
    assert result['metadata_match'] and result['configuration_match'] and result['paired_checksums']
    assert not result['capture_hash_errors']
    assert result['first_state_difference']['frame']==599
    assert result['first_state_difference']['differences']==[{'actor':7,'field':'hp','first_bits':0,'second_bits':1}]
    assert result['first_input_difference'] is None and result['first_extra_rng_difference'] is None
    assert compare(a,a)['first_state_difference'] is None
    print('PASS: paired comparison identifies injected actor/field/frame and independently recomputes captured checksums.')
    with tempfile.TemporaryDirectory() as temp:
        folder=Path(temp)
        env=dict(os.environ,BT3_NET_DIAG='0')
        subprocess.run([exe,'disabled'],cwd=folder,env=env,check=True)
        assert not (folder/'net-desync.txt').exists()
        (folder/'disable-net-diagnostics.txt').touch()
        subprocess.run([exe,'disabled'],cwd=folder,check=True)
        (folder/'net-desync.txt').mkdir()
        subprocess.run([exe,'fail'],cwd=folder,check=True)
    print('PASS: environment/file opt-out and report write failure do not interrupt the game.')


if __name__=='__main__':main()
