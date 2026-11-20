Beta 17 experimental: native creator save correction
====================================================

The released creator could generate a preview on a new installation but could
not publish its first character: saving assumed gamedata/mods and enabled.txt
already existed. The development libraries used by the previous runtime checks
already contained these files.

The native save path now initializes an empty library using exclusive file
creation. It preserves an existing activation index and refuses to replace or
silently rebuild a missing index when character folders already exist.

The menu exposes only implemented native controls. Separate melee overrides,
custom donor costumes, part removal, extra layers, sizing, portraits, and other
unimplemented controls are unavailable. Supported attack names, effects, aura,
colours, parts and form links remain available. These are capability checks,
not a guarantee for every possible mesh combination.

A rejected save keeps the draft, restores its previous preview where available,
returns to the editor when the notice closes, and reports unsupported settings
or missing source data instead of asking for the reference executable.

Validation: native transaction and first-library tests with sanitizers; every
exposed control passes the recipe capability gate; Windows first save from a
missing library, fight, and return; reported mixed-head recipe generation,
publication and GPU fight. No helper is required for these tested paths.

This does not establish why a tester's earlier character directory is absent,
and does not claim to restore missing characters. Preserve existing folders.
The missing spaceship reported in an attract demo is a separate unresolved
report. Clearer setting descriptions and animated aura/moves previews are
deferred. Ordinary-fighter networking, prior crash fixes and diagnostics remain
included; custom fighters remain offline.
