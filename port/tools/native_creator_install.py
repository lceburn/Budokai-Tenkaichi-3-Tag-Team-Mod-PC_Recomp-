"""Prepare private creator sources from the installed BT4 data, atomically.

The cache is generated locally; it is never a release asset or a BT3 dependency.
A failed preparation leaves imported data and any existing cache untouched.
"""
import hashlib
import json
from pathlib import Path
import shutil
import tempfile

from prepare_warrior_bt4_sources import prepare

SCHEMA = 1
MARKER = 'creator-cache.json'


def identity(profile):
    fields = {key: profile.get(key) for key in ('game', 'revision', 'language',
              'executable_sha256', 'menu_sha256', 'addon_sha256')}
    return hashlib.sha256(json.dumps(fields, sort_keys=True).encode()).hexdigest()


def ensure(data, enabled=True, progress=print):
    if not enabled:
        return False
    data = Path(data).resolve(strict=True)
    profile = json.loads((data / 'native-disc.json').read_text(encoding='utf-8'))
    if profile.get('game') != 'bt4':
        return False
    if profile.get('revision') != 'b14-rev2':
        raise ValueError('The character creator requires supported BT4 B14 REV2 data.')
    cache = data / 'warrior-sources'
    wanted = {'schema': SCHEMA, 'disc': identity(profile)}
    if cache.is_symlink() or getattr(cache, 'is_junction', lambda: False)():
        raise ValueError('The installed creator source folder must not be a linked development folder.')
    if cache.exists():
        try:
            stamp = json.loads((cache / MARKER).read_text(encoding='utf-8'))
        except (OSError, ValueError):
            raise ValueError('An unrecognized creator source folder already exists. It has been left unchanged.')
        if stamp != wanted:
            raise ValueError('The existing creator sources belong to a different disc or format. They have been left unchanged.')
        if not all((cache / name).is_file() for name in ('index.tsv', 'assets.tsv', 'provenance.json')):
            raise ValueError('Creator source preparation is incomplete. The existing folder has been left unchanged.')
        return True
    if shutil.disk_usage(data).free < 3 * 1024 ** 3:
        raise ValueError('Preparing the character creator needs at least 3 GB of additional free space.')
    work = Path(tempfile.mkdtemp(prefix='.creator-sources-', dir=data))
    try:
        progress('Preparing character creator / Preparando creador de personajes...')
        built = work / 'warrior-sources'
        def report(number, name):
            progress(f'Preparing creator: {name} / Preparando creador: {name}')
        prepare(data, built, progress=report)
        (built / MARKER).write_text(json.dumps(wanted) + '\n', encoding='utf-8')
        # Atomic publication of a complete, usable cache. No existing folder is
        # replaced. Older executables simply ignore this locally generated data
        # if the subsequent program update is cancelled or rolls back.
        if cache.exists():
            raise ValueError('Creator sources appeared during preparation. No existing folder was replaced.')
        built.rename(cache)
    finally:
        assert work.parent.resolve() == data and work.name.startswith('.creator-sources-')
        if work.exists():
            shutil.rmtree(work)
    return True
