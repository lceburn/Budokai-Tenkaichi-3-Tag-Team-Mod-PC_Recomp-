"""Check a build's startup mappings against a locally owned disc, without installing.

The report contains a disc-profile name and the build-manifest digest, never
disc contents or the ISO path. A successful result checks startup tables only.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

import native_tables


def check(iso, manifest):
    data = Path(manifest).read_bytes()
    result = {'schema': 1, 'manifest_sha256': hashlib.sha256(data).hexdigest(),
              'profile': None, 'status': 'invalid_manifest'}
    if len(data) < 16 or data[:4] not in (b'BT3D', b'TTMD'):
        return result
    count, = struct.unpack_from('<I', data, 4)
    if len(data) != 16 + count * 16:
        return result
    try:
        profile, blobs, sizes = native_tables.identify(iso)
    except ValueError:
        result['status'] = 'unsupported_disc'
        return result
    result['profile'] = profile['id']
    try:
        tables = native_tables.reconstruct(profile, blobs, sizes)
        native_tables.verify(tables, data)
    except ValueError:
        result['status'] = 'startup_mismatch'
        return result
    result['status'] = 'compatible'
    return result


def main():
    frozen = getattr(sys, 'frozen', False)
    base = Path(sys.executable).parent if frozen else Path(__file__).resolve().parents[2] / 'port/build'
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--iso', type=Path)
    parser.add_argument('--manifest', type=Path, default=base / 'startup-manifest.dat')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    gui = args.iso is None
    if gui:
        import tkinter as tk
        from tkinter import filedialog, messagebox
        window = tk.Tk()
        window.withdraw()
        selected = filedialog.askopenfilename(title='Choose your BT3 or BT4 disc image',
                                             filetypes=[('Disc image', '*.iso')])
        if not selected:
            window.destroy()
            return 0
        args.iso = Path(selected)
    try:
        result = check(args.iso, args.manifest)
    except OSError:
        if gui:
            messagebox.showerror('Startup check', 'Could not read the disc or build manifest. '
                                 'Extract the complete check archive and try again.')
            window.destroy()
            return 2
        raise
    text = json.dumps(result, indent=2) + '\n'
    if gui:
        messages = {
            'compatible': 'Startup tables match this build. Gameplay still needs testing.',
            'startup_mismatch': 'The current startup mappings do not match this build. '
                                'Do not install it yet; the mappings need updating.',
            'unsupported_disc': 'This disc revision is not supported by the current profiles.',
            'invalid_manifest': 'The build manifest is damaged. Extract the complete check archive again.',
        }
        messagebox.showinfo('Startup check', messages[result['status']] +
                            '\n\nThe report contains no disc contents or ISO file path.')
        selected = filedialog.asksaveasfilename(title='Save startup check report',
                                              initialfile='startup-check.json',
                                              defaultextension='.json', filetypes=[('Report', '*.json')])
        if selected:
            Path(selected).write_text(text, encoding='utf-8')
        window.destroy()
    else:
        if args.report:
            args.report.write_text(text, encoding='utf-8')
        print(text, end='')
    return 0 if result['status'] == 'compatible' else 1


if __name__ == '__main__':
    raise SystemExit(main())
