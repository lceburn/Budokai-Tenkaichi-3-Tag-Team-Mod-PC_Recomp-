# Custom-fighter networking test release

Candidate: Windows Beta 19 Experimental, native room protocol 10.

The release adds session-only sharing of selected saved fighters and their
reachable linked forms, a nineteen-form room registry, and independent character
duplication. Saved customs appear at the end of the existing fighter picker;
there is no separate custom-only picker or battle mode.

## Verified

- ASAN/UBSAN registry, package, sender/receiver, budget, generation, stale ACK,
  malformed/truncated data, nineteen-form capacity and local-library isolation.
- Resource-layout checks for models, mesh records, texture uploads, animation
  and effect packs, portraits and optional chips. Legacy six-section effects
  and native seven-section effects are both supported without changing saves.
- Separate host ICEBURN and guest Bomani libraries exchanged packages over the
  real room socket and started synchronized matches with matching room IDs.
- ICEBURN's linked transformations execute and return on both clients.
- Two completed ordinary-room matches, return to lobby and normal room exit;
  temporary directories are removed and the saved libraries remain unchanged.
- Three-team custom match, practice return, joining during that match and joining
  already-running practice after custom sharing; CPU fallback after disconnect.
- Normal controller navigation to the custom tail of the standard picker, GPU
  capture and confirmation through the real picker into room admission.
- Ten-player/ten-spectator ownership, promotion, full-room rejection, readiness
  reset and reconnect checks; paired diagnostic reports from an intentional HP
  mismatch remain complete.
- The corrected final executable repeats two-way sharing, transformations and
  standard-picker confirmation after downloaded-resource validation was added.
- Both Windows archives have valid signatures under the existing updater key.
  The frozen updater preserved all 84 checked saved-character, save, settings
  and mod files when upgrading an isolated Beta 18 installation to Beta 19.
- Startup source ranges and checksum match the published Beta 18 manifest;
  the candidate verifies against the existing local BT4 startup tables.

The rematch repair files, updater preservation logic, creator installation
logic, F1 controls and bundled Lua mods remain unchanged. Network diagnostics
are extended by the room catalog, not replaced.

## Test boundary

These are local-process tests, not proof of separate-computer connectivity.
Publish the signed experimental update so both computers can update through
their usual launchers, then test LAN/internet joining, transformations, rematch
and room exit. All clients must install the same version.

New families are shared from native setup. Running-practice editing can use
fighters already registered for the room. Received fighters are not imported
into the permanent library. Explicit permanent sharing, larger capacities and
unrelated editor polish are outside this update.
