# Windows build follow-up

The source integration is published on `integration/z3xox-main`. `main` remains
unchanged. This follow-up builds that integration; it is not a release announcement.

## Experimental installer without the standalone checker

The standalone checker is optional and is no longer a prerequisite. At the
user's request, an experimental installer can defer compatibility verification
to normal Setup using `package_native_setup.py --experimental-startup`. Its
archive name, readme and release metadata explicitly identify it as experimental.
The normal packaging path still rejects recipe coverage gaps, and malformed
manifests are rejected in both modes.

This option does not change runtime checks: Setup still reconstructs and verifies
the startup-table checksum against the player's local ISO before extracting data
or creating an installation. The updater retains the same verification. A
checksum mismatch stops installation; this option cannot make missing mappings
work and does not establish compatibility or gameplay readiness.

The standalone checker was reported blocked by the user's browser and Windows.
It is not required, and no security settings were changed or bypassed. A newly
packaged unsigned installer has not been verified as accepted by those systems.

The experimental package uses beta version `6-experimental`; it is not a stable
Beta 6 release. Regression checks cover experimental labelling, rejection of
corrupt manifests, strict normal packaging, and stopping a wrong startup checksum
before any installation or disc extraction occurs.

## Completed

- Built the repository's isolated Ubuntu 22.04 compiler image with Clang/LLVM
  22.1.8, matching libclang bindings, SDL 3.4.12, real shader compilation and
  the MinGW-w64 Windows cross-compiler.
- Compiled 490 objects and linked `port/build/bt3.exe` successfully. The game
  data tables were built blank; no ISO or extracted game assets were used.
  Generated the accompanying state map and startup manifest, SDL DLL, and Play
  launcher. The executable starts on Windows and exits with the expected missing
  disc-data message and status 2 when given a nonexistent data folder.
- The link reports 103 generated fail-fast stubs for unimplemented routines,
  predominantly inherited PS2 libraries. Linking alone does not prove that a
  gameplay path cannot reach one. Real gameplay tests are still required.
- Passed the production tag HUD test against the actual SDL library, script-mod
  tests under address/undefined-behavior sanitizers, and the UI translation test.
- Fixed native packaging to include the replacement audio library's license and
  other newly imported dependency licenses. Bundle the upstream example scripts
  on fresh installs; the native updater skips player-owned `mods/` files.
- Passed the setup suite (8 passed, 1 POSIX launcher test skipped on Windows),
  updater suite (8), package-resource smoke test (1), and startup checker tests (3).

Reproduce the engine build from Linux/WSL with Docker:

```sh
sh port/release/build_native_windows.sh
```

This command compiles the engine and launcher, not the final installer. The
generated files and logs stay under the ignored `port/build/` folder.

## Installer gate

The new skeleton build's manifest requests 210,773 bytes in 2,388 runs from 69
tables. Existing native startup recipes leave 706 manifest records without full
explicit coverage, for each supported disc profile. The normal installer packager
rejects this gap. Experimental packaging can defer the compatibility decision to
the mandatory install-time checksum check, as described above. No stable
installer has been published.

The recipe reconstruction starts with zero-filled tables. A static coverage gap
does not establish whether the player's reconstructed tables will match the new
manifest's checksum. Check that on the computer holding the supported disc
before deciding whether the gap is only implicit zero data or requires refreshed
disc mappings. Do not assume either outcome without this check.

The standalone `Check-BT4-Startup.exe` transfer bundle contains the checker and
the exact build manifest. It is a diagnostic tool, not a game installer:

1. Extract the complete bundle on the Windows computer holding the BT4 B14 REV2 ISO.
2. Run the checker and choose the ISO. It reads the image locally without installing
   or modifying the game and without contacting a network service.
3. Save `startup-check.json` and return that report. It contains the supported disc
   profile, build-manifest digest and compatibility result; it contains no ISO
   path or disc contents.

The frozen checker passed a Windows command-line smoke test that verifies an
invalid manifest is rejected and a report is written. Its synthetic match,
mismatch and unsupported-disc tests also pass. Visual inspection of its packaged
file-picker dialog was not completed because Computer Use app access timed out.

The source can be run with Python and pycdlib instead of the standalone checker:

```sh
python port/tools/check_native_startup.py --iso "your-disc.iso" \
  --manifest port/build/bt3.dat --report startup-check.json
```

A compatible result validates startup tables only. Full BT4 gameplay, four-seat
input, camera/fusion behavior, audio and multiplayer acceptance tests remain.
