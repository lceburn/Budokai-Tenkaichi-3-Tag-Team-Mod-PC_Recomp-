# Follow-up: camera-dependent network targeting (2026-11-23)

## Evidence received

Two paired Beta 24 reports match build, data, configuration and inputs:

- Gauntlet: first retained state disagreement at blank 20128 includes player actor 2 targeting actor 1 versus 3. A right-stick directional input precedes it. RNG also differs at that retained snapshot; its original divergence is not established by this window.
- Free-for-All: first retained disagreement at blank 3624 is **only** player actor 2's target (1 versus 0), following release of L3 while relocking. No captured AI-check or input disagreement precedes it.

These reports do not establish the players' camera-mod settings. They provide a target-selection lead, distinct from the previously repaired loading cadence.

## Controlled reproduction

An opt-in pad probe sends ordinary synchronized inputs: hold L3 to unlock, tap to relock, flick the right stick up/down. Two local clients run the same executable, settings, scripted inputs and 114% speed, with stock fighters and CPU combat.

- `beta24-rightstick-camera-isolation`: guest alone has a visual Lua camera roll. Desync at checksum blank 1290. First retained difference at 1288 is **only target** (actor 1, host 0 versus guest 2). No input, RNG, AI sense or attack-readiness difference at the first state divergence.
- `beta24-rightstick-camera-control`: same candidate and inputs without the camera modification; completed 3600 blanks without desync.

The reproduction demonstrates a concrete defect: `relock`, screen-ordered targeting and directional targeting read the rendered `world2screen` matrix. A local visual camera setting can therefore change simulation target selection. It is consistent with the user captures; it does not prove every reported desync has this cause.

## Local correction under validation

Network cameras retain an unmodified targeting projection before script camera changes, presentation zoom, collision smoothing and local camera-distance adjustments. Its 4:3 projection is canonical, independent of client aspect preference. All three target-selection paths use that projection online; offline retains the existing displayed-camera behavior. The targeting views live in rollback memory. Rendering still uses each client's chosen visual camera.

No release has been published for this follow-up. The diagnostic pad hook is opt-in and disabled during ordinary play. Local run artifacts and paired comparisons are under `port/build/`.

## First corrected runtime result

`beta24-rightstick-camera-fixed` repeated the failing inputs and guest-only camera roll on the corrected candidate. Both clients completed 3600 blanks at 114% without a desync. This is a local regression result, not a published update or proof of every original failure's cause.

Reusable probe (Windows Python; supply isolated imported data folders):

```
python port/tests/tag_target_camera_runtime.py --exe <diagnostic-candidate>/engine/TagTeam.exe --data <host-data> --client-data <guest-data> --label target-camera-probe
```

Add `--same-camera` for the unmodified-camera control. An unfixed build with the opt-in pad hook should fail the changed-camera test; a corrected build should finish it. The hook is necessary so this test does not silently run without exercising right-stick targeting.

## Longer custom-fighter regression

`beta24-target-fixed-custom-3v3` completed 7200 blanks with two-way sharing (three custom forms total), six fighters, CPU combat, 114% speed, scripted relock/right-stick targeting, guest-only camera roll and frame skipping off. No desync. Both saved libraries were checked unchanged by the runtime harness. This is not a Gauntlet wave-transition or arbitrary-transformation acceptance test; those remain required before a release covering this correction.

## Gauntlet release check

`beta25-target-gauntlet` passed all three Quick Test waves, the result and return to practice with two-way shared custom fighters, repeated synchronized lock-off/relock/right-stick inputs, guest-only camera roll and 114% speed. Both clients passed wave lifecycle checks through 2160; neither desynced. Both saved libraries remained byte-identical. This uses the existing resource-lifecycle fixture (survivors are kept alive and waves are ended after ordinary combat), not a claim of exhaustive natural matches.

Both signed Windows archives passed integrity/signature checks, and an isolated upgrade from the previous release preserved all 85 checked character, save, settings, mod and deletion-recovery files.
