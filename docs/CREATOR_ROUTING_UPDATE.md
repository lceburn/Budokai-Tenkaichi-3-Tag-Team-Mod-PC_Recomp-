# Creator routing and spectator controls

Explicit Aura Source takes priority over inherited aura abilities. Default keeps native Move Set aura behavior. The choice is carried in the model's animation metadata, including when that model is shared online. Aura black (`000000`) is a valid color.

Default stance uses Move Set animations and standing stream, scaled to the body. An explicit Stance donor replaces those animations and its standing stream. Body geometry stays independent. Effects changes and aura-color edits preserve camera choices; both Blast 2 loop-camera selectors now override their attack donors.

Six saved custom characters/forms are supported. Existing IDs remain stable; the sixth uses 256 to avoid the game's 255 no-character sentinel. Four direct transformation links per fighter and the nineteen-form room budget are unchanged.

Whole-head assembly keeps ordinary body texture bindings separate from auxiliary fade uploads and relocates fade indices and aliases. Retained material pixels and palettes were compared against original body uploads across twelve Zangya, Broly and Caulifla head swaps. The 241-donor compatibility matrix retained its prior 230 successes and eleven unsupported combinations.

After updating, select an existing affected character and use Save all to rebuild its older generated resources without changing its recipe or selected identity. The refresh marker is published atomically with those resources. New characters use the updated builder immediately. Surface-color choices require saving pending part changes first.

F1 > Spectator offers Button passthrough while in spectate mode, off by default. In manual Free, Follow or Overview views, enabled passthrough forwards fighter buttons while analog sticks remain camera inputs. Select remains reserved when camera cycling is enabled. Shoulder buttons, triggers and stick clicks no longer also manipulate the camera. Game camera restores normal input. Frozen Photo Mode and automatic death-spectator views retain their existing input rules.

Verification covers production override assembly, runtime aura precedence, all exposed animation groups, loop-camera preservation, six-slot routing/export/install/cleanup, sanitizer-backed room lifecycle, saved-character refresh, spectator controls and live Windows rendering and transformations. These checks do not establish support for every donor combination.

## Surface recoloring

Recoloring reads original palettes and shading ramps rather than transforming shared resources repeatedly. Surfaces with different edits receive independent shading bindings; identical results share one upload. Unchanged assignments preserve the complete original model. Mesh and facial-animation geometry is preserved while shading references can change.

The first displayed surface retains its saved recipe key. Additional palettes using the same pixel pattern receive distinct keys, allowing independent edits without repurposing the original selection. Auxiliary fade uploads are no longer listed as editable surfaces. Facial-expression palettes receive the exact color transform selected for the assembled face slot.

Existing generated resources from builder versions 47 through 49 refresh to version 50 when the selected character is saved, including an unchanged Save all. Recipe content, routed filenames, form links, selected identity and unrelated library entries are preserved. Capacity failures reject the edit before publishing any model data.

The sanitizer regression checks byte-exact identity edits for all 241 donor models, single-surface edits for the 240 donors with structurally valid upload tables, and distinct material identities for all 23 palette-alias donors. One pre-existing malformed upload table remains rejected for actual edits. Multi-surface tests compare each material's mesh and facial-animation shading against an isolated edit and verify request-order independence and unchanged geometry. Costume/damage generation and a mixed Broly/Goku head are also covered.

### Blink and speech replacements

All alternate eye and speech textures are uploaded into the assembled model's `faceTex` slot. Their stored donor addresses do not identify their destination after head assembly. Facial recoloring now follows that same runtime slot, including replacement textures whose addresses still name their original donor. Unrelated body palette edits leave those facial sets untouched. The actual face selector is tested for default, both blink frames and all four alternate speech sets; mixed-head palette regressions cover Kefla, Kefla Super Saiyan, Goku (End), Zangya and Kibitokai across intact/damaged costumes. Builder version 50 refreshes saved resources from versions 47–49 on Save all.
