# Beta 9 camera and HUD changes

Work is on `integration/z3xox-main`; `main` is unchanged.

## Camera selection and lifecycle

F1 > Spectator offers **Use Select to cycle camera**, disabled by default. During combat, Select cycles Game camera → Free camera → Follow fighter → Fight overview → Game camera. When enabled, Select is reserved for this cycle instead of the original distance preset. It uses the first local controller; the existing per-player death-spectator button still works independently.

An explicit F1 selection takes priority over automatic death-spectator views, including selecting Game camera. Using the death-spectator cycle returns to automatic selection. Battle reset/termination restores temporary camera pointers before clearing host camera state, and the next match starts in Game camera. Custom views retain their existing local fighter-input capture; Start remains available.

## Follow and overview

Follow fighter now uses the selected fighter's native camera pose, including its terrain handling and authored camera behavior. It no longer constructs a separate follow orbit. The target selector uses the full actor count, independently of the alive-fighter list used by overview.

Overview uses body height for framing bounds rather than a collision/culling radius, reduces excess framing space, and immediately starts at the fitted distance. At 100% zoom it fits the live fighters; increasing zoom now gives a genuinely closer view instead of moving backward to cancel the zoom. Higher zoom can crop fighters. Stage collision traces receive a position with homogeneous component 1 rather than a bounding radius.

## Stage-destruction presentation

Custom spectator views yield during the stage-loading transition. The online/shared draw path now preserves the actual authored render View: DemoCam can install a different View without replacing the current fighter camera. Copying the fighter View previously replaced the projection used by transition particles. These changes address concrete camera interference with the planet explosion, but the explosion itself still requires BT4 B14 REV2 gameplay verification; there are no disc assets on the development computer.

## Hide all HUD

The master setting now gates the Lua HUD renderer, scoped native HUD primitives, and native panel bars in addition to the existing native/tag HUD checks. Modern HUD and other Lua HUD scripts obey it without being disabled individually. F1 settings remain visible.

## Validation and acceptance

`port/tests/presentation_checks.sh` covers camera fitting and zoom, Select cycle edges and ordering, manual Game selection priority, rollback isolation, battle cleanup, native follow target mapping, authored View preservation, temporary draw restoration, production viewport initialization, unlocked look, Lua mods, and both native and Lua HUD hiding.

Runtime acceptance on the installation computer must check all four modes in a real match and attract demo; two-fighter overview framing; Select cycling during combat; ending/restarting a match from every custom mode; F1 Game selection after defeat; stage destruction with Game and custom views; and Hide all HUD with Modern HUD and the texture pack enabled. Local split-screen and online matches also need gameplay acceptance before calling this a stable release.
