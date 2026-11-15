# Beta 10 changes

Branch: `integration/z3xox-main`. Main is unchanged.

## Select camera cycle

With F1 > Spectator > Use Select to cycle camera enabled, Select now cycles Game camera → Free camera → Fight overview → Game camera during combat. Follow fighter remains in F1, with its target selector; the existing death-spectator controls are unchanged. Pressing Select while Follow is selected goes to Overview.

## Fighter-anchored unlocked look

Vertical right-stick look rotates the camera position around the fighter's existing body anchor along with its viewing angle. This preserves the original distance and framing rather than pointing away from the fighter. The native horizontal head-plane push is skipped for this orbit because it breaks steep vertical framing; terrain collision remains enabled.

Unlocked input is now based on actual native lock flag 5, not just the tag manual lock-off setting. After L3 starts searching, horizontal and vertical look remain available until the target is acquired. Pitch resets on actual lock acquisition. Authored cut/fixed cameras keep their existing behavior. Existing horizontal/vertical sensitivity sliders still apply, and online rooms must agree on control settings.

## Health-based clothing damage

The native disc model path exposes normal/damaged costume variants, not a sequence of arbitrary damage levels. At or below 50% health, an undamaged living fighter switches to its damaged costume at its next idle action (0xB). Sturdy Clothes prevents this. Clothing stays damaged if healed until the normal match/member lifecycle resets it.

The change uses the native model-only loader, serializes through the existing change queue, and yields to pending transformation/member requests. It automatically acknowledges model readiness and completion without requiring a new attack animation. Placement is saved/restored; the existing active gauge stores the variant. Loading may cause the same short time stop as other native costume reloads. No new costume assets are supplied; appearance depends on the damaged model in the user's disc and texture pack.

## Deflected small blasts

Hit detection records the actual deflector's object ID in a task class-free byte (+0x0B, initialized on task creation). Post-update consumes that value rather than using the shooter's current lock target, and transfers ownership to that fighter rather than XORing the original owner with 1. This applies to both regular blasts and modeled ki objects, and preserves the original source for emitter resources. The feedback is in native task memory covered by rollback.

Regular blast termination explicitly kills persistent emitter handles. The existing `EftEmit_TermState` function is empty, so calling it alone can leave visuals alive when the blast task's work storage is recycled. Normal blast lifetime and reflection lifetime reset remain intact.

## English errors

Play.exe startup and unexpected-exit dialogs now use English text only, including directions to game.log and bt3_crash.txt. Engine crash reports were already in English. Windows-owned dialogs follow the Windows language setting.

## Validation and limits

Automated checks exercise production camera calculations at both pitch limits, preserving orbit distance and body-anchor framing; look while searching and native locked-on behavior; Select cycle ordering and held-button edges; production model-change queue eligibility/readiness/completion and isolation from normal requests; actual multi-fighter deflector ownership, reflection lifetime, source retention and emitter cleanup; and the existing Lua/native HUD checks.

BT4 B14 REV2 disc gameplay must still confirm costume reloads with individual characters and texture packs, repeated deflection effects, camera collision near terrain, and searching/lock acquisition. No disc assets are available on the development computer. Multiplayer participants must all use the new build; real local/online acceptance remains outstanding.
