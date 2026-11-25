# Emotes, loading input, and creator portraits

This follow-up keeps the existing deterministic network loading cadence and canonical targeting projection. The loading minigame reads local button edges separately from synchronized fighter input. Its local cosmetic choices use a separate RNG so unequal button mashing cannot advance fight randomness.

Photo Mode's paused-menu toggle is now Square (X on Xbox). The F1 Spectator option remains opt-in, and only an actually frozen offline pause enables it. L1 remains available for spectator camera movement. Select changes views and Start resumes.

F1 > Cheats > Emote selects the replacement for the normal manual taunt. While idle, hold L1 and press Circle (LB + B); movement cancels. The chosen emote is carried in the existing 18-byte synchronized controller report, and activation state follows simulation ticks and rollback. CPU injection and replay playback do not start new emotes. Drawing temporarily changes the model's node matrices and restores them before gameplay uses the model again.

Adapted the emote state machine, animation samples, rendering and selection UI from [NauTaro's release](https://github.com/NauTaro/DBZ-BT4-Tag-Team-Decomp/releases/tag/v0.1.39.6-R1.20), commit `a5e13c9a1dcd4b5847746ca330e55f9796b3941f`, under the repository's AGPL-3.0 license. The original animation-source attribution is retained in `battle_emote_motions.inc`. Other upstream networking, rendering and updater changes are not merged.

The creator's Portrait option selects stock images independently of all part choices: the large portrait, two supporting HUD/picker resources, and the active health-bar texture sheet embedded in each costume/damage resource. Only that image section changes; geometry, skin textures, facial animation, moves and stats remain intact. Default follows the original body images. Resource filenames remain stable during an edit, and image files publish transactionally with the recipe. A failed publication rolls back already-replaced resources; save paths remain confined to the selected warrior folder.

Validation completed so far:

- Focused tests cover emote activation/cancellation and deterministic state, isolated local loading input, Square Photo Mode, portrait resource bounds and transactional rollback.
- A rendered offline run confirmed emote activation and movement cancellation, Square menu hide/restore, frozen fighters, camera movement, view switching and normal resume.
- A paired live run confirmed that the guest's button presses advanced the loading minigame while both clients saw its synchronized emote, without a match desync.
- Shared-custom Gauntlet completed all three quick-test waves and returned to practice with both saved libraries unchanged. Different-camera target switching also passed its paired regression.
- The portrait flow selected, saved, reopened and loaded the fighter into battle. The captured health bar shows the selected Vegeta portrait with an unchanged Goku model. Real-resource tests verify that the active HUD section changes in all eight costume/damage resources and that every other model section remains byte-identical. A failed save restores all replaced files.
- Both signed Windows archives passed integrity checks. The isolated updater preserved all 85 checked character, save, settings, mod and deletion-recovery files.

The final rebuilt-candidate paired custom-character regression and public-release verification are recorded separately. These checks reduce regression risk; they do not establish that every possible network desync is fixed.
