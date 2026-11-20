"""Verify native Play.exe creator migration with a compiled stub child (Windows)."""
from pathlib import Path
import argparse
import shutil
import subprocess
import tempfile
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--launcher', type=Path, required=True)
parser.add_argument('--stub', type=Path, required=True)
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix='tag-creator-launch-test-') as temp:
    root = Path(temp)
    (root / 'engine').mkdir()
    (root / 'gamedata/warrior-sources').mkdir(parents=True)
    shutil.copy2(args.launcher, root / 'Play.exe')
    shutil.copy2(args.stub, root / 'engine/TagTeam.exe')
    shutil.copy2(args.stub, root / 'CheckUpdates.exe')
    roster = root / 'gamedata/native-roster.tsv'
    marker = root / 'gamedata/warrior-sources/creator-cache.json'
    roster.write_text('BT4 probe')
    def run(argument, output):
        output.unlink(missing_ok=True)
        result = subprocess.run([str(root / 'Play.exe'), argument], cwd=root, timeout=20)
        assert result.returncode == 0
        deadline = time.monotonic() + 5
        while not output.exists() and time.monotonic() < deadline:
            time.sleep(.05)
        assert output.is_file()
        return output.read_text()
    helper = run('--skip-update', root / 'launcher-helper.txt')
    assert '--prepare-creator' in helper and '--launch' in helper
    assert not (root / 'launcher-engine.txt').exists()
    marker.write_text('{}')
    text = run('--skip-update', root / 'launcher-engine.txt')
    assert 'models=1' in text and 'head=1' in text
    marker.unlink()
    assert 'models=off' in run('--creator-unavailable', root / 'launcher-engine.txt')
    roster.unlink()
    assert 'models=off' in run('--skip-update', root / 'launcher-engine.txt')
print('PASS: first-launch migration, prepared creator, failure fallback, BT3 stock path')
