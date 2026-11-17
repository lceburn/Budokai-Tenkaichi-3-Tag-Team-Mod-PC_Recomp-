# Beta 14: native LAN and internet desync capture

This experimental Windows update adds diagnostics for native Tag Team network matches. It does **not** claim to fix the reported desyncs. The Beta 13 rematch reload repair is retained. Main is unchanged; development remains on `integration/z3xox-main`.

## Install and collect

1. Update **both computers** through Play.exe, or fully extract the updater archive and run Update.exe with its payload beside it. Both must use the same release.
2. Start Play normally. Native network capture is **enabled automatically**; no command file, console window, offline effect checker or ProcDump is required. This works for LAN and internet matches using the native Tag Team rooms.
3. Reproduce the desync. On the existing match-stopped event, each game attempts to save **net-desync.txt beside Play.exe**. It is a normal text report, not a process dump. The diagnostic does not introduce a new stop condition or disable the desync check.
4. Before another desync or another launch, copy **net-desync.txt and game.log from BOTH PCs** somewhere safe. Label them host-net-desync.txt / host-game.log and client-net-desync.txt / client-game.log. Additional participating PCs should preserve their own copies too. A menu-only practice coordinator may have no local fighter simulation; collect the reports from the computers actually simulating the fight.
5. Return the paired files privately in this chat with the selected mode, roster, and what was happening immediately beforehand (transformation, fusion, destruction, death, join/leave, rematch). Do not upload reports, dumps, saves, disc files or session data to public GitHub.

Only the latest desync report is kept per installation. A later desync overwrites it. The report is not refreshed on a successful match. Play overwrites game.log at launch, but net-desync.txt remains until a new capture. If saving fails, game.log records the failure; the capture does not raise an exception to compensate.

Beta 14 automatically suspends the **offline effect-lifecycle checker during native network play**, even if an old effect-diagnostics.enabled marker remains. The native network capture is separate and supports rollback. For older releases, the offline effect marker must still be removed before LAN play.

## Bounded overhead and opt-out

The recorder holds a fixed 512-tick ring in host memory (a few MB). It copies selected state and does no per-frame disk logging or network transmission. On a desync, it writes at most 121 ticks around the reported mismatch, subject to available history. No full game memory or proprietary resources are included. Input bytes, roster IDs and gameplay state are still private test data: share them only with intended recipients.

To turn this capture off, close the game and create an empty **disable-net-diagnostics.txt** beside Play.exe, then restart. Remove that file while closed to re-enable. Advanced alternative: set BT3_NET_DIAG=0 in the launch environment. Enable/disable state is read once per process. An existing explicit BT3_TAG_NET_AUDIT environment setting enables an older verbose console audit; leave that unset for this bounded workflow.

## Report contents and time semantics

- Build identity, game-data fingerprint, room round/revision, local and peer roles, mismatch tick and local/remote checksums.
- Rules, roster/costume/controller assignments, CPU levels, items and synchronized option values, without IP addresses or player names.
- The exact checksum components: match header, every fighter's identity, health/ki/blast, action, target/controller/lock-off, fusion/revival/assist fields, position/rotation float bits, libc RNG and lobby fingerprint.
- Additional diagnostic-only RNG state: MT19937's 624 words/index, scene/fighter generators and VU0 R. These extra values do not alter or extend the wire checksum.
- Recent actual received inputs and inputs used for prediction, with validity masks. Inactive producer slots are excluded. A missing record or mask means unavailable, not zero.
- Frame confirmation/check progress, recorded rollback counts and resimulation flags. Replayed ticks overwrite earlier predictions. A rewind invalidates abandoned future snapshots before rebuilding them. Round changes reset the capture history.

State is captured at **tick entry, before that tick's inputs**. A state difference at tick T can result from earlier input or simulation work. The reported mismatch is a periodic checksum checkpoint, not necessarily the first divergent instruction. The first difference in this bounded window may already be downstream of an earlier cause.

The code reads state without invoking RNG generators, writing simulation structures, changing inputs, changing network packets, or replacing checksum values. Capture metadata is host-only and excluded from rollback snapshots. Differences in the extra RNG fields alone are investigation clues, not proof of the cause or necessarily a difference in the wire checksum.

## Comparing paired reports

The repository includes an offline comparison tool; testers do not need Python to generate reports:

```powershell
python port/tools/compare_net_desync.py host-net-desync.txt client-net-desync.txt
```

It checks build/data/round/mismatch pairing, verifies reversed local/remote hashes, independently recomputes complete captured state checksums, and identifies the first recorded differing fighter field/component and input bytes. Extra RNG differences are reported separately. Reports from different events are rejected for state comparison. The tool does not send any files over the network.

## Validation and limits

Asset-free tests cover ring wraparound, all 15 supported actor indices, full RNG state, input validity masks, rollback replay/future invalidation, first-report behavior, round reset, disabled capture, and report-write failure. Paired synthetic reports identify an injected health difference at the exact actor/tick and recompute the captured checksum. The retained offline lifecycle tests check that native networking suppresses that offline checker.

Release notes record the completed Windows build, Windows capture harness and package checks. Matching DWARF/debug information, linker map, source/build digests and executable SHA-256 are included in the separate symbols archive. No PDB is emitted by this toolchain.

**Not yet verified on the development machine:** a real BT4 LAN/internet desync capture, correctness of techniques during network gameplay, and the root cause of the supplied desyncs. Those require the tester's local disc installation and paired network sessions. The next reports are evidence for that investigation, not proof that the desync is fixed.
