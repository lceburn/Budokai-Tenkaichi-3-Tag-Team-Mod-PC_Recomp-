"""Prepare local developer launchers only after the recorded game smoke tests pass.

The output refers to this checkout's private game data. It is not a distributable
release or an installer and never publishes files or touches another installation.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'port/tests'))
from warrior_runtime import warrior_inputs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--results", type=Path, default=ROOT / "port/build/warrior-runtime")
    parser.add_argument("--transform-results",type=Path,required=True,help="Successful round trips starting from each form")
    parser.add_argument("--selection-results",type=Path,required=True,help="Native picker, return, retry and direct launcher checks")
    parser.add_argument("--exe", type=Path, default=ROOT / "port/build/bt3.exe")
    parser.add_argument("--data", type=Path, default=ROOT / "gamedata-bt4")
    parser.add_argument("--sdl", type=Path, required=True)
    parser.add_argument("--output", type=Path, default=ROOT / "port/build/warrior-playtest")
    parser.add_argument("--visual-approved", action="store_true", help="Confirm manual review of this build's GPU captures")
    parser.add_argument("--creator-results",type=Path,help="Native edit/build/refresh/fight result.json for this executable")
    parser.add_argument("--builder-copy",type=Path,help="Prepared isolated local authoring helper")
    parser.add_argument("--python-exe",type=Path,default=Path(sys.executable))
    parser.add_argument("--live-preview",action="store_true",help="Package the highlighted-preview/layers/sizing candidate")
    parser.add_argument("--confirmed-preview",action="store_true",help="Build previews only after applying a choice; require browsing and whole-head checks")
    parser.add_argument("--attack-controls",action="store_true",help="Require attack name/colour/effect and reviewed runtime-label checks")
    parser.add_argument("--attack-label-result",type=Path)
    parser.add_argument("--authoring-result",type=Path,action="append",default=[])
    parser.add_argument("--classic-result",type=Path,help="Reviewed regular-versus fight result for this build and data")
    args = parser.parse_args()
    if args.confirmed_preview:args.live_preview=True
    results = json.loads((args.results / "results.json").read_text())
    record = json.loads((args.results / "build.json").read_text())
    digest = hashlib.sha256(args.exe.read_bytes()).hexdigest()
    authoring={}
    if args.live_preview:
        if not args.creator_results:parser.error('Live previews require a native creator fight proof')
        for path in args.authoring_result:
            proof=json.loads(path.read_text())
            if not proof.get('passed') or proof.get('executable')!=digest:
                parser.error(f'Missing or stale native authoring check: {path}')
            authoring[proof['case']]=path
        required={'browse-no-build','whole-head-save-fight'} if args.confirmed_preview else {'body-cancel','layer-save','sizing-save-fight'}
        if args.attack_controls:required|={'attack-controls-save','attack-effects-save-fight'}
        if not required.issubset(authoring):
            parser.error('The required native authoring checks are missing for this preview mode')
        if args.attack_controls:
            if not args.attack_label_result:parser.error('Attack controls need reviewed battle-label checks')
            labels=json.loads(args.attack_label_result.read_text())
            if labels.get('executable')!=digest or not all(labels.get(k) for k in ('passed','pause_list_reviewed','mod_list_reviewed')):
                parser.error('The runtime attack-label check is missing or stale')
        if not args.classic_result:parser.error('The corrected regular-versus picker needs a reviewed fight check')
        classic=json.loads(args.classic_result.read_text())
        if not classic.get('passed') or classic.get('executable')!=digest or classic.get('warrior_inputs')!=warrior_inputs(args.data):
            parser.error('The regular-versus fight check is missing or stale')
    if args.creator_results:
        proof=json.loads(args.creator_results.read_text())
        if proof.get('executable')!=digest or not all(proof.get(k) for k in
                ('entered_creator','built','reloaded','fight_completed')):
            parser.error('This creator executable needs a successful native edit-to-fight check')
        if not args.builder_copy or not (args.builder_copy/'.bt4-builder-copy.json').is_file():
            parser.error('The creator needs a prepared isolated builder copy')
        for name in ('zw_bg','zw_emblem','zw_bar','zw_bar_sel','zw_dialog','zw_kk_point','zw_platform','zw_pill','zw_pill_steel'):
            if not (args.exe.parent/'tbo/ui'/f'{name}.png').is_file():
                parser.error(f'Missing creator artwork: {name}')
    if record["executable"] != digest or not results or not all(result["passed"] for result in results):
        parser.error("The smoke results do not establish this executable passed")
    if not args.visual_approved or record.get("renderer")!="gpu":
        parser.error("Review successful GPU captures before preparing a playtest")
    if record.get("warrior_inputs") != warrior_inputs(args.data):
        parser.error("Warrior data or folder order changed after the smoke test")
    for chara in (250, 251):
        if not any(result["case"].startswith(f"{chara}-0-0-") for result in results):
            parser.error("Both custom forms need successful smoke tests")
    transforms=json.loads((args.transform_results / "results.json").read_text())
    transform_build=json.loads((args.transform_results / "build.json").read_text())
    if (transform_build.get("executable")!=digest or
        transform_build.get("warrior_inputs")!=record["warrior_inputs"] or
        not transform_build.get("transform_roundtrip") or
        not all(result["passed"] for result in transforms) or
        not all(any(result["case"].startswith(f"{c}-0-0-") for result in transforms) for c in (250,251))):
        parser.error("This executable and warrior data need successful round trips from both forms")
    for case in ("base-match","second-match","return-characters","retry","direct-match"):
        selected=json.loads((args.selection_results/case/"result.json").read_text())
        if(not selected.get("passed") or selected.get("executable")!=digest or
           selected.get("warrior_inputs")!=record["warrior_inputs"]):
            parser.error(f"Native selection check is missing or stale: {case}")
    if args.output.exists() and any(args.output.iterdir()):
        parser.error("Choose an empty playtest output folder")
    engine = args.output / "engine"
    engine.mkdir(parents=True)
    for source, name in ((args.exe, "Warrior-test.exe"),
                         (Path(str(args.exe) + ".mem"), "Warrior-test.exe.mem"),
                         (args.exe.with_suffix(".dat"), "Warrior-test.dat"),
                         (args.sdl, "SDL3.dll")):
        shutil.copy2(source, engine / name)
    (args.output / "logs").mkdir()
    (args.output / "saves").mkdir()
    # Use the same bundled script library as a normal fresh package. Keep its
    # enable/settings file local to this playtest, just like saves and settings.
    for mod in sorted((ROOT / "port/examples/mods").iterdir()):
        if mod.is_dir() and (mod / "mod.lua").is_file():
            shutil.copytree(mod, args.output / "mods" / mod.name)
    (args.output / "settings.txt").write_text("tag_extra_intros=0\n", encoding="utf-8")
    def batch_path(path):
        return str(path.resolve()).replace("%", "%%")
    for chara, name in ((250, "Play base form"), (251, "Play second form")):
        script = ("@echo off\nsetlocal\n"
                  f'cd /d "{batch_path(ROOT)}"\n'
                  f'set "BT3_DATA={batch_path(args.data)}"\n'
                  'set "BT3_GS=gpu"\nset "BT3_DEMO=1"\nset "BT3_REPLAY="\nset "BT3_TAG_MENU_TEST="\n'
                  'set "BT3_TAG_TEAM=1"\nset "BT3_TAG_ROSTER=1,1"\nset "BT3_TAG_PLAY=1"\n'
                  'set "BT3_WARRIOR_MODELS=1"\n'
                  f'set "BT3_CHARA={chara}"\n'
                  'set "BT3_COSTUME=0"\nset "BT3_VARIANT="\nset "BT3_TEST_FRAMES="\n'
                  'if "%BT3_PLAYTEST_SMOKE%"=="1" set "BT3_TEST_FRAMES=60"\n'
                  'set "BT3_UPDATE_CHECK=0"\nset "BT3_TAG_TRACE=1"\n'
                  'set "BT3_SAVES=%~dp0saves"\nset "BT3_SETTINGS=%~dp0settings.txt"\n'
                  'set "BT3_MODS=%~dp0mods"\nset "BT3_MODS_SETTINGS=%~dp0mods-settings.txt"\n'
                  f'"%~dp0engine\\Warrior-test.exe" > "%~dp0logs\\{chara}.log" 2>&1\n'
                  'if errorlevel 1 (echo The experimental game exited with an error. See the logs folder. & pause)\n')
        (args.output / (name + ".cmd")).write_text(script, encoding="utf-8")
    roster=(script.replace('set "BT3_DEMO=1"','set "BT3_DEMO="')
            .replace('set "BT3_TAG_MENU_TEST="','set "BT3_TAG_MENU_TEST=roster"')
            .replace('set "BT3_CHARA=251"','set "BT3_CHARA="')
            .replace('if "%BT3_PLAYTEST_SMOKE%"=="1" set "BT3_TEST_FRAMES=60"\n','')
            .replace('logs\\251.log','logs\\roster.log'))
    (args.output / "Play custom roster.cmd").write_text(roster,encoding="utf-8")
    (args.output / "Read me.txt").write_text(
        "LOCAL OFFLINE CUSTOM WARRIOR PLAYTEST\n\n"
        "Run Play custom roster.cmd for the native team setup and picker.\n"
        "Start team setup, then Up followed by Right from the first fighter\n"
        "cell reaches ICEBURN on the final row. Confirm and choose either form.\n"
        "Small roster icons use the supplied portrait when a native chip exists.\n"
        "The two ICEBURN folders currently supply the same portrait artwork.\n\n"
        "The other two CMD files still start one human-versus-CPU match directly.\n"
        "Either form can start the match. Press your mapped Transform input\n"
        "(R3 on a controller) with one blast stock to switch to the other form.\n"
        "Both directions cost one stock.\n"
        "F1 > Warriors opens the first native editor pass: change an imported\n"
        "warrior's name or voice, save, then close and restart to apply it.\n"
        "Saving keeps warrior.txt.editor-backup beside the recipe.\n"
        "Body, parts, moves, forms and new-character creation are not editable yet.\n"
        "Use F1 for input settings. Close the game window to quit; run a CMD again\n"
        "for another match. Report which form and move triggered any issue.\n\n"
        "Bundled script mods are available under F1 > Mods. Enable the ones you\n"
        "want there; this test has its own mod settings and starts them disabled.\n\n"
        "This folder uses private data from this development checkout. Do not\n"
        "upload it or distribute its game data. It is not a normal game update.\n"
        "Your existing installation and its saves are not modified.\n", encoding="utf-8")
    (args.output / "verified-build.json").write_text(json.dumps(record, indent=2) + "\n")
    (args.output / "verified-transforms.json").write_text(json.dumps(transform_build,indent=2)+"\n")
    (args.output / "Build info.txt").write_text(
        "ICEBURN native editor foundation revision\n\n"
        "Includes animation scaling and the bundled script mods.\n"
        "Both forms can transform to each other for one blast stock.\n"
        "Native team picker groups both forms. Fight Again and character\n"
        "reselection verified. Includes native small portrait icons.\n"
        "F1 > Warriors: edit name and voice, save with backup, restart to apply.\n"
        "Full creator, parts, moves and form editing are not implemented yet.\n",encoding="utf-8")
    if args.creator_results:
        artwork=engine/'tbo/ui';artwork.mkdir(parents=True)
        for name in ('zw_bg','zw_emblem','zw_bar','zw_bar_sel','zw_dialog','zw_kk_point','zw_platform','zw_pill','zw_pill_steel'):
            shutil.copy2(args.exe.parent/'tbo/ui'/f'{name}.png',artwork/f'{name}.png')
        helper=(f'set "BT3_WARRIOR_PYTHON={batch_path(args.python_exe)}"\n'
                f'set "BT3_WARRIOR_BUILD_TOOL={batch_path(ROOT / "port/tools/build_warrior_draft.py")}"\n'
                f'set "BT3_WARRIOR_BUILDER_COPY={batch_path(args.builder_copy)}"\n')
        for launcher in args.output.glob('*.cmd'):
            contents=launcher.read_text().replace('setlocal\n','setlocal\n'+helper)
            launcher.write_text(contents,encoding='utf-8')
        main=(args.output/'Play custom roster.cmd').read_text().replace('BT3_TAG_MENU_TEST=roster','BT3_TAG_MENU_TEST=main').replace('logs\\roster.log','logs\\creator.log')
        (args.output/'Play Z Warriors.cmd').write_text(main,encoding='utf-8')
        shutil.copy2(args.creator_results,args.output/'verified-native-creator.json')
        (args.output/'Read me.txt').write_text(
            'LOCAL OFFLINE Z WARRIORS CREATOR PLAYTEST\n\n'
            'Run Play Z Warriors.cmd. Choose Z Warriors after Evolution Z.\n'
            'Choose a warrior, or Create a Warrior. L1/R1 changes tabs.\n'
            'Cross edits; Triangle goes back. Start saves and builds all drafts.\n'
            'Wait for Built and saved before leaving. You can then enter a fight\n'
            'without restarting. The preview shows the last SAVED model.\n'
            'Right stick turns it; L2/R2 zooms. Save parts before choosing their\n'
            'surface colours (last row of Parts). Save again to apply colours.\n'
            'Forms: link another library form and set its blast-stock cost, or\n'
            'Add a Form to clone and link. Current limit: four total custom forms.\n\n'
            'Test a new character, parts, moves, colours, Save, then fight.\n'
            'Check the same character again after reopening the game.\n'
            'ICEBURN remains available with both transformations. R3 transforms\n'
            'when the required blast stock is available.\n\n'
            'F1 > Mods contains your bundled QoL scripts. This folder has its own\n'
            'settings/saves. Enable the scripts you want.\n'
            'The earlier F1 Warriors name/voice editor is a diagnostic interface;\n'
            'use the dedicated game menu for this combined creator test.\n\n'
            'This build uses the prepared local BT3 creator as an authoring\n'
            'helper. Fights still use your modified BT4 engine. Failed builds\n'
            'keep the previously playable files; successful builds keep backups\n'
            'inside the test data .warrior-builds directory.\n'
            'Painting, sculpting, fine numeric tuning and sharing are unfinished.\n'
            'This is a local development build, not a normal updater release.\n'
            'Do not distribute its private game data or builder.\n',encoding='utf-8')
        (args.output/'Build info.txt').write_text(
            'Combined native Z Warriors creator development build\n'
            f'Engine SHA256: {digest}\n'
            'Native menu, library, new creation, parts, attacks/animations,\n'
            'aura/material colours, forms and saved-model preview.\n'
            'Native edit/build/refresh/fight without restart verified.\n'
            'Existing ICEBURN fights, transformations and native picker verified.\n'
            'Uses a private local compiled authoring helper; advanced editing\n'
            'and multiplayer sharing are unfinished.\n',encoding='utf-8')
        if args.live_preview:
            certificates=args.output/'verified-authoring';certificates.mkdir()
            for case,path in authoring.items():shutil.copy2(path,certificates/f'{case}.json')
            shutil.copy2(args.classic_result,args.output/'verified-classic.json')
            (args.output/'Read me.txt').write_text(
                'LOCAL OFFLINE CREATOR PREVIEW PLAYTEST\n\n'
                'Run Play Z Warriors.cmd. Choose Z Warriors after Evolution Z.\n'
                'Your copied library includes ICEBURN and Misakah. This test uses\n'
                'its own character data, settings and saves. The earlier playtest\n'
                'and your regular game installation remain intact.\n\n'
                'HIGHLIGHTED PREVIEWS\n'
                'Choose a body or part: the assembled draft updates automatically,\n'
                'before confirming or saving. New combinations take a short build;\n'
                'wait for Draft preview. Cached choices can appear sooner.\n'
                'Cross confirms a choice; Triangle cancels it.\n\n'
                'REPLACEMENT AND OPTIONAL LAYERS\n'
                'Whole head changes hair, head and face together. Those three\n'
                'pieces remain separately editable. Whole outfit changes the nine\n'
                'body pieces from chest through accessories. Unrelated pieces are\n'
                'retained, including inherited accessories.\n'
                'Extra layer 1-4 deliberately add a part on top. Circle while Hair\n'
                'is selected opens the first empty layer. In a layer picker, use\n'
                'Left/Right for the part and L1/R1 for its source costume. None\n'
                'clears that layer. Normal part selection replaces its own piece.\n\n'
                'SIZING, SAVE AND PLAYTEST\n'
                'Sizing uses 50-180%, in 5% steps; Square restores 100%.\n'
                'Start saves and builds all drafts. Wait for Built and saved.\n'
                'SELECT in the editor playtests the SAVED character in training;\n'
                'save unsaved edits first. Pause, Return to Character Select,\n'
                'and confirm Yes to return to the creator. No restart is needed.\n'
                'For surface colours, save changed geometry first to refresh the\n'
                'available materials, then edit and save the colours.\n\n'
                'REGULAR PLAY AND MODS\n'
                'Saved customs appear in the regular offline versus picker and\n'
                'the custom team roster. ICEBURN retains both transformations.\n'
                'Current prototype limit: FOUR TOTAL custom forms, including\n'
                'transformed forms. Online custom use is still disabled.\n'
                'F1 > Mods contains your QoL scripts. Preferences are local to\n'
                'this folder. This is not an update through the normal updater.\n\n'
                'CHECK THIS REVISION\n'
                'Browse a body, head and chest without saving; check the assembled\n'
                'preview. Cancel a choice. Try an explicit extra layer separately\n'
                'from ordinary replacement. Save, SELECT to playtest, return, and\n'
                'reopen the character. Confirm the regular versus roster too.\n\n'
                'REFERENCE PARITY IS NOT FINISHED\n'
                'Painting, sculpting/posing, portrait photos/cropping, independent\n'
                'costume/damage authoring, full technique/stat/form tuning and\n'
                'sharing still need work. This revision verifies the preview and\n'
                'authoring foundation; it is not the complete reference creator.\n'
                'The prepared private BT3 creator remains a local build helper;\n'
                'fights run in your modified BT4 engine. Failed saves preserve the\n'
                'previous playable files. Close other game windows if those files\n'
                'are in use. Do not distribute private game data or the helper.\n',encoding='utf-8')
            (args.output/'Build info.txt').write_text(
                'Creator preview, extra layers and sizing test candidate\n'
                f'Engine SHA256: {digest}\n'
                'Highlighted assembled previews; separate replacement and optional\n'
                'layers; reference sizing; regular-versus roster; editor training\n'
                'shortcut and return. Native authoring checks and ICEBURN\n'
                'GPU/mod, transform and roster regressions verified.\n'
                'Full reference parity remains unfinished.\n',encoding='utf-8')
            if args.confirmed_preview:
                (args.output/'Read me.txt').write_text(
                    'LOCAL OFFLINE PARTS AND CONFIRMED-PREVIEW UPDATE\n\n'
                    'Run Play Z Warriors.cmd. Choose Z Warriors after Evolution Z.\n'
                    'This build keeps the reference-style part controls and your\n'
                    'copied character library. Full reference parity is unfinished.\n\n'
                    'BROWSE FIRST, THEN APPLY\n'
                    'Scrolling through a picker does not rebuild the model or\n'
                    'start a helper. Cross applies your choice to the unsaved draft.\n'
                    'The assembled preview then updates after one short build.\n'
                    'Wait for Draft preview. Triangle cancels a picker.\n'
                    'Saved designs display directly without rebuilding.\n'
                    'The Python worker uses the installed windowless interpreter;\n'
                    'the reference subprocess also suppresses its console.\n\n'
                    'WHOLE HEAD AND INDIVIDUAL PIECES\n'
                    'Parts > Whole head changes Hair, Head and Face together.\n'
                    'After applying it, all four rows should name the chosen source.\n'
                    'Head by itself changes the skull/ears; Hair and Face remain\n'
                    'separately editable. Other inherited attachments remain as\n'
                    'in the reference. Extra layer 1-4 deliberately add a piece\n'
                    'on top; normal selection replaces its own piece.\n\n'
                    'SAVE AND TEST\n'
                    'Start saves all drafts. Wait for Built and saved. SELECT from\n'
                    'the editor starts a training fight with the saved character.\n'
                    'Pause > Return to Character Select > Yes returns to the creator.\n'
                    'Saved customs also appear in the regular offline versus picker.\n'
                    'ICEBURN keeps both transformations. Your QoL mods are under\n'
                    'F1 > Mods; this folder has its own preferences and saves.\n\n'
                    'CHECK THIS FIX\n'
                    'Open Misakah, Parts, Whole head. Browse several sources first.\n'
                    'Then choose Goku (Early), press Cross, and wait for the preview.\n'
                    'Confirm Hair, Head and Face all show Goku (Early). Save, close\n'
                    'and reopen the character, then SELECT to check the saved look\n'
                    'in a fight. Try individual Hair/Head/Face changes afterward.\n\n'
                    'Only four total custom forms are currently supported. Online\n'
                    'custom use remains disabled. Painting/sculpting/posing, photos,\n'
                    'full costume/damage and advanced tuning/sharing need more work.\n'
                    'This local build still uses the prepared private creator helper;\n'
                    'fights run in your modified BT4 engine. Failed saves keep the\n'
                    'previous playable files. Do not distribute private game data.\n',encoding='utf-8')
                (args.output/'Build info.txt').write_text(
                    'Confirmed-choice preview and complete-head update\n'
                    f'Engine SHA256: {digest}\n'
                    'No rebuilds while browsing. Applied choices update the draft\n'
                    'preview through a windowless worker. Whole-head preview/save/\n'
                    'fight/return verified with identical preview and saved geometry.\n'
                    'ICEBURN mods, transforms and selection regression checked.\n'
                    'Full reference parity remains unfinished.\n',encoding='utf-8')
    if args.attack_controls:
        shutil.copy2(args.attack_label_result,args.output/'verified-attack-labels.json')
        readme=args.output/'Read me.txt'
        readme.write_text('Z WARRIORS ATTACK CONTROLS PLAYTEST\n\n'+readme.read_text(encoding='utf-8'),encoding='utf-8')
        with (args.output/'Read me.txt').open('a',encoding='utf-8') as stream:
            stream.write('\nATTACK NAMES, COLOURS AND VISUAL SOURCES\n'
                'Moves now includes five optional attack names, five attack colour\n'
                'or glow settings, and five separate visual-source choices.\n'
                'B1 means Blast 1, B2 means Blast 2. Names allow 31 UTF-8 bytes;\n'
                'blank restores the original name. Overrides appear in battle\n'
                'captions, the pause skill list and the QoL skill-list display.\n'
                'Attack look/effects controls are separate from attack behavior.\n'
                'Use Left/Right in the source picker to select its technique slot.\n'
                'Blast 1 sources have two slots; other techniques have three.\n'
                'Borrow visuals from a compatible technique kind. Different\n'
                'fighters may share identical effects. Not every technique has\n'
                'a visible colour/glow component.\n\n'
                'CHECK THIS UPDATE\n'
                'Change a technique name, use a bright colour for a beam, and\n'
                'choose a distinct compatible visual source. Save and test the\n'
                'actual attacks. Check the pause and QoL lists for the name.\n'
                'Damage controls and advanced fine tuning are deferred. This\n'
                'playtest still needs the prepared external creator helper.\n')
        with (args.output/'Build info.txt').open('a',encoding='utf-8') as stream:
            stream.write('Includes per-attack name, colour/glow and visual-source controls.\n'
                'Runtime caption/skill-list integration included. Helper removal pending.\n')
    print(args.output.resolve())


if __name__ == "__main__":
    main()
