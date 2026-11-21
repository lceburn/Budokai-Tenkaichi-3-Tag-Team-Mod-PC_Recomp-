# In-development custom-fighter room protocol

Implementation notes for the unpublished network update. Protocol 10 connects
catalog acknowledgements, bounded package uploads, host admission and verified
downloads to the live lobby. A private host-ICEBURN/guest-Bomani-library test
downloaded the host's two-form package and started a synchronized match while
preserving both saved libraries. This is not full feature or release acceptance.
Two-way sharing, linked transformations in both directions and two completed
ordinary-room matches with lobby returns also pass with those separate libraries.
The standard fighter picker now appends saved customs, with controller/GPU and
confirmation tests passing. Practice return, late joining, disconnect-to-CPU,
normal temporary-file cleanup and controlled paired diagnostics also pass.
Signed archive, upgrade and publication checks are the remaining release gate.
Older clients receive the existing version-mismatch explanation.

## Identity and lifecycle

The host assigns an append-only room index to each admitted form. Gameplay IDs
are `PORT_WARRIOR_SESSION_FIRST + index`; local IDs never cross the wire.
Export the selected fighter's reachable form set, not every saved fighter in
the participant's library. Unused local characters must not consume room slots.
The initial room capacity is `PORT_WARRIOR_SESSION_MAX` (nineteen), including
base fighters and transformations. A package is admitted in full or rejected.
Keep mappings and resources stable through fights, disconnect-to-CPU fallback,
and rematches. Do not reclaim an in-use form when its owner disconnects.

The room catalog records participant generation, package hash, form count and
assigned index range. Every form target must resolve within the admitted
package. The host sends a catalog revision and hash; every client acknowledges
the same verified revision before readiness or match loading is accepted.
Reject foreign/local IDs and stale catalog revisions on every received edit,
loadout, practice event and start path, not only in the visible selector.

Received files live under a uniquely generated `.warrior-sessions` room folder.
The existing `mods/enabled.txt` and character folders are never edited. Restore
the local registry before clearing owned temporary files at final room exit.
Temporary import and explicit permanent sharing are different operations.

## Transfer requirements

- Use bounded chunk messages on the existing reliable room connection, with
  explicit expected length, offset and cryptographic package digest.
- The current frame limit is 4096 bytes. Reserve header space and use chunks
  no larger than 3072 bytes; do not enlarge the simulation packet allowance.
- Send a limited amount per pump and honor queue backpressure. Ping, control
  and simulation messages retain priority. No file transfer from rollback code.
- Check packet bounds, ownership, declared count, cumulative allocation and
  aggregate room capacity before allocation or disk writes.
- Verify complete content before relay or activation. A content hash establishes
  agreement, not model safety: validate playback resource layouts independently.
- Package parser bounds: 128 MiB per package, per-resource limits, nineteen forms
  per room. Add a separate aggregate memory/disk budget when wiring transfers.
- Show transfer progress. Provide bounded timeouts, cancellation and a clear
  error; failure must preserve existing libraries and the last valid catalog.
- Spectators and late joiners must receive and acknowledge the active catalog
  before replay/catch-up or battle loading starts. No registry changes while
  the battle still owns decoded resources.
- Reused player seats need generation-qualified identities so a new participant
  cannot replace the previous occupant's active resources.

## Required checks

Two clients must have genuinely different local libraries and form ordering.
Test selecting each owner's fighter, linked transformations both directions,
rematch, returning to the lobby and closing the room. Check byte-identical local
libraries afterward. Also test missing/corrupt chunks, over-capacity admission,
stale acknowledgements, interrupted transfer, reconnect and spectator catch-up.
Keep the stock-fighter, multi-team, rematch and diagnostic regressions.

Only then prepare a signed updater for the user's two-computer LAN/internet
playtest. A localhost test does not establish separate-computer connectivity.
