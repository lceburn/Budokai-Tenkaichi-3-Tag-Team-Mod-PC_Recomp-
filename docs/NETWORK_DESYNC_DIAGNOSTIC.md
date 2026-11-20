# Beta 15: RNG caller and phase desync capture

This experimental Windows update adds diagnostics for native Tag Team network matches. It does **not** claim to fix the reported desyncs. The Beta 13 rematch reload repair is retained. Main is unchanged; development remains on `integration/z3xox-main`.

Beta 15 extends the Beta 14 reports with random-number consumption attribution. The paired Beta 14 captures showed matching captured fighter state but differing shared RNG state. That establishes a useful lead, not the responsible caller or a repair. The generator algorithms, checksum and network protocol remain unchanged.

## Install and collect

1. Update **both computers** through Play.exe, or fully extract the updater archive and run Update.exe with its payload beside it. Both must use the same release.
2. Start Play normally. Native network capture is **enabled automatically**; no command file, console window, offline effect checker or ProcDump is required. This works for LAN and internet matches using the native Tag Team rooms.
3. Reproduce the desync. On the existing match-stopped event, each game attempts to save **net-desync.txt beside Play.exe**. It is a normal text report, not a process dump. The diagnostic does not introduce a new stop condition or disable the desync check.
4. Before another desync or another launch, copy **net-desync.txt and game.log from BOTH PCs** somewhere safe. Label them host-net-desync.txt / host-game.log and client-net-desync.txt / client-game.log. Additional participating PCs should preserve their own copies too. A menu-only practice coordinator may have no local fighter simulation; collect the reports from the computers actually simulating the fight.
5. Return the paired files privately in this chat with the selected mode, roster, and what was happening immediately beforehand (transformation, fusion, destruction, death, join/leave, rematch). Do not upload reports, dumps, saves, disc files or session data to public GitHub.

Only the latest desync report is kept per installation. A later desync overwrites it. The report is not refreshed on a successful match. Play overwrites game.log at launch, but net-desync.txt remains until a new capture. If saving fails, game.log records the failure; the capture does not raise an exception to compensate.

Beta 14 automatically suspends the **offline effect-lifecycle checker during native network play**, even if an old effect-diagnostics.enabled marker remains. The native network capture is separate and supports rollback. For older releases, the offline effect marker must still be removed before LAN play.

## Bounded overhead and opt-out

The recorder holds a fixed 512-tick state ring, 64 caller/phase/view aggregates per tick and 65,536 recent RNG events in host memory (under 16 MiB in the tested layouts). It does no per-frame disk logging or diagnostic network transmission. On a desync, it writes at most 121 state ticks and RNG event intervals from 32 ticks before to 8 after the reported mismatch, subject to available history. A report can occupy several MB; only the latest is kept. No full game memory or proprietary resources are included. Input bytes, roster IDs and gameplay state are still private test data: share them only with intended recipients.

To turn this capture off, close the game and create an empty **disable-net-diagnostics.txt** beside Play.exe, then restart. Remove that file while closed to re-enable. Advanced alternative: set BT3_NET_DIAG=0 in the launch environment. Enable/disable state is read once per process. An existing explicit BT3_TAG_NET_AUDIT environment setting enables an older verbose console audit; leave that unset for this bounded workflow.

## Report contents and time semantics

Multi-team integration retains the bounded RNG/state capture as
`net-desync-v3`. Its match header additionally records the number of teams and
winning team, and each fighter's checksum state includes team membership. The
comparison tool still reads the older v1/v2 four-word-header captures and
reconstructs both layouts' checksums. Use matching versions and symbols on all
participating computers.

The upstream room protocol also exchanges a small semantic mismatch snapshot
between the participating peers and writes `tagnet-report.txt`. That report
does not replace `net-desync.txt` or the local RNG history. Preserve both reports
and `game.log` if available. The bounded RNG recorder itself does not transmit
its history or upload files.

- Build identity, game-data fingerprint, room round/revision, local and peer roles, mismatch tick and local/remote checksums.
- Rules, roster/costume/controller assignments, CPU levels, items and synchronized option values, without IP addresses or player names.
- The exact checksum components: match header, every fighter's identity, health/ki/blast, action, target/controller/lock-off, fusion/revival/assist fields, position/rotation float bits, libc RNG and lobby fingerprint.
- Additional diagnostic-only RNG state: MT19937's 624 words/index, scene/fighter generators and VU0 R. These extra values do not alter or extend the wire checksum.
- Recent actual received inputs and inputs used for prediction, with validity masks. Inactive producer slots are excluded. A missing record or mask means unavailable, not zero.
- Frame confirmation/check progress, recorded rollback counts and resimulation flags. Replayed ticks overwrite earlier predictions. A rewind invalidates abandoned future snapshots before rebuilding them. Round changes reset the capture history.
- RNG generator advances and wrapper requests: caller return address, coarse execution phase, local view, before/after state, result bits, per-kind ordinal and local replay branch. Per-tick counts and call-site aggregates survive event-ring overwrites; aggregate overflow and overwritten event counts are explicitly reported. Wrapper kinds annotate the requesting function and must not be counted as additional generator advances.
- The first captured round-entry RNG state, plus up to 32 pre-tick libc/VU0 seed events. MT events record its index and returned value; the full MT state remains in the state snapshots. One VU0 event represents one public Float01 request, including its original internal shifts.

State is captured at **tick entry, before that tick's inputs**. A state difference at tick T can result from earlier input or simulation work. The reported mismatch is a periodic checksum checkpoint, not necessarily the first divergent instruction. The first difference in this bounded window may already be downstream of an earlier cause.

RNG events attached to tick T describe work **after** its entry snapshot, potentially explaining the state at T+1. The last current-tick interval may be incomplete. Phase tags identify the most recent explicit coarse boundary (update, camera, draw, HUD, etc.), rather than a complete call stack. Local view numbers, sequence numbers and replay branches can legitimately differ between peers. Return addresses require the matching release map; a different build's symbols must not be used. At heavy load, exact earlier events may be overwritten: the report states those limits rather than inventing missing evidence.

The code reads state without invoking RNG generators, writing simulation structures, changing inputs, changing network packets, or replacing checksum values. Capture metadata is host-only and excluded from rollback snapshots. Differences in the extra RNG fields alone are investigation clues, not proof of the cause or necessarily a difference in the wire checksum.

## Comparing paired reports

The repository includes an offline comparison tool; testers do not need Python to generate reports:

```powershell
python port/tools/compare_net_desync.py host-net-desync.txt client-net-desync.txt
```

For Beta 15 caller names, download that release's Network Diagnostic Symbols ZIP and add `--symbols "Tag-Team-Recomp-Beta.15-Network-Diagnostic-Symbols.zip"`. The tool validates its build identity, then resolves return addresses to the nearest retained linker symbol plus offset. It compares complete tick intervals by generator counts, caller/phase aggregates and ordered generator events, excluding local view/branch/sequence differences. Overflow can limit attribution. Beta 14 reports remain readable.

It checks build/data/round/mismatch pairing, verifies reversed local/remote hashes, independently recomputes complete captured state checksums, and identifies the first recorded differing fighter field/component and input bytes. Extra RNG differences are reported separately. Reports from different events are rejected for state comparison. The tool does not send any files over the network.

## Validation and limits

Asset-free tests cover ring wraparound, all 15 supported actor indices, full RNG state, input validity masks, rollback replay/future invalidation, first-report behavior, round reset, disabled capture, and report-write failure. Paired synthetic reports identify an injected health difference at the exact actor/tick and recompute the captured checksum. The retained offline lifecycle tests check that native networking suppresses that offline checker.

The RNG regression harness compares instrumented production functions with retained Beta 14 sources across libc, VU0 and MT requests, checking output and generator state. Its shared float adapters test instrumentation equivalence, not independent PS2 arithmetic conformance. Additional tests exercise wrapper attribution, phases/views, event-ring and aggregate overflow, disabled tracing, replay invalidation, symbol-build pairing and incomplete interval handling. A Windows compiler harness extracts the same production function bodies to avoid linking unrelated math code.

Release notes record the completed Windows build, Windows capture harness and package checks. Matching DWARF/debug information, linker map, source/build digests and executable SHA-256 are included in the separate symbols archive. No PDB is emitted by this toolchain.

**Not yet verified on the development machine:** a real BT4 LAN/internet desync capture, correctness of techniques during network gameplay, and the root cause of the supplied desyncs. Those require the tester's local disc installation and paired network sessions. The next reports are evidence for that investigation, not proof that the desync is fixed.
