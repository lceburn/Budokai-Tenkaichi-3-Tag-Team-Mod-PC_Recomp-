# Z Warriors reference parity audit

This is a working inventory, not a declaration of feature parity. The user
requires the installed reference creator's complete options and behavior.
The local reference is the isolated copy of the working BT3 creator; BT4 keeps
the existing integration engine and QoL changes. No original installation is
modified by this research.

Evidence comes from the installed executable's authoring strings, its
`Zw_PartWho` grouped-part resolver, controlled builder outputs, and native BT4
controller tests. A reference tooltip establishes an advertised option, not a
verified end-to-end implementation. Every option still needs a behavior check.
Private evidence is under `port/build/zwarriors-research` and the native test
directories; reference executable, artwork and game resources are not tracked.

| Reference option | Current BT4 implementation / verification | Remaining work |
| --- | --- | --- |
| Native creator entry, character library | Controller menu and persistent library tested | Match full library operations and reference navigation |
| Name and title | Recipe editing provided; name save and fight tested | Title-specific persistence/display check |
| Base body | Source selection and temporary draft preview wired | Visual browsing test across body families and costume choices |
| Complete head | Virtual control sets hair, head mesh and face together; controller save/fight passed | Compare exact reference behavior across donor combinations |
| Complete outfit | Reference groups nine pieces from chest through accessories; matching virtual control written | Visual and save/reload check |
| Individual hair, head/ears, face, chest, waist, upper arms, forearms, hands, hips/thighs, shins, feet, accessories | Source controls wired; complete-head unsaved preview captured | Per-control comparison, cancel/default/none and costume behavior tests |
| Extra part layers, including another hairstyle or scouter | Four optional source/part/costume controls and hair layering shortcut; highlighted preview and save verified without replacing existing head/hair/face | Broader per-layer/part/costume fight coverage |
| Head, hair, hands, feet, arms, chest and leg sizing | Seven controls use the recovered 50-180% bounds, 5% steps and 100% default; 120% head preview/save/fight/return verified | Per-control visual and boundary coverage |
| Random parts and reset parts/sizes/layers | Advertised by reference; not implemented here | Add controls with equivalent scope and discard behavior |
| Voice | Save and actual fight verified | Compare defaults and costume/form inheritance |
| Surface colour editing | Generated material identities recovered; saved colour preview and fight verified | Match reference picker controls and live colour adjustments |
| Painted textures and imported pictures | Existing assets copied losslessly; no authoring UI | Painting, picture import and asset validation |
| Pose editing | Reference supports posing bones while retaining breathing | Implement matching pose controls and generated representation |
| Sculpt/stretch/move/hide pieces and hole patching | Advertised by reference; not implemented here | Recover and validate complete geometry-edit representation |
| Shadow colours by part or texture; borrowed shading/glow | Advertised by reference; not implemented here | Match selectors, material bindings, strength and rendered output |
| Large portrait, small grid chip, battle HUD portrait | Source portrait and generated chips supported | Photo/crop/custom-picture authoring and matching preview/export |
| Character-select / test-fight option | Regular versus roster fix visually verified; saved-character training fight captured and return to library verified | Compare reference's two actions and add direct select-screen shortcut |
| Four independently authored costumes | Imported generated costume sets supported; donor costume fields provided | Match enabled switch, current costume, per-costume editing and reset-from-first |
| Damage appearance | Generated damaged models load | Add reference damage modes, painted wear and custom-warrior appearance sources |
| Borrow appearance from another created warrior | Advertised by reference; not implemented here | Implement dependencies, cycle handling and costume/form rules |
| Whole move set, ki/effects, finishers and attacks | Builder-to-fight path tested with changed moves/attacks | Per-technique reference comparison, inheritance and error behavior |
| Attack visual effects independent of behavior | Five independent source/technique-slot controls implemented; distinct Scouter effect resource change and native save/fight/return verified | Broader compatible-kind and actual attack appearance coverage |
| Technique names and colours | Five name and five colour/glow overrides; native preview/save checked, custom label visually verified in pause and QoL skill lists; battle-caption hook implemented | Actual attack-caption and per-technique colour/glow visual playtests |
| Movement, charge, melee, ki, grab, guard, intro, victory and taunt animations | Source selectors provided | Verify each reference-supported category and semantic-only swaps |
| Aura source/colour, light and ki trails | Aura source/colour controls provided | Check reference modes, strengths, glow and live display |
| Health/stats/ki-rate donor and numeric overrides | Style donor provided | Expose and test remaining stat overrides |
| Stance / battle idle | Donor selector provided | Compare default, pose interaction and scaling |
| Technique cameras | Donor selectors provided | Verify each opening/charge/loop camera control and restrictions |
| Technique behavior copy | Advertised by reference; not implemented here | Match permitted technique kinds and rush exclusions |
| Technique fine tuning: damage, hits, size, speed, unblockable and other settings | Advertised by reference; not implemented here | Enumerate every option, range and generated field; verify gameplay |
| Forms, return forms, costs, transform inputs, aura and glow | Linking and bidirectional saved transformations tested; simple stock cost controls provided | Match all reference transformation settings; test each input and direction |
| Collapsible editor sections | Advertised by reference; not implemented here | Match navigation and saved expansion behavior where relevant |
| Create / duplicate / delete / sharing and other library actions | New creation and cloned linked forms implemented | Enumerate full reference library and share-code options through UI |
| Highlighted assembled preview | Body, head, chest, optional layer and head-size previews captured before saving; body cancellation leaves saved library unchanged; accept/save/fight/return passes | Broader combinations, responsiveness and additional invalid/stale-result cases; current helper introduces build latency |
| Save / reload / rollback | Atomic family publish, external-edit rejection and failed-publish rollback tested | Full authoring-feature persistence and reference-equivalent recovery checks |

## Required behavior distinctions

Normal part replacement and explicit extra layers are separate actions. The
complete-head control changes three related sources; it does not silently
delete all other accessories. The helmet seen on Misakah has not yet been
assigned to a verified part category. Compare that exact combination in the
reference rather than assuming that every visible overlap is an importer bug.

Confirmed previews must show the assembled draft, retaining unrelated choices,
without publishing or enabling it. They must not show a standalone donor as a
substitute. Cancelling restores the previous draft, and leaving the editor
restores permanent slot/resource bindings before any battle or roster screen.
The user accepted preview-on-confirmation after testing the helper's latency.
Scrolling a source list must not launch a build. Cross applies the choice and
requests one temporary assembled preview. Saved designs display directly. The
confirmed-choice full-head save/fight/return path and browsing without a helper
launch have passed; the user also reports that this revision passed their test.
Instant reference-like composition is not yet established.

Saved custom characters belong to the local test library. Offline roster and
fight checks are required separately for each supported selection screen.
Online custom use remains disabled until peers can load and verify equivalent
content. Future remote character exchange is session-only unless explicitly
imported into a permanent library.

## Completion standard

Do not label the creator complete or equivalent based on a successful save or
one head/attack example. Enumerate the reference UI's remaining options, match
their meaning and interactions, then validate preview, confirm/cancel, save,
reload and gameplay where applicable. Keep the existing playable package as a
baseline until the candidate's required checks pass. The local preview/layers/
sizing candidate has passed its package smoke check. It remains an intermediate
authoring build, not complete reference parity.

## Current delivery scope

The latest user direction requires the complete reference editor to work
without the separate helper, including aura, texture/colour and part edits.
The earlier narrowed attack-update scope is superseded. All remaining reference
settings stay open in the inventory above. GitHub remains explicitly on hold.
Blast 1 and Blast 2 source lists must display actual attack names, with the
fighter retained as source information, rather than character names alone.

The user requested every reference authoring setting, including colour editing,
attack controls, auras, painting, sculpting, posing, costumes and transformations.
The latest scope clarification defers damage amounts and combat-stat/fine-tuning
controls. It does not defer damage appearance, texture authoring or the other
creative controls. Existing basic selectors do not establish complete support.
Keep those categories open until behavior, persistence and rendered/gameplay
checks pass.

Selected attacks keep their normal damage and combat values; custom numeric
damage boosts are outside this delivery. Match the installed creator's existing
creative behavior rather than adding features or fixing cosmetic gaps it also
leaves. Compare generated native output against controlled reference output.

The next delivery must support testing on another computer without an installed
BT3 creator or a development-machine helper path. Test create, confirmed preview,
save, reload, fight and form changes with helper fallback unavailable. Existing
characters merely loading is insufficient. Source resources must come from the
user's local game-data setup rather than private development paths. GitHub stays
on hold until the user reviews the portable candidate.

The user explicitly confirmed that bundling the old BT3 creator is not the end
goal: installation of their BT4 build and their own BT4 game files must suffice.
Remove both executable/helper dependencies and the native source catalogue's
current dependence on extracted BT3 resources. A packaged old creator, or native
code using a privately prebuilt BT3 cache, does not meet this completion standard.

The attack candidate adds the reference's five optional technique-name overrides
and five technique-colour overrides. Names accept at most 31 UTF-8 bytes; empty
names restore the original label. Compilation, controller-driven edit/preview/
save of the first attack's name and colour, and a GPU fight load with QoL scripts
have passed. Other saved characters remained unchanged. Per-attack rendered
effects and label behavior still require checks. These controls are not yet
published into the user's playable package.

The candidate also exposes `blast1_1fx`, `blast1_2fx`,
`blast2_1fx`, `blast2_2fx`, and `ultimatefx`. The installed reader confirms
fighter-name plus colon and technique slot syntax. Blast 1 sources have two
slots; the other technique sources have three. These visual-source controls are
separate from the behavior selectors. A distinct Vegeta (Scouter) source changes
the effect resource and passes editor preview/save/training/return. The initial
Goku sample yielded identical effect data. Broad compatibility across technique
kinds and the visible effect during actual use still need playtest coverage.

Native label loading now validates optional names per saved warrior. Stock
fighters and unset entries retain their script text. The pause special-attack
list and the QoL skill-list display have been visually checked with an override;
the battle-caption path is connected but its actual displayed caption remains
a playtest check. Twenty-three synthetic routing tests pass, including duplicate
and overlong attack-label rejection and ordinary resource routing.

GitHub publication is explicitly paused. Report the finished creator update to
the user before any push. The subsequent milestone is removing the separate
BT3 authoring helper by implementing the builder in the native game. Native
metadata, material colours and selected model-assembly paths have been tested;
see warrior_native_builder.md for the precise coverage. Full replacement is
unfinished. Packaging or hiding the helper does not satisfy that requirement.
Already-generated characters can run in BT4 without the authoring helper;
unsupported creation and rebuild recipes still require it.
