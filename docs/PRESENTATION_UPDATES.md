# Beta 7 presentation and update changes

Work branch: `integration/z3xox-main`. `main` is unchanged.

## Camera mods

The integration deliberately skipped Lua cameras in tag battles because the original bridge stored two camera histories. The bridge now supports all 15 actor/view slots, exposes `cam.view` and `cam.actor`, and supplies the actual camera owner's health and target health. Bundled camera scripts use independent history for each view and reset it when ownership changes. Zoom and collision preferences now cross the host bridge correctly. Online view remapping saves and restores the local view's zoom caches.

Enable one camera mod at a time under F1 > Mods to make its sliders predictable. Both enabled scripts run in sequence. Technique cuts retain the game's framing. Spectator camera modes override these fight camera mods while active.

## Modern HUD

Tag-team gauges and combo widgets draw through separate native trees. Those trees now emit the same replacement scope markers as the original two-player HUD. A Lua HUD requesting `hide_game("gauges")` or `hide_game("combo")` suppresses those original pieces. The timer already uses the original scoped tree. Tag-team status information remains available.

The bundled Modern HUD still presents the two-side script API. It replaces the original gauges in single-view play; split-screen automatically keeps the native HUD so no player's gauges disappear. A Modern HUD for every split-screen viewport is a separate layout task.

## Spectator camera

Adapted camera math, host controller, UI controls and regression tests from [NauTaro/DBZ-BT4-Tag-Team-Decomp](https://github.com/NauTaro/DBZ-BT4-Tag-Team-Decomp/tree/e113f0da1737318304866425d50aa4092abefac7), commit `e113f0da1737318304866425d50aa4092abefac7`, under AGPL-3.0. Original attribution is preserved in adapted files; the repository's AGPL license applies.

F1 > Spectator offers Game camera, Free camera, Follow fighter, and Fight overview, plus target, speed, zoom and reset. It supports this branch's 15-actor practice capacity. WASD moves, Q/E moves vertically, arrows turn, Shift accelerates and +/- zooms. Controller left/right sticks move/turn, shoulders move vertically, triggers zoom and L3 accelerates.

Camera input replaces player one's local fighter input while active, with Start/Select retained. Opening F1, losing focus, typing chat or opening the pause menu releases camera input capture appropriately. Online tag-team participants retain their fighter cameras; observers can use the new controls. The spectator view is applied during drawing, then the selected view and scene state are restored. It is not included in rollback memory. Orbit cameras use the branch's stage trace; free flight can pass through geometry.

## Windows automatic updates

The installed `Play.exe` hands off to a temporary copy of `CheckUpdates.exe` before starting the game. The helper checks only this fork's releases, stays on the installation's stable or experimental channel, and offers newer signed updater archives. It checks archive size, optional GitHub SHA-256 digest, pinned Ed25519 signature, archive paths, and the payload's per-file checksums before invoking the existing transactional updater. Unsigned releases are never installed automatically.

The Windows downloader uses [WinHTTP with system TLS certificate validation](https://learn.microsoft.com/en-us/windows/win32/api/winhttp/nf-winhttp-winhttpopenrequest); it does not depend on the frozen Python runtime's OpenSSL setup. The game starts after accepting, declining, or failing the check. Updates cannot replace files while the game is running. Saves, settings, imported assets, texture packs and edited/custom scripts are preserved. Unmodified bundled scripts are upgraded so camera fixes reach existing installs. If startup tables change, the helper asks for the installation's original local ISO; no disc data is uploaded.

F1 > Misc > Check for updates controls the next launch's check. `Play.exe --check-updates` forces a manual check without starting the game. Creating `disable-auto-updates.txt` beside Play.exe also disables automatic checks. Network/check failures are recorded in `update-check.log` and do not prevent play.

Beta 6 needs one manual updater installation to gain this launcher/helper. Future updates must attach both the updater ZIP and a `.zip.sig` Ed25519 signature to a tag such as `v0.1.0-beta.8-experimental.1`. The package's `release_sequence` must match the tag's final number (currently packaging defaults to 1). The public key lives in `port/tools/native_update_key.py`; the private seed is kept only in ignored `port/build/private/native_update_seed.txt`. Back it up securely before cleaning build outputs. Never commit or distribute the private seed. Sign archives with `update_key.py sign --any-key --key-file <private-seed-file> <updater.zip>`.

## Validation limits

Automated checks cover camera math, host input isolation, rollback exclusion, extra-view camera bridge behavior, Lua scripts, signed archive rejection, safe extraction, installer contents, and transactional update preservation/recovery. No compatible disc is present on the development computer. Actual BT4 B14 REV2 camera framing, multi-view HUD appearance, online observer behavior, and launcher-to-install end-to-end updates still require gameplay testing on the installation computer.
