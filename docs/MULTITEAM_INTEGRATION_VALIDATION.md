# Multi-team integration validation

This candidate integrates upstream Beta 6 (`2f09590`) into our Creator Save
Hotfix baseline (`cecc2d7`). It retains the native creator, ordinary-fighter
online eligibility, rematch repair, F1 controls, Lua mods, and updater data
preservation. Custom fighters remain offline-only.

## Integration corrections

- Preserve our RNG and rollback capture alongside upstream's semantic report
  exchange. The v3 capture includes team membership and the winning team; the
  comparison tool still reads legacy captures. Separate clients save reports
  beside their configured settings rather than overwriting a shared file.
- Bound lobby team counters by `TAG_SQUAD_TEAMS`, fixing an upstream stack
  overflow exposed by five-team validation under AddressSanitizer.
- Apply the existing Hide all HUD setting to the new winner banner.
- Retain both the native team-menu entry and Z Warriors entry.
- Keep our installer, creator-data preparation, signed updater, and release
  metadata handling rather than replacing them with upstream packaging.

## Completed checks

- Full Windows link; 103 inherited stubs, no link errors.
- Squad rules: 552 size vectors. Lobby rules: 527 arrangements under address
  and undefined-behavior sanitizers.
- Rematch reload/pool/scheduler checks across 2–15 fighters and repeated loads.
- RNG capture and diagnostic regression suites, including legacy reports,
  team attribution, checksum recomputation, and output-directory isolation.
- Windows HUD regression: individual visibility controls, previews, online
  settings, and master hiding including the winner banner.
- Native menu flows: three-team setup, five-team CPU setup, retry, back, and
  settings pagination.
- GPU fights with three teams and five teams; team winners and targets checked.
- Cross-row teammate revival and two-player fusion/separation.
- Local online ordinary-fighter match with creator enabled; three-player,
  three-team round completed with matching winner and return to practice.
- Forced desync: both clients produced both report formats. Our paired capture
  comparison finds the injected HP difference at frame 300, with matching
  configuration and no capture checksum errors.
- First save with no existing character library, fight, and editor return;
  no external builder child process.
- Both ICEBURN forms transform in both directions with script mods enabled.
- Update preservation suite (9 tests) and creator cache setup suite (5 tests).
- Baseline comparison: 46 protected files remain unchanged, including creator,
  rematch repair, updater preservation, F1 settings, and bundled Lua mods.

The final executable repeats first-save, ICEBURN, and paired-diagnostic checks.
Earlier gameplay checks used the same gameplay code before the final diagnostic
file-location correction.

## Release boundary

The user subsequently confirmed a five-team match with two fighters per team,
including custom fighters. The full installer and updater archives were built
and their signatures verified against the launcher's public key. The frozen
updater successfully upgraded an isolated previous-release installation to
Beta 18, preserving all 84 checked character, save, settings, and mod files.
Archive integrity and payload executable identity also passed.

This validation does not establish separate-computer LAN or internet
interoperability, every possible character combination, or a fresh installation
from every supported disc image. Public publication is a separate step.

Build logs, captures, private character copies, and locally linked BT4 assets
remain under ignored `port/build`; they are not source or release payloads.
