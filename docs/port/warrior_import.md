# Experimental warrior import foundation

This branch begins support for generated Z Warriors model packs in the native
BT4 profile. The current prototype discovers generated warrior folders and maps
up to four forms to additional offline character identities. It does not yet
provide the creator or fused gameplay. The native offline team picker groups
connected imported forms under an additional roster entry. Basic named form
changes are available offline through the normal transformation input.

## Folder resource routing

With the experimental switch enabled, `plat_warriors.c` reads the data folder's
`mods/enabled.txt` and each listed folder's `.warrior_built`. It registers complete
generated resource sets only: one to four intact/damaged costume pairs, animation and effects packs,
two member records, and the portrait. Invalid, duplicate or incomplete lists
reject the entire set. Recipes are not executed or rebuilt.

The host file loader can read these resources through the internal namespace
`zwarriors/<import-index>/<resource-role>.bin`. Import indices are **not** game
character ids. A missing warrior resource never falls back to a disc file. The
disc profile maps prototype character ids 250..253 to these routes, resolves
style and voice donors by name against the actual disc roster, and extends the
gauge table using the selected style donor. The generated model's own combat
tables remain embedded. The roster scanner can inspect 32 folders, but the first
playable prototype accepts at most four forms to stay below existing byte-sized
character markers. That is a prototype restriction, not the final capacity.

Named form dependencies are resolved during discovery. The first supported
transformation family has a blast-stock cost (0..5), standard sequence 0, basic
change kind 2, no prop override (-1), and damage preservation (0). Other numeric
families remain unavailable. Targets must resolve to another registered playable
custom form. Neutral transformation input chooses the first supported link.
Imported transformations use blast stocks; native BT4 fighters keep their Ki
rules. The headless demo override can launch each custom form separately. No
stock fighter is replaced. Fusion links remain disabled.

## Native team selection

The offline team picker appends one entry per connected component of supported
form links, in enabled-folder order. Each group's forms use the native form
reel. The stock grid remains intact; additions reuse trailing filler cells and
the padded output stays within the native 165-cell buffer. The current four-form
prototype fits, but this is not an unlimited-roster implementation. If a grid
has insufficient capacity, it is left unchanged rather than partially written.

Large portraits load the supplied generated portrait resource, and the selected
warrior's name is drawn without indexing the stock text tables. Small grid and
team icons currently use the style donor as a placeholder. These additions are
specific to the team picker, including local tag battles; the other game modes'
standalone selection screens have not been extended. Online custom imports
remain blocked.

`Play custom roster.cmd` enters the native team setup. From the initial fighter
cell, Up then Right reaches the appended ICEBURN group in the final row. The two
direct-to-fight launchers remain available. Saves, settings and mod preferences
are local to the test folder.

## Static inspection

`port/tools/inspect_warrior.py` reads a warrior directory or a ZIP containing one
warrior directory. It checks the generated resource list, describes pack tables,
and reports unresolved transformation folder references across the supplied set.
It neither extracts archives nor modifies the game installation.

Use `--output` with a private location under `port/build/` for reports from real
characters. Reports contain names and resource hashes and must not be published
with game assets, recipes, saves, or logs. The synthetic unit tests need no assets.

## Embedded model metadata

BT4 normally replaces the model pack's embedded character parameters, AI, lip
tables, and combat tables with its separate metadata resources. Generated
BT3-layout packs carry those tables themselves. With the developer-only switch
`BT3_WARRIOR_MODELS=1`, model binding retains the embedded tables for validated
252-entry packs. The pack table must fit in the file, offsets must address its
payload, and its first parameter entry must contain at least 256 bytes.

The raw engine switch defaults off; the installed launcher enables it after
Setup or Update prepares the local BT4 creator cache. Ordinary BT4 models
continue through the existing metadata path. Native online scope hides the
local custom roster, and every incoming fighter/loadout check accepts only disc
fighters. The creator can therefore remain available offline while ordinary
fighters enter native online rooms. Custom-character networking is not enabled.

Generated packs may also carry a `BT3WARRI` extension in entry `0x1A`: a
default animation translation ratio and eight-byte overrides containing a first
motion, motion count and ratio. The experimental decoder applies the selected
ratio to translation keys in the freshly decompressed animation buffer. Rotation
keys, timing, events and source files are preserved. This accounts for borrowed
motions authored for different body proportions, rather than adding a fixed
world-space height offset. Extension bounds, motion ranges and ratios are checked;
shared translation tracks are scaled once. Native fighters bypass this path.

## Validation

```text
python -m unittest discover -s port/tests -p test_inspect_warrior.py -v
cc -Wall -Wextra -Werror -fsanitize=address,undefined -g -Iport/include \
  port/tests/warrior_model.c port/src/plat_game.c port/src/plat_warriors.c -o warrior_model_test
./warrior_model_test
```

The C harness can additionally receive private model-file paths. It checks the
opt-in decision for each file without running game code. A passing result does
not establish rendering, combat, transformation, save, or network behavior.

`port/tests/warrior_runtime.py` exercises the actual experimental executable and
keeps its logs, content hashes and GPU captures private. The local-only playtest
preparer requires successful results for both forms, unchanged input data and
explicit visual review. Its launchers refer to this development checkout's data;
they are not redistributable installers or normal updater releases.
Packaging also requires successful round trips starting from both forms, tied
to the same executable and exact warrior inputs. The opt-in runtime fixture
checks insufficient gauge rejection, both resource changes, health preservation,
cost deduction and return to idle with the cinematic camera released.
The playtest includes the normal bundled script mods with a separate enable and
settings file; enable them under F1 > Mods. It does not import personal mod
preferences from another installation.
`port/tests/warrior_selection_runtime.py` drives the real native menu with pad
input: each form, equipment/outfit, CPU opponent and stage, then gameplay. It
also exercises character reselection, Fight Again with the retained form, and
the direct team-setup launcher. Packaging requires matching successful records
for those paths, in addition to the gameplay and transformation checks.

Next steps are interactive team-picker checks, custom icon presentation,
broader manifest-family support, creator/editor integration, technique and
HD-texture validation, and eventually network validation.

## Creator reference and remaining limits

The earlier F1 Warriors editor remains a name-and-voice diagnostic interface.
The combined development build adds a controller-driven Z Warriors entry after
Evolution Z. Its library supports new warriors, parts and costume sources,
move and attack sources, aura and material colours, and linked forms. Start
builds the drafts and refreshes the custom registry before the next fight,
without restarting the game. Highlighted body and part choices request a
temporary assembled model without saving, enabling, or publishing a character.
The viewer keeps the previous model while a new combination builds; cached
choices can be reused. This local-helper bridge has build latency and is not an
instant native mesh composer. Stale preview results cannot replace the latest
highlight, and preview slot bindings are restored before leaving the editor.
The Head control groups hair, head mesh and face; those parts are also editable
separately. Other inherited pieces are retained. Four custom forms
are supported in this prototype. This is not full parity with the reference:
painting, sculpting, detailed numeric tuning and sharing are not implemented.

The native controls use `build_warrior_draft.py` and an isolated copy of the
installed creator executable as a local authoring helper. Configure
`BT3_WARRIOR_PYTHON`, `BT3_WARRIOR_BUILD_TOOL` and
`BT3_WARRIOR_BUILDER_COPY` to enable saving. The copy must have the preparation
marker and the expected executable hash. This development bridge requires
private BT3 builder data; it is not a redistributable standalone builder.
BT4 still runs every fight in the existing integration engine.

The worker isolates temporary names, resolves form dependencies, validates
generated resources and model buffer bounds, then publishes the whole family
with rollback. It rejects externally changed recipes and restores the helper's
enabled list. Failed builds leave the existing playable family intact. Voice
and fighting-style choices must resolve against the actual BT4 roster; mesh
and animation sources use the helper's BT3 catalogue. Generated material colour
keys come from texture pixel identities, which stay stable across palette edits.
Changed meshes must be built before choosing their new surface colours.

The development editor also provides SELECT to playtest a saved character in
an offline training fight. Unsaved drafts must be saved first. It restores the
permanent resource bindings before setting up the fight and returns to the
creator when the playtest ends.

The working BT3 Z Warriors installation is a behavioral reference, not the
engine or game-data base for this BT4 integration. Experiments use an isolated
copy. The first controlled save of an unchanged default warrior produced a
short recipe (`name`, `title`, `body`, `moves`) and eleven generated resources:
three costumes, three damaged costumes, animation, effects, portrait, and two
member files. The importer accepts one to four contiguous intact/damaged
costume pairs; incomplete pairs and gaps reject registration. Existing ICEBURN
virtual resource identities remain unchanged. Omitted recipe style inherits
`moves`, then `body`; omitted voice inherits `body`, matching the default save.
These names must still resolve against the BT4 roster. A runnable editor and saved
output provide reproducible examples; they do not provide the builder's source.

Optional `chip.bin` resources provide small native roster icons. They use the
verified 64-by-64 indexed texture layout and preserve the native silhouette.
The local conversion tool derives artwork from the supplied portrait and a
disc chip template. Identical supplied portraits produce identical icons;
this does not synthesize different artwork for transformed forms. Missing or
invalid chips fall back to the existing donor artwork. The nine required
ICEBURN resources are unchanged. Native file reads round up to sectors, so chip
buffers include sector padding rather than allocating only the file length.

## Eventual multiplayer character exchange

The desired multiplayer flow includes peers who do not already have each
other's custom fighters. After offline imports and matching-content play are
proven, the lobby should exchange the selected warriors' recipes and all form
dependencies, build them locally, and verify the resulting gameplay identity
before starting a match. Each peer must use the same agreed mapping from custom
warrior identity to session character slot, regardless of local library order.

Recipe-only exchange is a hypothesis until the builder and share-code format
are understood. Painted textures, sculpted parts or external assets may require
additional bounded content exchange. Received data must be validated as data;
the protocol must not install or execute code. A missing dependency, unsupported
builder version or gameplay mismatch must prevent match start with an actionable
error. Remote warriors belong to a temporary session library, not the player's
permanent custom roster. They remain available for fights and rematches while
connected, then are removed when the session ends. Interrupted sessions need
cleanup of abandoned temporary data on the next launch. The player's own
warriors are never removed by that cleanup. Any future permanent import of a
remote warrior would require an explicit separate user action. Both peers must
load the remote model and gameplay data; displaying only its appearance is not
enough to simulate the same fight. This exchange is not implemented;
the current experimental mode remains offline-only.
