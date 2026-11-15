# Beta 8 camera changes

User reports from Beta 7: non-game spectator modes show a black screen; overview briefly appeared much too far away with two fighters. Requested: cycle all character views, free camera and overview after defeat; vertical right-stick look and adjustable look speed while locked off.

## Black views and framing

`BtlCam_InitViews` returned early in tag mode before initializing `gBtlCam->layouts`. The spectator renderer copied layout zero, including a zero screen distance and invalid near/far projection values. All three templates are now initialized before constructing tag-team views. A regression executes the production initializer and checks finite full-screen/split-screen matrices and all four viewports.

Spectator framing now derives its field of view and aspect ratio from the actual viewport. Overview excludes eliminated and fused-away actors, and a regression bounds its distance with two nearby fighters. Invalid projection values cause a fallback rather than being used to build another camera.

## Death spectator cycle

The existing target/spectator button (default L3) cycles through watchable native character views, Free camera, Fight overview, then the first character. The existing Watch Fallen Fighters preference still controls eligible character views. Each local split-screen view keeps its own camera state and input slot. Online participants use their local player's spectator mode after defeat; room observers retain the F1 controls.

The cycle's integer mode and selected actor remain part of match/replay state; presentation poses and raw camera controls remain local. Free/overview input capture preserves Start, Select and the configured cycle button. Teammate takeover is available in character view, preventing a takeover of an unrelated fighter while free-flying. Revival returns to the controlled fighter view. HUD labels identify free camera and overview.

## Unlocked look

While locked off, the right stick controls horizontal yaw and vertical tilt. Mod Settings > Controls adds separate Unlocked Look Horizontal Speed and Unlocked Look Vertical Speed sliders (25–300, default 100). Vertical tilt is bounded and resets when locking on again or starting another battle. Locked-on camera input retains its original behavior.

The camera yaw also defines movement direction in the original game, so these speeds use the room host's agreed control settings online. They are not unsynchronized per-client gameplay changes. All room participants must use the new build.

## Validation

Checks cover the production tag projection initializer, production unlocked camera functions, cycle ordering with skipped actors, per-player capture and cycle-button passthrough, camera reset, rollback isolation, two-fighter overview distance, Lua mods and HUD rendering. The Windows engine and shaders are built using the existing release container. No disc assets are present on the development computer: BT4 B14 REV2 gameplay and simultaneous local/online camera acceptance still require the installation computer.

Work remains on `integration/z3xox-main`; `main` is unchanged.
