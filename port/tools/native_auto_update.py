"""Check this fork's signed releases before starting the installed Windows game."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import subprocess
import tempfile
import urllib.request
import zipfile

import native_setup as setup
import native_update as updater
from native_update_key import PUBLIC_KEY
from update_key import verify

REPO = 'lceburn/Budokai-Tenkaichi-3-Tag-Team-Mod-PC_Recomp-'
API = 'https://api.github.com/repos/' + REPO + '/releases?per_page=100'
PREFIX = 'https://github.com/' + REPO + '/releases/download/'
MAX_ARCHIVE = 512 * 1024 * 1024


def version(value):
    match = re.fullmatch(r'(?:v0\.1\.0-)?beta\.(\d+)(-experimental)?(?:\.(\d+))?', value)
    return (int(match[1]), int(match[3] or 0), bool(match[2])) if match else None


def choose_release(releases, current):
    old = version(current['version'])
    if old is None:
        return None
    old = (old[0], current.get('release_sequence', 1), old[2])
    candidates = []
    for release in releases:
        new = version(release.get('tag_name', ''))
        if release.get('draft') or not new or new[2] != old[2] or new[:2] <= old[:2]:
            continue
        assets = {a['name']: a for a in release.get('assets', [])}
        name = f'Tag-Team-Recomp-Beta.{new[0]}-' + ('Experimental-' if new[2] else '') + 'Updater-Windows.zip'
        if name not in assets or name+'.sig' not in assets:
            continue
        archive, signature = assets[name], assets[name+'.sig']
        if not 0 < archive.get('size', 0) <= MAX_ARCHIVE or signature.get('size', 0) > 1024:
            continue
        for asset in (archive, signature):
            url = asset.get('browser_download_url', '')
            if not url.startswith(PREFIX) or '?' in url or '#' in url:
                break
        else:
            candidates.append((new[:2], release, archive, signature))
    return max(candidates, key=lambda c: c[0])[1:] if candidates else None


def fetch(url, limit):
    request = urllib.request.Request(url, headers={'User-Agent': 'TagTeam-native-updater', 'Accept': 'application/vnd.github+json' if url == API else 'application/octet-stream'})
    with urllib.request.urlopen(request, timeout=12) as response:
        data = response.read(limit+1)
    if len(data) > limit:
        raise ValueError('Update download exceeds the permitted size.')
    return data


def verified_payload(data, signature, destination, expected_size, expected_digest=None):
    if len(data) != expected_size:
        raise ValueError('Incomplete update archive.')
    if expected_digest and expected_digest != 'sha256:'+hashlib.sha256(data).hexdigest():
        raise ValueError('Update checksum does not match GitHub.')
    try:
        sig = bytes.fromhex(signature.decode('ascii').strip())
    except (ValueError, UnicodeError):
        raise ValueError('Invalid update signature.') from None
    if not verify(PUBLIC_KEY, data, sig):
        raise ValueError('Update signature verification failed.')
    archive = destination / 'update.zip'
    archive.write_bytes(data)
    payload = destination / 'payload'
    total = 0
    seen = set()
    with zipfile.ZipFile(archive) as source:
        for info in source.infolist():
            name = info.orig_filename
            parts = PurePosixPath(name).parts
            if '\\' in name or ':' in name or name.startswith('/') or '..' in parts:
                raise ValueError('Invalid archive path.')
            if not parts or parts[0] != 'payload' or info.is_dir():
                continue
            if '/'.join(parts) != name or name.casefold() in seen or (info.external_attr >> 16) & 0o170000 == 0o120000:
                raise ValueError('Invalid or duplicate archive entry.')
            seen.add(name.casefold())
            total += info.file_size
            if total > 1024*1024*1024:
                raise ValueError('Expanded update is too large.')
            target = destination.joinpath(*parts)
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(source.read(info))
    setup.payload_files(payload)
    return payload


def run(target, force=False):
    """The launcher hands off before starting the engine; an offline failure never blocks play."""
    import tkinter as tk
    from tkinter import filedialog, messagebox, ttk
    win = tk.Tk(); win.withdraw()
    target = target.resolve()
    try:
        current = json.loads((target/'release.json').read_text(encoding='utf-8'))
        settings=(target/'settings.txt').read_text(encoding='utf-8') if (target/'settings.txt').exists() else ''
        if not force and ((target/'disable-auto-updates.txt').exists() or re.search(r'^update_check\s*=\s*0\s*$',settings,re.M)):
            return
        candidate = choose_release(json.loads(fetch(API, 4*1024*1024)), current)
        if not candidate:
            if force: messagebox.showinfo('Tag Team updates', 'No newer signed update is available for this release channel.')
            return
        release, asset, signature = candidate
        if not messagebox.askyesno('Tag Team update', f"Install {release['tag_name']} before playing?\n\nSaves, settings, textures and edited mods are kept.\nChoose No to play your current version."):
            return
        win.deiconify();win.title('Updating Tag Team');win.geometry('540x150')
        status=ttk.Label(win,text='Downloading and verifying the signed update...',wraplength=500);status.pack(padx=20,pady=30)
        win.protocol('WM_DELETE_WINDOW',lambda:None);win.update()
        with tempfile.TemporaryDirectory(prefix='tag-team-download-') as folder:
            payload = verified_payload(fetch(asset['browser_download_url'], MAX_ARCHIVE), fetch(signature['browser_download_url'], 1024), Path(folder), asset['size'], asset.get('digest'))
            bundled = json.loads((payload/'release.json').read_text(encoding='utf-8'))
            if version(bundled.get('version','')) != (version(release['tag_name'])[0], 0, version(release['tag_name'])[2]):
                raise ValueError('Release version does not match its signed payload.')
            if bundled.get('release_sequence',1) != version(release['tag_name'])[1]:
                raise ValueError('Release sequence does not match its signed payload.')
            state=json.loads((target/'installation.json').read_text(encoding='utf-8'))
            iso=None
            try: updater.tables_for_update(target,payload,state)
            except ValueError:
                messagebox.showinfo('Startup data', 'This update needs your original disc image locally. It will not be uploaded.')
                iso=filedialog.askopenfilename(title='Choose the ISO used for this installation',filetypes=[('Disc image','*.iso')])
                if not iso:return
            def progress(text):status.configure(text=text);win.update()
            updater.update(target,payload,iso or None,progress)
    except Exception as error:
        if force or win.state() != 'withdrawn': messagebox.showerror('Tag Team update', str(error)+'\n\nYour current installation will start.')
        try:(target/'update-check.log').write_text(str(error)+'\n',encoding='utf-8')
        except OSError:pass
    finally:
        win.destroy()


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-folder', type=Path, required=True)
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--launch', action='store_true')
    args=parser.parse_args()
    try:
        run(args.game_folder,args.check)
    finally:
        if args.launch:
            subprocess.Popen([str(args.game_folder.resolve()/'Play.exe'),'--skip-update'],cwd=args.game_folder.resolve())
