# Beta 12: Rematch effect-lifecycle diagnostic

This is an experimental Windows x64 diagnostic update for BT4 B14 REV2. It does **not** claim to fix the second-fight Rematch/Retry crash. Gameplay and effect lifetimes are unchanged when tracing is disabled. Main is unchanged; development stays on `integration/z3xox-main`.

## Enable and reproduce

1. Close the game and install this update through Play or the extracted updater. Keep the whole extracted payload beside Update.exe.
2. Open the **engine** subfolder of the installed game folder and double-click `Enable effect diagnostics.cmd`. It creates `effect-diagnostics.enabled` in the parent game folder beside Play.exe. Alternatively create that empty file yourself. No separate checker or disc upload is needed.
3. Start Play again. Tracing is read once at process startup. Test **offline**: host sidecar generations are deliberately not restored by network rollback. A rollback attempt stops with an explicit unsupported-mode diagnostic.
4. Record the game mode, both teams, fighter names/forms/costumes and controllers. The report also records numeric fighter, character/form, costume, variant, team and controller IDs for up to 15 fighter indices. Please supply the human-readable names alongside it.
5. Complete the first fight and select Rematch/Retry. Leave the controls idle initially, matching the reported crash. If it continues, try the same techniques and another rematch. Separately test returning through character selection.
6. The first invalid relationship deliberately stops the process with exception **0xE0425443** after saving `effect-lifecycle.txt` in the installed game folder. This may stop earlier than the original access violation: that is intentional. Preserve the file before another run overwrites it.

To disable tracing, close the game and delete `effect-diagnostics.enabled`. Also unset `BT3_EFFECT_DIAG` if you explicitly set that environment variable to `1`. There is no silent state repair or effect skipping in diagnostic mode.

## ProcDump capture

Use your existing ProcDump installation. Create the destination folder first. Start monitoring **before** launching Play (or attach while at the main menu). Monitor **TagTeam.exe**, not Play.exe:

```powershell
New-Item -ItemType Directory -Force 'D:\TagTeamDiagnostics\Dumps\EffectLifecycle'
.\procdump64.exe -ma -e 1 -f 'E0425443,C0000005' -n 1 -w TagTeam.exe 'D:\TagTeamDiagnostics\Dumps\EffectLifecycle'
```

The filter includes both the intentional diagnostic exception and the original access violation. A C0000005-only filter misses the diagnostic stop. Full dumps can be several GB; allow time for capture to finish. ProcDump's documented first-chance and include-filter options are described in [Microsoft's ProcDump documentation](https://learn.microsoft.com/en-us/sysinternals/downloads/procdump).

Return the small `effect-lifecycle.txt`, the observed mode/teams/names and the reproduction steps privately in this chat. Keep full dumps, saves, disc files and session data private and off GitHub. An ordinary access violation may also produce `bt3_crash.txt`; the intentional stop uses its own lifecycle report instead.

## What the build records

- A bounded 4,096-event memory ring, flushed at the first violation or best-effort on an ordinary crash. No per-frame disk log.
- Monotonic event sequence, match generation, simulation frame, reset mask/reason, task/list/child pointers and generations, class, owner tags and allocation results.
- List allocation/destruction; task init, removal and address reuse; pool reset; character effect readiness; slot binding/invalidation; and reset/rebuild boundaries. Removal records retain the caller address, sequence, frame and match even if the ring wraps.
- Fighter index, active character/form ID, costume/variant, team/controller, technique slot, effect type and move ID. Names require the tester's notes rather than proprietary resource publication.

Before EftShot_Start changes slot arguments or creates an instance, validation checks registered allocation ownership before inspecting task memory. It detects stale/free tasks, task/list generation mismatches, wrong class/list, active/free chain inconsistencies, missing/replaced child lists, and requests before character effects are initialized. Reset/rebuild boundaries validate retained bindings too. Metadata is host-only; ABI-sensitive game structures are unchanged.

The report's `last_removal` caller and matching event sequence identify the cleanup operation. Subsequent `task-init/reuse` and `slot-bind` events establish whether a replacement was created and bound before the request. Use the matching linker map and DWARF symbols, because addresses from older releases are not interchangeable. Sidecar-capacity exhaustion is also an explicit stop, not an inferred game lifecycle failure.

## Provenance and investigation limits

The reported Beta 11 executable SHA-256
`75a6fdbbcdb9ca104711a19d06268c2ee7e29a801b70d25c91e2cc359cd8015b`
exactly matches the retained Beta 11 packaged executable built at source
`7f0836f8d99b23c9aad4c8eaf79db2a0c5b0c53c`. The older source reference
`ef016d6e088e3d237d2de27f9c0bc63233b9354b` is not that build's source.

This update adds diagnostics to the Beta 11 integration baseline, not an unconditional null check or a guessed rematch fix. Inspection shows that scene resets recurse through task reset callbacks and can kill marked tasks; character teardown can reset effect pools; slot creation depends on character effect initialization. The capture must determine which path actually invalidates this group. Whether groups should survive reset or slots should be invalidated/rebuilt remains unresolved pending that evidence.

The separate diagnostic symbols archive contains the exact executable's DWARF information, linker map, build identity/source digest and executable SHA-256. This MinGW/Clang pipeline emits DWARF rather than a PDB. Keep it paired with this release.

## Validation

Automated fixtures use the production task-list implementation and diagnostic module. Coverage includes removal leaving a bound slot, same-address/class task reuse, 12 reset/rematch cycles across all 15 supported fighter indices, character-selection teardown/rebinding, pool invalidation, pre-init requests, unknown pointers, and child-list destruction/replacement. Instance creation/reset callbacks run in the fixture; this does not validate actual technique visuals or gameplay.

Windows fixtures use the same ptr32-transformed game ABI as the engine. A separate Windows harness verifies that a real first violation writes its report and exits with 0xE0425443. Release notes record completed build/package checks.

**Not run on the development machine:** actual BT4 fights, real rematches, character-selection gameplay, technique visuals/behavior, or a ProcDump capture of the game. Those require the tester's local disc data. A successful fixture run alone does not establish that the game crash is fixed.
