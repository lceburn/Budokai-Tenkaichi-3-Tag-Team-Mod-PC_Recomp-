# Custom fighters in network rooms (in development)

This work follows the published Beta 18 integration. It is not enabled in a
released build. Scope: temporary custom fighters and linked transformations for
LAN/internet rooms, plus an independent Duplicate Character editor action.

## Acceptance requirements

- Nineteen custom forms per room, counting base fighters and transformations.
  A room over capacity must explain the limit before starting; never omit forms.
- Room identities must be shared consistently, separate from local library IDs.
- Received packages must not execute recipes or contain arbitrary filenames.
- Readiness requires all participants to validate the same complete packages.
- Rematches retain the room data; leaving restores the original local library.
- No permanent import of received characters without explicit sharing/import.
- Preserve ordinary online play, diagnostics, rematch fixes, creator and mods.
- Publish a signed testing update only after internal tests, then test separate
  computers using the existing updater. The user authorized that release flow.

## Implemented foundation

- `PortWarriors_Session` transactionally registers room resources, resolves form
  links and restores the untouched local registry. Capacity is defined once.
- Room character IDs start at 512, avoiding local IDs and the legacy 255
  transformation sentinel. Gameplay routing and gauge/equipment tables support
  the separate room namespace. Network eligibility requires session registration.
- Character selectors use the mapping rather than deriving IDs from array
  indices. Local legacy IDs are preserved.
- `WarriorNetPack` exports generated resources and bounded playback metadata;
  imports use fixed filenames in fresh session directories. Authoring recipes,
  scripts and executables are excluded. Parsing validates complete packages
  before creating files, including form references and resource-size limits.
  Export follows only the selected fighter's reachable transformation graph,
  including cycles; unrelated saved fighters are excluded. Windows destination
  checks reject reparse points as well as ordinary symbolic links.
- Sanitized synthetic tests pass for nineteen room forms, linked transformations,
  model routing, equipment slots, truncated packages, unsafe destinations,
  failed-activation preservation and restoring a byte-identical local library.
- Chunk reception now enforces ordered offsets, bounded chunks and shared
  memory/form reservations. BLAKE2b package verification, participant generations,
  malformed-input rejection, progress timeouts and cancellation pass sanitizer
  tests. This receive state is not yet connected to the room socket.
- The shared catalog assigns immutable room IDs, rejects partial over-capacity
  admission, retains resources after disconnection and tracks generation-bound
  acknowledgements of its complete digest. Tests cover nineteen forms, stale or
  mismatched acknowledgements, missing local verification, reconnect generations,
  append-only catalog decoding, truncation and attempted ID replacement. These
  readiness rules are connected to the live lobby's ready/start paths as below;
  package admission and transfer still need connecting.
- Live protocol 9 now exchanges generation-bound catalog acknowledgements and
  gates match start. A two-client stock-fighter match passed with the creator
  enabled. The ten-player/ten-spectator permission, promotion, ownership,
  readiness-reset and reconnect suite also passed with the new handshake.
  This only exercises the empty catalog: custom package transmission and live
  registration are still missing. Stock late-join behavior remains available;
  custom late-join synchronization must be implemented before enabling assets.
- The unpublished protocol 10 candidate now wires offer/accept, bounded frames,
  host admission, catalog downloads and registration into ordinary setup rooms.
  A real localhost test used separate saved libraries (host ICEBURN pair, guest
  Bomani), transferred the 9,321,963-byte/two-form host package, and started a
  synchronized match. Both saved libraries stayed byte-identical. This test
  covers host-to-guest sharing only, not transformations or a release UI.
- A read-only saved-local snapshot/export path now passes isolated tests while
  the downloaded-room registry is active. It avoids the join-order restriction
  where receiving room data prevented exporting the player's own character.
  Its two-way Windows socket test passes: the host receives Bomani's
  8,235,904-byte package, both clients register three forms, and both enter the
  same match. The readiness cycle found by that test is fixed: catalog ACKs do
  not wait for a player's not-yet-selected custom fighter.
- The same Windows executable passes the real ICEBURN transform-and-return
  fixture on both clients, including settled-state checks, and two completed
  ordinary-room matches with lobby returns. The form packages are admitted once
  and retained across those returns; all saved-library files remain unchanged.
- Saved customs are appended to the existing standard fighter picker. The
  initial separate source page was removed per the user's requirement. Normal
  pad navigation, GPU capture and confirmation-to-room admission pass. Read-only
  local resource keys keep the live registry intact while the picker is open;
  local IDs remain rejected on the wire. Costumes, items and CPU difficulty are
  carried through selection instead of being reset during admission.
- The prior Windows candidate passes normal room exit after two completed
  matches: both temporary room directories are removed and saved libraries
  remain byte-identical. A custom three-team match, joining during that match
  and participating in subsequent practice, disconnect-to-CPU continuation, and
  a controlled mismatch with complete reports on both peers also pass.
- Active-practice late joining also passes: the new participant downloads the
  complete catalog, registers it, then receives configuration and replay/start
  messages. Catalog ACK is required before those starts are sent.
- Resource parsing now checks offset-table extents, model parameters and mesh
  hierarchies, texture upload extents, animation/effect layouts, compressed
  portrait headers and optional chips before installation. Six-section legacy
  effects and native seven-section effects both pass, including the unchanged
  ICEBURN files. Malformed nested-resource tests pass under ASAN/UBSAN. The exact
  corrected Windows candidate repeats two-way transfer, transformation and
  standard-picker confirmation successfully.
- Duplicate Character creates an independently editable draft with a unique
  Copy name and folder, without modifying or automatically linking the source.
  Square in the native library opens it. The draft-operation test passes unique
  naming, source independence, capacity handling and the existing linked-form
  action. A Windows controller test duplicated a saved warrior, saved the copy,
  loaded character 251 in a fight and returned to the two-character library;
  all original character files remained byte-identical and no builder child
  process ran. Existing local capacity remains four forms; room capacity is
  separate.
- The internal Windows foundation links successfully (103 inherited stubs,
  no link errors). These are development artifacts, not a release or launcher
  replacement. Selected-package and future-capacity changes were subsequently
  relinked; the duplicate runtime test uses the same unchanged duplicate code.

## Release boundary

The bounded sender now emits big-endian begin/chunk/end frames one at a time.
It retains its offset when the transport queue is full and expires stalled sends.
Sanitizer tests round-trip these frames into the production receiver, verify the
bytes, and check backpressure at every frame, cancellation, and timeout. This is
a transport-adapter test, not a custom-character transfer over a live socket.
Verified receive-buffer ownership transfer also passes: staging cancellation
cannot free a buffer retained by the room relay. Local-library preservation and
nineteen-form registry tests pass with these changes.

Both signed archives and the frozen updater upgrade check now pass, preserving
all 84 checked character, save, settings and mod files. The remaining publication
steps are source publication, asset upload and public updater verification.
There is no new download until those steps finish. The user has authorized that
test-release publication without another approval prompt. Actual LAN/internet
play on separate computers remains the user's next acceptance test. Current
sharing is selected from native setup; running-practice editing can use fighters
already registered for the room. Permanent importing/sharing, larger capacities
and unrelated editor polish remain outside this update.
