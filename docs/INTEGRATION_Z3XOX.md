# z3xox integration, 2026-11-14

## Refs and merge topology

Repository: `lceburn/Budokai-Tenkaichi-3-Tag-Team-Mod-PC_Recomp-`.
[PR #1](https://github.com/lceburn/Budokai-Tenkaichi-3-Tag-Team-Mod-PC_Recomp-/pull/1)
was a draft audit comparing an upstream snapshot with the integration baseline.
It was not a reconciled implementation.

| Role | Ref | Commit |
| --- | --- | --- |
| Common ancestor | Tenkaichi3Decomp 0.1.37 | `ec3c85a98cffd9cd22c22ac14cd62fb584d23664` |
| Tag-team baseline, merge first parent | `origin/integration/z3xox-main`, also `origin/main` at inspection | `ef016d6e088e3d237d2de27f9c0bc63233b9354b` |
| Upstream, merge second parent | `origin/audit/z3xox-upstream-2026-11` | `48d66c1f66fdcaf71428a06c9fa9eb036e994e2b` |

The live `z3xox/Tenkaichi3Decomp` `main` was checked with `git ls-remote` and
matched the snapshot. The baseline has 13 commits after the ancestor; upstream
has 216. The result is a true two-parent merge on `integration/z3xox-main`.
Neither local nor remote `main` is part of the write operation.

## Resolutions

| Area | Result |
| --- | --- |
| Networking | Keep `TagNet` dispatch ahead of upstream input/spectator handling. Retain ten-player direct rooms, practice, spectators, immutable match configuration, source/build identity and host speed. Retain upstream's separate two-player lobbies, rollback and spectator transport. Apply upstream lobby result-menu ownership only outside TagNet rooms. |
| Rendering | Combine tag viewport/HUD markers with upstream widescreen menu markers, shader/render improvements and script look passes. Keep tag runtime screenshot filenames. Apply camera culling expansion after computing the actual tag viewport frustum. Keep projection/zoom caches separate for every native camera, rather than aliasing views with `index & 1`. |
| Input | Preserve four keyboard/controller profiles, seat assignment, clash controls and four rumble slots. Use upstream rumble stall detection in all four slots. Expand the newly introduced press-age history to four seats to avoid an out-of-bounds write for players 3 and 4. |
| UI | Keep tag HUD/name helpers, Spanish/BT4 resources, tournament/practice visibility, showcase behavior and Mod Settings. Retain upstream Online, Mods, Tech and accessibility UI. Settings navigation has 10 ordinary tabs or 11 with Tag Team; both controller cycling and the in-game Mod Settings request use the correct index. Preserve translation before widescreen text measurement; combine larger upstream catalog size with tag group capacity. |
| Audio | Keep the expanded ADX/voice/effect pool, tick-based network sound state, BT4 songs and opening movie. Use the shared upstream gain function for SDL and miniaudio; classify every actor voice through `TAG_ACTOR_CAPACITY`, including practice actors beyond the first ten. Retain the EU opening soundtrack fallback for BT3. |
| Battle | Preserve multi-target contact/damage, tag spawning, fusion/giant rules and BT4 transformation costs. Invoke the optional upstream Z Counter hook inside each established contact pair, before its normal resolution, and stop that contact when the hook handles it. Keep upstream gameplay policy and counter snapshots in the rollback-state whitelist. These optional Tech rules remain offline-only. |
| Distribution | Keep fork install/disc instructions, native launchers and save-preserving updater. Upstream's automatic updater does not check or install updates during a `BT3_TAG_TEAM` launch, preventing replacement with upstream archives. Keep release/macOS workflows under the disabled `workflow-examples` directory. |

No conflict-marker blocks remain. No tag-only tracked file was removed by the
integration. This is a source integration, not a validated release binary.

For the subsequent full Windows build and current installer blocker, see
[Windows build follow-up](INTEGRATION_BUILD.md).

## Validation

Run from the repository root. Generated diagnostics and downloaded SDL 3.4.12
headers are local, ignored files under `port/build/`; no disc data was used.

- Clang 18 syntax checks: **315/315** game sources after `portsrc.prepare()`;
  311 checked as freestanding 32-bit C, four using 64-bit syntax fallback because
  WSL lacks 32-bit libc headers. Existing decompilation return/type diagnostics
  were suppressed as in the build pipeline. This does not validate the 64-bit
  pointer-rewriting/soft-float ABI.
- **47/47** renderer, audio, network and UI sources plus selected game-facing
  port sources passed host syntax checks with SDL 3.4.12 headers. Shader and bar
  include declarations were stand-ins for this syntax check only; no shader
  compilation or GPU validation was performed.
- Python tests passed: `test_net_build.py` (7), `test_native_disc.py` (3),
  `native_setup.py` (8 passed, 1 skipped), `native_update.py` (7),
  `native_play.py` (2). The POSIX launcher setup check was skipped on Windows;
  real-disc gameplay tests were not run.
- Executable regressions passed: `tag_view_layout.c`, `tag_clash_sequence.c`,
  `tag_lobby_rules.c`, and `settings_store.py` (500 settings/bindings).
- New production-code regressions passed under AddressSanitizer:
  `tag_input_slots.c` and `tag_audio_gain.c`. These use small SDL call stubs and
  linker section collection, rather than requiring a window or audio device.
- `tag_hud_render.cpp` could not link because this environment lacks the SDL3
  runtime library. Its production implementation passed the port syntax check.

For the new tests, with SDL installed (replace the include path if necessary):

```sh
clang -O0 -g -fsanitize=address -ffunction-sections -fdata-sections \
  -Wl,--gc-sections -Iport/build/SDL/include -Iport/third_party/imgui \
  port/tests/tag_input_slots.c -o port/build/test_tag_input_slots
port/build/test_tag_input_slots
clang -O1 -g -fsanitize=address -ffunction-sections -fdata-sections \
  -Wl,--gc-sections -Iport/build/SDL/include \
  port/tests/tag_audio_gain.c -lm -o port/build/test_tag_audio_gain
port/build/test_tag_audio_gain
```

## Decisions and release gates still open

1. **Network product direction.** TagNet TCP rooms and upstream's two-player
   lobby/matchmaking transport remain distinct. Combining room codes, relay,
   ranked rules or chat with ten-player rooms needs a protocol decision and
   multi-process tests; this merge does not claim interoperability between them.
2. **Script camera API.** Upstream scripts describe two fighters. Camera
   overrides are kept for ordinary modes and bypassed during tag-team battles
   so they cannot replace native shared/cinematic/spectator cameras. Decide how
   scripts should identify arbitrary actors, teams and views before enabling
   them there. Script HUD/game feeds also retain their upstream two-side schema.
3. **Optional Tech rules.** Z Counter and Bombardment remain off by default and
   unnegotiated online. Validate multi-contact ordering offline before endorsing
   them for tag modes. Upstream's counter snapshot code caps its roster at ten;
   it deliberately declines larger practice rosters. Supporting all fifteen
   practice actors and negotiating these rules online remain separate work.
4. **Releases and updates.** Select the mod release repository, asset names,
   signing key and installer layout before adopting the upstream automatic
   updater. The existing native updater remains the supported mod path. macOS
   source support is imported, but no macOS Tag Team package has been validated.
5. **Build and gameplay acceptance.** A full Windows/Linux build with the
   documented Clang/LLVM 22, SDL3 and shader tools is still required. Run BT3/BT4
   resource imports, four-seat split screen and fusions, cinematic cameras,
   rendering/interpolation on Vulkan and OpenGL, 10-player + 10-spectator rooms
   under lag, and audio/voice routing with owned disc assets. Rebuild network
   identities and regenerate/verify stripped installer table recipes for the
   resulting binaries. Do not ship the previous binary's table recipes merely
   because they merged without a textual conflict.
