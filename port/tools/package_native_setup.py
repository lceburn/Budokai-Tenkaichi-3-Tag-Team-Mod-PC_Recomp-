"""Package a self-contained native installer from an already stripped build.

Requires PyInstaller and pycdlib in the packaging Python. No disc assets are
included. Run strip_data.py first and regenerate recipes if its ranges change.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import zipfile
import package as upstream
import native_tables

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--platform', choices=('windows', 'linux'), default='windows' if sys.platform == 'win32' else 'linux')
    parser.add_argument('--portable', type=Path, default=ROOT / 'port/build/installer-portable')
    parser.add_argument('--sdl', type=Path)
    parser.add_argument('--launcher', type=Path, default=ROOT / 'port/build/Play.exe')
    parser.add_argument('--output', type=Path, default=ROOT / 'port/build/native-setup')
    parser.add_argument('--version', type=int, default=5, help='Native beta release number')
    parser.add_argument('--release-sequence', type=int, default=1, help='Final numeric component of the GitHub release tag')
    parser.add_argument('--effect-diagnostic', action='store_true', help='Include opt-in effect lifecycle diagnostic instructions and enable helper.')
    parser.add_argument('--net-diagnostic', action='store_true', help='Include network desync capture instructions (capture enabled by default).')
    parser.add_argument('--experimental-startup', action='store_true',
                        help='Package an explicitly experimental build; Setup must verify the startup checksum against the local ISO before installation.')
    args = parser.parse_args()
    windows = args.platform == 'windows'
    if windows != (sys.platform == 'win32'):
        raise ValueError('Freeze Setup on the same operating system as the release.')
    dat = (args.portable / 'bt3.dat').read_bytes()
    try:
        native_tables.check_recipe_coverage(dat)
    except native_tables.RecipeCoverageError:
        if not args.experimental_startup:
            raise
        print('EXPERIMENTAL: startup coverage is unverified. Setup retains its mandatory disc-table checksum check.')
    test_note = ('EXPERIMENTAL TEST BUILD — gameplay and disc compatibility are unverified.\n'
                 'Setup checks your disc startup data before installing and stops if it does not match.\n\n'
                 if args.experimental_startup else '')
    output = args.output.resolve()
    if args.effect_diagnostic:
        test_note += ('REMATCH EFFECT-LIFECYCLE DIAGNOSTICS — gameplay verification still required.\n'
                      'After updating, open engine/Enable effect diagnostics.cmd in the installed game folder, then restart Play.\n'
                      'Read engine/Effect diagnostic instructions.txt for offline reproduction and ProcDump capture.\n\n')
    if args.net_diagnostic:
        test_note += ('LAN / INTERNET DESYNC DIAGNOSTICS — capture enabled automatically during native network matches.\n'
                      'After a desync, save net-desync.txt and game.log from BOTH installed game folders.\n'
                      'Read engine/Network diagnostic instructions.txt. No effect diagnostic CMD or ProcDump is required.\n\n')
    if output.exists() and any(output.iterdir()):
        raise ValueError('Choose an empty packaging output folder.')
    output.mkdir(parents=True, exist_ok=True)
    payload = output / 'payload'
    engine = payload / 'engine'
    engine.mkdir(parents=True, exist_ok=True)
    if args.net_diagnostic:
        shutil.copy2(ROOT/'docs/NETWORK_DESYNC_DIAGNOSTIC.md',engine/'Network diagnostic instructions.txt')
    if args.effect_diagnostic:
        # Keep helper files under engine/ so existing updaters accept them too.
        shutil.copy2(ROOT/'docs/REMATCH_EFFECT_DIAGNOSTIC.md',engine/'Effect diagnostic instructions.txt')
        (engine/'Enable effect diagnostics.cmd').write_text('@echo off\ncd /d "%~dp0.."\ntype nul > effect-diagnostics.enabled\necho Effect lifecycle diagnostics enabled. Start Play.exe and reproduce offline.\necho To disable, delete effect-diagnostics.enabled while the game is closed.\npause\n')
    if windows:
        if not args.launcher.is_file():
            raise ValueError('Build Play.exe with port/tools/build_native_launcher.py first.')
        shutil.copy2(args.launcher, payload / 'Play.exe')
    shutil.copy2(ROOT / 'docs/assets/tag-team-logo.png', payload / 'Tag Team Mod Logo.png')
    binary = 'TagTeam.exe' if windows else 'TagTeam'
    source_binary = 'bt3.exe' if windows else 'bt3'
    mem = args.portable / (source_binary + '.mem')
    if not mem.is_file():
        mem = ROOT / ('port/build/bt3.exe.mem' if windows else 'port/build/bt3_64.mem')
    sdl = args.sdl or (ROOT / 'port/build/SDL3.dll' if windows else Path('/usr/local/lib/libSDL3.so.0'))
    for src, name in [(args.portable / source_binary, binary), (mem, binary + '.mem'),
                      (sdl, 'SDL3.dll' if windows else 'lib/libSDL3.so.0')]:
        (engine / name).parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, engine / name)
    if not windows:
        (engine / binary).chmod(0o755)
    if dat[:4] != b'BT3D':
        raise ValueError('Use the manifest from strip_data.py')
    (engine / 'TagTeam.dat').write_bytes(b'TTMD' + dat[4:])
    licenses = payload / 'licenses'
    licenses.mkdir(exist_ok=True)
    upstream.take_translations(payload)
    upstream.take_lang(payload)
    upstream.take_fonts(payload, licenses)
    upstream.take_tbo_art(payload, licenses)
    upstream.take_example_mods(payload)
    if windows:
        # Installed checker runs from a temporary copy, before the engine starts.
        subprocess.run([sys.executable, '-m', 'PyInstaller', '--noconfirm', '--clean', '--onefile', '--windowed',
            '--name', 'CheckUpdates', '--distpath', str(payload),
            '--workpath', str(ROOT / 'port/build/auto-update-freeze'),
            '--specpath', str(ROOT / 'port/build/auto-update-freeze'),
            '--paths', str(ROOT / 'port/tools'),
            '--add-data', str(ROOT / 'port/tools/native_table_recipes.json') + os.pathsep + '.',
            str(ROOT / 'port/tools/native_auto_update.py')], check=True)
    for source, name in [('LICENSE', 'AGPL-3.0.txt'), ('LICENSE-MIT', 'upstream-MIT.txt'),
        ('port/third_party/imgui/LICENSE.txt', 'dear-imgui.txt'),
        ('port/third_party/newlib_libm/COPYING.NEWLIB', 'newlib.txt'),
        ('port/third_party/xxhash/LICENSE', 'xxhash.txt'),
        ('port/third_party/miniaudio/LICENSE', 'miniaudio.txt'),
        ('port/third_party/miniz/LICENSE', 'miniz.txt'),
        ('port/third_party/lua/LICENSE', 'lua.txt'),
        ('port/third_party/monocypher/LICENCE.md', 'monocypher.txt')]:
        shutil.copy2(ROOT / source, licenses / name)
    shutil.copy2(ROOT / 'port/licenses/SDL.txt', licenses / 'SDL.txt')
    launcher = 'Play.exe' if windows else 'Play.sh'
    (payload / 'Read me.txt').write_text(
        test_note + f'Tag Team Mod — Recomp Beta .{args.version}\n\nStart {launcher}. Press Select at the main menu for Modded Modes.\n'
        'Double-click the game picture or press Alt+Enter to switch fullscreen. F11 also works.\n'
        'F1 opens graphics, controller, language and mod settings. Online modes are in the game menu.\n'
        'Mod Settings > Controls changes target, lock-off and menu buttons. F1 > Controls changes keyboard and gamepad bindings.\n'
        'Your saves are in saves/. Setup copies imported saves; it never changes the original.\n'
        'English and Spanish mod menus are included. Disc audio comes from your selected disc.\n'
        'BT3: USA, Europe/Australia, Japan. BT4: B14 REV2 English and Spanish.\n\n'
        'Source: https://github.com/lceburn/Budokai-Tenkaichi-3-Tag-Team-Mod-PC_Recomp-\n', encoding='utf-8')
    manifest = {p.relative_to(payload).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                for p in sorted(payload.rglob('*')) if p.is_file() and p.name != 'manifest.json'}
    release = dict(version=f'beta.{args.version}' + ('-experimental' if args.experimental_startup else ''),
                   platform=args.platform+'-x86_64', release_sequence=args.release_sequence)
    if args.effect_diagnostic:release['diagnostic']='rematch-effect-lifecycle-v1'
    if args.net_diagnostic:release['diagnostic']='network-desync-v1'
    if args.experimental_startup:
        release.update(experimental=True, startup_verification='install-time-checksum')
    (payload / 'release.json').write_text(json.dumps(release)+'\n')
    # Include release metadata in the same integrity manifest as every asset.
    manifest['release.json']=hashlib.sha256((payload/'release.json').read_bytes()).hexdigest()
    (payload / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    for name,script in [('Setup','native_setup.py'),('Update','native_update.py')]:
        subprocess.run([sys.executable, '-m', 'PyInstaller', '--noconfirm', '--clean', '--onefile', '--windowed',
            '--name', name, '--distpath', str(output), '--workpath', str(ROOT / ('port/build/'+name.lower()+'-freeze-'+args.platform)),
            '--specpath', str(ROOT / ('port/build/'+name.lower()+'-freeze-'+args.platform)), '--paths', str(ROOT / 'port/tools'),
            '--add-data', str(ROOT / 'port/tools/native_table_recipes.json') + os.pathsep + '.',
            str(ROOT / 'port/tools'/script)], check=True)
    setup = 'Setup.exe' if windows else './Setup.sh'
    (output / 'Read me.txt').write_text(test_note + f'Extract the whole archive, then run {setup}.\n'
        'Already installed? Run Update instead and choose your existing game folder.\n'
        'Updating keeps your saves, settings, bindings and imported disc data. Close the game first.\n'
        'Choose your disc image, English or Spanish, and an optional existing save.\n'
        'Setup needs no separate Python installation. Keep the payload folder beside Setup.\n'
        f'After installation, open {launcher} in the folder you chose.\n\n'
        f'Extrae todo el archivo y ejecuta {setup}. Elige el disco, el idioma y una partida existente si la tienes.\n'
        'No hace falta instalar Python. Conserva la carpeta payload junto a Setup.\n'+
        'Si ya lo tienes instalado, ejecuta Update y elige la carpeta del juego. Se conservan tus partidas y ajustes.\n'+
        ('' if windows else '\nLinux: x86-64, glibc 2.35 or newer (Ubuntu 22.04+), and a working Vulkan graphics driver.\nRun Setup.sh and Play.sh as your normal user, without sudo.\n'), encoding='utf-8')
    if not windows:
        for name in ('Setup','Update'):
            (output/(name+'.sh')).write_text('#!/bin/sh\nset -eu\ncd -- "$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"\nexec "$PWD/'+name+'" "$@"\n')
            (output/(name+'.sh')).chmod(0o755)
    updater_readme=(test_note+'Close the game. Extract this archive outside your installed game folder.\n'
        'Run '+('Update.exe' if windows else './Update.sh')+' and choose that existing folder.\n'
        'Saves, settings, controller bindings and disc assets stay in place.\n'
        'Use the same disc ISO only if the updater asks for additional startup tables.\n'
        'This works with earlier Recomp betas; it is not an updater for the PCSX2 mod.\n\n'
        'Cierra el juego. Extrae el archivo fuera de la carpeta del juego instalado.\n'
        'Ejecuta Update y elige la carpeta existente. Se conservan tus partidas, ajustes y controles.\n'
        'Solo hace falta el disco original si se solicitan tablas adicionales.\n')
    for update_only in (False,True):
        import io
        label='Updater-' if update_only else ''
        archive=output.parent/(f'Tag-Team-Recomp-Beta.{args.version}-'+('Experimental-' if args.experimental_startup else '')+label+('Windows.zip' if windows else 'Linux-x86_64.tar.gz'))
        paths=[p for p in sorted(output.rglob('*')) if p.is_file() and not (update_only and p.name in ('Setup','Setup.exe','Setup.sh'))]
        if windows:
            with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
                for p in paths:
                    if update_only and p==output/'Read me.txt':z.writestr('Read me.txt',updater_readme)
                    else:z.write(p,p.relative_to(output))
        else:
            with tarfile.open(archive,'w:gz') as tar:
                for p in paths:
                    info=tar.gettarinfo(str(p),arcname=p.relative_to(output).as_posix())
                    info.uid=info.gid=0;info.uname=info.gname=''
                    data=updater_readme.encode() if update_only and p==output/'Read me.txt' else p.read_bytes()
                    info.size=len(data);tar.addfile(info,io.BytesIO(data))
        print(archive)


if __name__ == '__main__':
    main()
