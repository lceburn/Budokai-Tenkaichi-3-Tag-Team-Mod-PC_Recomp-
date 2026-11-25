# Damaged surface colors and Photo Mode

Surface Colors now offers Normal and Damaged material lists. The preview follows the selected material's costume and damage variant. Normal color edits carry into corresponding damaged materials, including mixed-donor assemblies, while retaining texture shading and damage marks. Ambiguous matches remain available for manual editing.

Copy Color and Paste Color reuse a shade across materials and variants. Reset Color restores the selected material's original donor default, including after saving and reopening. A damaged material can override the inherited normal color without changing the normal model.

The F1 Spectator menu adds an opt-in Photo Mode setting. During an actually frozen offline pause, Square hides or restores the pause menu, spectator camera movement and Select view cycling remain available, and Start resumes normally. The prior camera mode returns on exit. Online pause does not freeze a network match, so this mode is unavailable there.

The network loading minigame change is deferred. Existing deterministic network loading cadence, canonical target selection, crash repairs, diagnostics, updater preservation, and script mods remain in place.

## Verification

- Focused color tests cover variant isolation, inheritance, copy/paste, original-default reset, and recipe preservation.
- Real stock and mixed-donor generation covers normal/damaged costume variants, material mapping, and geometry preservation.
- The editor runtime covers preview, save, reopen, fight, and return.
- Photo Mode runtime covers frozen fighter poses, moving camera, single-view draw lists, view cycling, pause-menu restoration, and normal resume. The observer's obsolete object-layout offset was corrected using the actual compiled structure layout; failed earlier reports remain retained separately.
- Paired network regressions check the final release candidate before publication. These are regression checks, not a claim that every desync is eliminated.
