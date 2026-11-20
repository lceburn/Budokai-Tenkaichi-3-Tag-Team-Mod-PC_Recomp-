# Native character builder milestone

The current creator uses the installed reference executable as a local build
bridge. BT4 menus, preview rendering, roster selection, and fights run in this
engine. Removing the bridge means implementing resource generation here;
renaming, hiding, or bundling the reference executable does not meet the goal.

The user requested this milestone after testing the attack-control update and
before GitHub publication. Advanced creator parity and HD texture-pack handling
are deferred. The last accepted playable build remains the regression baseline.

## Backend contract

The editor supplies an immutable recipe, its local paint/photo assets, and a
versioned donor-resource catalogue. A game-owned worker produces a temporary
validated resource set. Preview results never enable or publish a character.
Save validates a complete form family before atomic publication, rejecting
external edits and preserving the previous playable files on failure.

Keep the existing model-buffer limit, source bounds checks, path validation,
per-process preview binding, stale-result rejection, and form-slot constraints.
Keep ordinary roster and battle behavior as regression checks.

## Implementation sequence

1. Audit which current settings affect recipe/runtime metadata, model packs,
   motion packs, effects, member data, portraits, and generated manifests.
   Establish precise donor inputs for each output; a recipe field alone is not
   proof of a rendered or gameplay change.
2. Implement metadata-only saves and native texture/material colour edits,
   with no subprocesses. Preserve unknown recipe fields and untouched assets.
3. Implement donor resource assembly for moves, attacks, effects, animation
   categories, and transformation metadata. Match the current imported motion
   scaling and BT4 donor binding rules.
4. Implement body/part assembly, skeleton mapping, optional layers, sizing,
   costume/damage variants, and portrait generation for currently supported
   controls. Compare output behavior against disposable reference builds.
5. Connect the backend to asynchronous preview and family-save transactions.
   Remove helper/interpreter configuration only when every supported authoring
   path has a native implementation.

The current donor catalogue uses reference BT3 names, while battle support data
comes from BT4. Native generation needs an explicit asset/catalogue mapping;
an absent donor must produce a clear unsupported-source error rather than a
silent substitution. The existing development install can supply comparison
inputs without changing the user's working installations. Requirements for a
clean-machine data setup still need to be established.

## Completion checks

- Run creation, part edits, colour edits, attack/effect edits, linked forms,
  preview, save, reload, and fights with the helper executable unavailable and
  all helper/interpreter environment settings removed.
- Observe no external authoring process launches.
- Exercise failure rollback, cancellation, stale previews, and external edits.
- Verify the supported creator paths on another computer with the documented
  game-data setup and the existing QoL mods.
- Report the result to the user before any GitHub push. Keep proprietary
  reference binaries and extracted game data outside source publication.

## Development progress

The attack-control playtest passed the user's test. The user requested clearer
attack names in the donor picker (it currently lists fighter names), but deferred
that UI improvement until after helper removal and the GitHub update.

The first native backend edits existing generated warriors; it is not yet a
complete character assembler. It includes:

- Native name/title/attack-label publication with backups, rollback, and
  external-edit protection. An editor-save-fight-return test passed with all
  helper settings absent, no child processes, and unchanged generated resources.
- A checked resource-pack reader/writer. Voice lip-track transplantation and
  the style donor's 48-byte stat range reproduce controlled reference model
  output byte for byte. Voice/style changes cannot be treated as recipe-only
  edits: the reference changes model resources too.
- A local donor-cache importer that reads extracted BT3 data without executing
  BT3. The cache contains 161 first-costume models and 2,093 original model,
  motion, effect, portrait, and member resources, with size and FNV-1a integrity
  checks. These private assets are excluded from Git.
- Native voice/stat publication passed isolated editor-save-fight-return tests
  without helper settings or child processes. All generated intact/damaged
  model variants match controlled reference output; unrelated resources remain
  unchanged. Unsupported edits still use the bridge during migration.
- Native material catalogues match the existing Python decoder for all 161
  donors. Native material recolouring now includes HSV palette changes,
  material-linked shading uploads, and facial-expression palette propagation.
  Six controlled colour cases match the reference across six costume/damage
  variants, including saturated, grey, white, black, and near-black targets.
  A native stock colour edit passed editor-save-fight-return with no helper
  configuration or child processes; unrelated warriors/resources stayed intact.
- A restricted default Goku (End) assembler reproduces 14 reference outputs:
  six costume/damage models, motions, effects, portrait, member data, and
  manifests. Binary resources match exactly; text manifests have equivalent
  content with normalized line endings. Animation scaling derives from skeleton
  heights, including costume differences. This is not general part assembly.
- New-folder activation passes rollback and changed-library tests. The default
  creation path passed an isolated Windows create-preview-save-fight-return
  test with no helper settings and no child processes. All eight generated
  model/motion/effect packs match the reference; all three existing warriors
  stayed unchanged. The captured native preview also renders the new warrior.
  This candidate is separate from the user's accepted playable package.
- Native edits to existing generated form links match all four reference
  records. Removing a link restores the default record; invalid/self/missing
  targets are rejected. The editor-save-fight-return test and both ICEBURN
  transformation round trips pass without helper settings. Creation of a new
  linked form family still needs a native family-generation transaction.
- Independent attack-effect overrides now have native resource transplantation
  and removal. Controlled comparisons pass for all five slots, including
  cross-slot donors. Invalid donors fail before publication. The native editor
  save/fight/return test also passed with no helper settings or child processes;
  the selected donor effect changed while other resources/warriors stayed
  unchanged. Full technique/animation assembly and attack colour changes are
  separate unfinished paths.
- Native mesh hierarchy reading now covers all 161 donor models: 9,873 records,
  at most 72 per model. It validates record extents, parent depth, termination,
  and the reference's bone-to-part slot mapping. Malformed records leave the
  previous result untouched. Mesh/texture assembly is still pending; this
  reader alone does not implement a head or clothing replacement.
- Whole-head planning now selects donor anchor records 46/47/48 while keeping
  the body hierarchy and auxiliary expression records. Its controlled Goku
  comparison matches the reference's selected record sizes and preserves both
  input meshes; missing/ambiguous anchors fail without replacing the plan.
  Record-size agreement is not geometry equivalence. Vertex fitting, auxiliary
  resources, texture packing/remapping, and emitted mesh topology remain pending.
- Native texture relocation now matches the reference material addresses across
  50 non-head records while preserving every other input byte. The comparison
  treats drawing bounds/hierarchy rebuilds separately; the reference also
  canonicalizes some signed-zero vertex floats. Auxiliary head materials need
  substitution rules beyond identity-based body relocation.
- Native combined-texture packing produces the same 21 texture contents as the
  controlled whole-head reference (allocation/order need not be identical).
  Only required donor textures are added; duplicate contents are reused. Native
  rebinding also reproduces all five reference mouth drawing streams exactly.
  DMA bounds, missing/ambiguous destinations, format limits, and failed-output
  preservation are tested with sanitizers. These components are not yet joined
  into a complete runtime model assembler.
- An experimental whole-head assembler now emits a complete model for the
  controlled Goku (End) / Goku (Early) case. It combines selected donor records,
  native packed textures and rebound facial streams while preserving body
  technique/voice/motion metadata. Isolated GPU loading, a front-facing preview,
  and a short AI fight passed; the fighter changed actions and took damage.
- `BT3_WARRIOR_NATIVE_HEAD_TEST=1` enables only that narrow recipe in development.
  Its native preview-save-fight-return test passed with no helper settings or
  child processes, rebuilding all six costume/damage variants and keeping the
  other three warriors unchanged. This is off by default and has not replaced
  the user-tested package. It does not establish general head-edit compatibility.
- A separate Chi-Chi/Goku reference probe confirms that cross-body assembly must
  keep the base bind coordinates. Face vertices match a head-anchor translation,
  while the neck requires more than a uniform translation/scale. General fitting,
  head auxiliary shading substitutions, and chain reconstruction remain under
  investigation before expanding the supported native recipe set.
- Native face-anchor fitting is implemented independently of neck stitching.
  Its Chi-Chi/Goku test compares all 416 face vertices with the reference;
  maximum coordinate error is 1.19209e-7. Other geometry and vertex attributes
  remain unchanged. Reapplying the same fit is idempotent, and invalid bind data
  fails without modifying the mesh. This does not yet enable the full cross-body
  head recipe: neck fitting, seam stitching and remaining head data still need
  implementation and runtime validation.

- Shared native fitting now measures bone axes, lengths and perpendicular
  bounds, selects the reference's limb/finger bone pairs, and calculates its
  separate longitudinal and width scales. Sanitizer checks cover all 161 donor
  models and 9,873 bounds, identity fits, controlled length/width changes and
  failure preservation. This is independent of seam stitching and is not yet
  connected to the experimental model assembler.
- Acceptance follows the installed reference's behavior. Small gaps also present
  in the reference do not require additional cosmetic correction before helper
  removal; new distortions or failed replacements still need correction.
- Native seam boundary discovery now reads 796,308 vertices across all donor
  models. Controlled geometry tests cover shared-edge exclusion, nearest boundary
  adjustment, bounded interior falloff, sibling preservation and failed-output
  preservation under sanitizers. The seam adjustment is implemented separately;
  matching cross-body reference output and connecting it to assembly remain.
- Cross-body anchor fitting and neck stitching are now connected to the
  experimental head assembler. The Chi-Chi/Goku reference comparison passes for
  all 44 neck vertices (maximum error 9.53674e-7) and 416 face vertices (maximum
  error 1.19209e-7). The neck uses its actual drawing parent, chest node 17 in
  this fixture. Assuming torso node 16 prevented the reference stitch from
  finding the matching boundary.
- Combined textures now canonicalize identical retained body uploads before
  assigning addresses. This fixes ambiguous texture destinations in the
  Chi-Chi fixture without weakening the ambiguity checks. Existing Goku head
  and 21-texture reference regressions pass. The expanded assembler still needs
  Windows runtime validation; arbitrary head recipes remain disabled.
- The first updated Windows candidate passed the native Goku head editor
  preview-save-fight-return test with no child processes and other warriors
  unchanged. A cross-body runtime probe exposed missing moves-donor effect
  resources; native common technique transfer now reproduces 32 reference model
  entries while preserving body geometry, standing data, chains and scaling.
- Native default motion mixing reproduces all 440 entries and the complete
  packed reference bytes: six stance motions remain from the body, the rest
  follow the moves donor. Explicit animation-group overrides are not handled.
- Repacked models must update both the normal texture range and the shading
  range's start index. A GPU capture exposed corruption from keeping the old
  shading start; correcting it removed the stage/texture corruption in the next
  isolated capture. Runtime completion alone does not prove rendering is right.
- Under the same development flag, the generator now accepts the specific
  Chi-Chi-body/Goku-End-moves/Goku-Early-head recipe. Four costume/damage model
  variants match the reference's technique, voice and scaling entries, and five
  supporting resources match exactly. Body portraits retain their contents under
  the moves template's routed filenames. All four generated variants passed
  Windows fights and a saved front-preview check; these are private probes.
  The original stock generator still matches all 14 published reference outputs.
  The latest generator changes still need an updated Windows editor rebuild test.
- The updated cross-body candidate passed native colour preview-save-fight-return
  without helper processes, preserving other warriors. The selected surface
  changes only the first intact costume in both the installed reference and
  native output; tests use that observed applicability instead of assuming all
  costume and damage variants share the surface.
- Native hand fitting and assembly match the tested Kid Goku donor across all
  six Goku (End) costume/damage variants. The generator follows matching donor
  costumes, falling back to the first when a slot is empty. All six generated
  variants passed Windows fights. The separately gated Windows hand editor also
  passed preview, save, fight and return with no helper process.
- Assembly now retains and relocates the base mesh's trailing CPU-skinned
  section, including its array offsets and material descriptors. This fixes the
  second-costume runtime crash caused by truncating that section. The controlled
  test preserves 48 reference vertices and 16 triangles and rejects bad offsets
  and indices without changing output.
- A shared torso/limb fitter and selected-record assembler now match controlled
  reference chest, waist, upper-arm, forearm, hip, shin and foot samples, including
  normals and bind coordinates. All seven samples passed six costume/damage
  fights each. Gated chest authoring passed the Windows preview/save/fight/return
  test without a helper, preserving the other saved characters.
- Combined arm/hand and outfit samples match the installed reference across six
  variants each, including geometry, normals, binds and retained CPU skin data.
  Their generated models passed Windows fights. Each donor fits against the
  original body; boundary joins run after all selected parts are assembled.
  The combined Windows editor passed a forearm-plus-hand preview, save, fight
  and return test with no helper process and the other saved characters intact.
  Arbitrary donors, independent head pieces and extra layers remain unverified.

Native transaction, pack, and source-cache tests include sanitizers, malformed
inputs, rollback after partial publication, concurrent-edit protection, model
size limits, missing/changed source assets, and catalogue validation.

The last user-tested native-parts package remains unchanged. Full helper-free part
assembly, moves/attack generation, attack/aura colours, arbitrary body recipes,
and new form-family generation are not complete. Material recolouring currently
uses the native stock assembly path; editing colours on arbitrary mixed-part
recipes still requires native part assembly. Do not label the helper-removal
milestone achieved or publish the release based on these partial paths.

## BT4-only source preparation and recent native checks

The end goal explicitly excludes both the old creator executable and BT3 source
data. The existing development cache was prepared from BT3; using it from native
code alone does not establish a portable BT4-only creator.

An experimental importer now reconstructs embedded donor models from the
player's extracted BT4 B14 resources: original model sections, separate fighter
metadata, AI, current disc-language technique text and both lip tables. It runs
no game executable. The current probe covers Goku (End) and at most four
costumes, not the complete BT4 roster. BT4-only generation has passed Windows
fights and an in-game preview/save/fight/return check. A separate empty-library
test created a character, applied mixed forearms and hands, saved it, fought,
and returned without helper processes. These are sample checks, not full parity.

BT4 has real data in entry 26, which the earlier native prototype used for
animation scaling. The extended container preserves that data and appends a
validated scaling record. BT4-origin records also identify the disc's single
technique script and retain BT4 combat-rule interpretation. Legacy imported
characters retain their earlier behavior. Synthetic metadata/routing checks
cover invalid extensions, per-character rule selection and duplicate rejection.
Model buffer limits remain enforced. One disc-language script is stored rather
than five duplicate copies, matching ordinary BT4's external script behavior.

Nine animation categories now reproduce controlled reference outputs across six
costume/damage variants: movement, charge, ki, grab, guard, intro, victory, taunt
and stance. Standing translations differ by at most floating-point rounding;
other bytes match. Native movement editing also passed controller preview, save,
fight and return with no helper. Melee semantic swapping remains unsupported.

Independent effect swaps, resource tinting and aura preset transfer have native
reference checks. The selected aura RGB reaches runtime particle data, but its
large visible glow is still incorrect; do not call aura colour visually verified.
Actual five-slot technique names are now loaded from source scripts and shown in
Blast 1/Blast 2 source lists, alongside source fighters.

Additional Piccolo (End) forearm and Android 18 hip samples match the reference
across six variants each. Their combined Windows editor preview/save/fight/return
passed without helper processes. These remain controlled donor combinations;
another chest sample differs, and arbitrary part support is not established.

BT4-only linked-form creation and transformation roundtrips also pass controlled
Windows tests. Mixed-material parts now handle independently bound palettes,
texture-atlas offsets and compatible split bone records. Generated model size is
bounded by a shared 2 MiB limit used by both generation and runtime allocation;
the ordinary non-creator allocation remains unchanged.

Whole move-set generation now matches the installed reference for Vegeta
(Scouter) and Nappa across all six legacy costume/damage variants. These checks
also caught body-attachment parameter preservation and alternate-costume scaling
errors. Existing-character regeneration maps new resources back to the saved
manifest's filenames, so changing move donors does not break resource routing.
Incompatible costume layouts or portrait/member replacements fail before any
publication. The corrected Windows candidate passed a BT4-only controller test
that changed the move set, previewed, saved, fought and returned without helper
processes. The ordinary fighter and both legacy ICEBURN transformation
roundtrips also passed on that executable, with their library unchanged.

Independent Blast 1, Blast 2 and Ultimate selection now has a native generation
path, including per-slot behavior, names, effects, animations and scaling.
Separate opening and Ultimate loop camera choices match the reference's rush
restrictions. Fifteen controlled reference comparisons cover cross-slot donors,
all five slots together and independent cameras across six costume/damage
variants. Identical technique text may have different zero-only pack padding.
BT4-origin generation also retains the additional costume partner fields and AI
records rather than treating them as legacy language scripts.

The Windows candidate passed a BT4-only controller attack/camera edit, preview,
save, fight and return with no helper processes. Both Blast 1 positions passed
real Solar Flare execution checks; both Blast 2 positions and an Ultimate passed
attack-startup, damage and recovery checks. All eight generated BT4 costume and
damage variants, the QoL-mod GPU check, ordinary fighter and legacy ICEBURN
transformation regressions passed. This completes the tested individual-attack
step, not the full reference editor. The user's playable launcher is unchanged.

Body and hand selections now resolve the requested source rather than a named
donor whitelist, retaining the structural, texture and model-size safeguards.
Shared empty anchors are accepted only when their bind coordinates agree.
Fitting provenance determines which neighboring surfaces need joining, matching
the reference's one-percent scale threshold.

Inherited auxiliary clothing follows its canonical parent. Root-attached cloth
follows the waist, while head chains and the special hair branch remain separate.
Replacing that owner removes its inherited attachments and rebuilds hierarchy
traversal without orphaning neighboring records. Seven body-part comparisons
pass all six reference costume/damage variants, including the waist case that
previously retained unwanted cloth. The comparisons check both record inventories
and retained oriented triangles; changed winding, weights, normals, UVs or
geometry cannot be hidden by differences in strip ordering. Five representative
BT4-only mixed-part recipes also generate with sanitizer checks.

The updated Windows candidate passed a BT4-only waist edit on a mixed Vegito-chest
body: preview, save, fight and return, with no child helper processes. All eight
saved costume/damage variants and a damaged GPU capture passed, as did the
ordinary fighter and both legacy ICEBURN transformation roundtrips. The separate
sample playtest's actual command launcher reaches the main menu and loads all
four QoL scripts. It uses eight prepared BT4 donor identities and local data
junctions; it is a development checkpoint, not a portable installer.
General heads, auxiliary donor transfer, explicit layers,
and other reference authoring features are not established by these body tests.
Validation covers representative structural cases, not every possible
combination of fighters and parts. The user's playable launcher remains unchanged.

The development donor catalogue now uses the BT4 roster rather than the legacy
161-name list. Preparation covers all 241 named playable identities, excluding
menu sentinels and unused blank entries. The source reader, generation asset
bounds, controller picker and displayed names support the full BT4 identity
range. Every prepared donor's identity, hash and model hierarchy passes the
catalogue check. A high-ID Orange Piccolo forearm edit passed Windows preview,
save, fight and return without the helper. This catalogue work does not establish
support for every exposed reference operation.

Some B14 models retain offsets for a stripped legacy tail. The BT4 importer
accepts that layout only when the retained geometry, facial data, textures and
secondary chains are complete and the physical suffix of the first incomplete
section is entirely zero. Absent sections remain empty; generic pack decoding
and nonempty truncated sections still fail. Current external BT4 metadata is
embedded as usual. The full catalogue includes these models.

Whole-head generation is no longer restricted to Goku (Early). It resolves the
selected source and includes bindings used only by its alternate facial streams.
Legacy recipe spelling variants resolve by name, with ambiguous matches rejected;
legacy numeric identities are never applied to BT4's changed roster. Android 18,
Gohan Beast, Piccolo and the saved Misakah recipe pass native generation with
BT4-only data. Whole-head assembly also transfers hair-owned auxiliary branches,
remaps conflicting node identities and merges their secondary-motion parameters
while retaining clothing and tail chains. Long-hair and cross-body hair surfaces
match controlled reference outputs. The corrected Windows candidate passed the
tested costume/damage fights with QoL mods, and its captures show the actual
chosen hair rather than the old body's hair.

Independent hair, head and face controls now use the same experimental head
switch as whole-head assembly. The reference divides skin and hair by material
batches within both neck and head records, rather than assigning each record to
one control. Native assembly retains that division, transfers auxiliary hair,
and applies source-aware seam adjustments after all selected surfaces are
assembled. Separate hair/head/face cases and a three-source mixed head match
reference oriented surfaces and attachment bindings across six costume/damage
variants, with address/undefined-behavior sanitizers enabled. Malformed draw
packets reject transactionally.

The Windows candidate passes independent hair selection, confirmed preview,
save, fight and return using BT4-only sources, with no helper process. Existing
whole-head, damaged-costume and ICEBURN transformation regressions pass. A
fourth character can be created alongside a copied three-form user library
without changing those existing characters. The separate development playtest
now prepares all 241 named playable BT4 donors; it retains the four-form roster
limit. None removal, explicit donor costumes, additional layers and other
advanced reference authoring features remain incomplete. This is not full
reference-editor parity or a portable installer; GitHub remains on hold.

Animated expression fitting now applies the resting face's donor-to-body anchor
translation to the separate mouth/expression draw streams. This fixes the
shared cross-body jump seen on Bomani (Krillin body, Vegeta Super Saiyan God
head) during speech and damage reactions. Matching anchors preserve exact
vertices; malformed streams reject transactionally. Sanitized tests cover 36
streams and 19,008 vertices across eight head/body combinations, preserving
nonposition attributes. Bomani's eight generated costume/damage packs change
only expression sections 4–7 (one-based); his recipe and other characters are
preserved. A Windows GPU fight completed with damage and active expression
shapes, and both ICEBURN transformation regressions passed. A user visual spot
check remains necessary before publication.

Setup and Update prepare the 241-donor cache from the installation's own BT4
resources and publish it atomically. Partial preparation leaves the previous
installation usable. The single installed launcher enables the creator when
that completed cache exists; a BT3 installation does not prepare the BT4 cache.
The updater continues to preserve saves, settings and edited/third-party mods.
No source cache, disc resource or personal character is included in releases.

Native online scope hides custom fighters while keeping the creator switch
active. Network roster/loadout validation independently rejects custom IDs,
including requests received outside the scoped picker. Leaving online scope
restores the local library. A two-client local Windows test passed 600 confirmed
frames with creator mode enabled and four custom forms present. This is not an
internet/LAN regression guarantee or custom-character network support. The
rematch crash-fix implementation and prior desync diagnostics are retained.

Older signed checkers update program files using their own embedded updater.
The new Play.exe also checks for an unprepared BT4 creator cache after that
update and starts the newly installed checker in local preparation mode. It
then resumes the same launcher. If preparation fails, ordinary play remains
available for that launch and preparation retries next time. This avoids
requiring an extra launcher or a separate creator install for previous releases.
