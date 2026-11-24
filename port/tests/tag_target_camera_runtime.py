"""Two real clients target with identical pads but different visual cameras.

Run once on the unfixed diagnostic candidate to reproduce, then on the fix.
The executable needs the opt-in BT3_TAG_TEST_TARGET_INPUT pad probe.
"""
import argparse
import pathlib
import subprocess
import sys

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--exe', required=True)
p.add_argument('--data', required=True)
p.add_argument('--client-data', required=True)
p.add_argument('--label', default='target-camera-regression')
p.add_argument('--port', type=int, default=7088)
p.add_argument('--frames', type=int, default=3600)
p.add_argument('--same-camera', action='store_true', help='Control: no guest camera modification')
a = p.parse_args()
root = pathlib.Path(__file__).resolve().parents[2]
exe = pathlib.Path(a.exe)
if not exe.is_absolute():
    exe = root / exe
if b'BT3_TAG_TEST_TARGET_INPUT' not in exe.read_bytes():
    p.error('This executable lacks the targeting pad probe; an idle match is not a valid test.')
probe = root / 'port/build/tag-online' / (a.label + '-camera')
(probe / 'mods/camera_probe').mkdir(parents=True, exist_ok=True)
(probe / 'mods/camera_probe/mod.lua').write_text(
    'return {name="Visual camera isolation probe",on_camera=function(cam,game)\n'
    ' if not cam.cut then cam.rot.z=cam.rot.z+math.pi end\nend}\n')
(probe / 'settings.txt').write_text('camera_probe=1\n')
cmd = [sys.executable, str(root / 'port/tests/tag_online_runtime.py'),
       '--bt4', '--creator', '--exe', a.exe, '--data', a.data,
       '--client-data', 'guest=' + a.client_data, '--players', '2', '--size', '2,1',
       '--frames', str(a.frames), '--host-speed', '114', '--guest-speed', '114',
       '--paced', '--gpu-client', 'guest', '--input', '--input-spread',
       '--timeout', str(max(150, a.frames // 40)), '--port', str(a.port),
       '--label', a.label, '--capture-desync']
for setting in ('tag_target_order=1', 'tag_lockoff_hold=80', 'tag_target_stick=2'):
    cmd += ['--setting', setting]
for value in ('BT3_FRAME_SKIP=0', 'BT3_TAG_TRACE=1', 'BT3_TAG_TEST_TARGET_INPUT=1'):
    cmd += ['--env', value]
if not a.same_camera:
    cmd += ['--client-env', 'guest:BT3_MODS=' + str(probe / 'mods'),
            '--client-env', 'guest:BT3_MODS_SETTINGS=' + str(probe / 'settings.txt')]
raise SystemExit(subprocess.call(cmd, cwd=root))
