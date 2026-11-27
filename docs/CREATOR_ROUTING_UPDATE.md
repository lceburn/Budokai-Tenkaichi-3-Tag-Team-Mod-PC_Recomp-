# Creator routing and spectator controls

Explicit Aura Source takes priority over inherited aura abilities. Default keeps native Move Set aura behavior. The choice is carried in the model's animation metadata, including when that model is shared online. Aura black (`000000`) is a valid color.

Default stance uses Move Set animations and standing stream, scaled to the body. An explicit Stance donor replaces those animations and its standing stream. Body geometry stays independent. Effects changes and aura-color edits preserve camera choices; both Blast 2 loop-camera selectors now override their attack donors.

Six saved custom characters/forms are supported. Existing IDs remain stable; the sixth uses 256 to avoid the game's 255 no-character sentinel. Four direct transformation links per fighter and the nineteen-form room budget are unchanged.

Whole-head assembly keeps ordinary body texture bindings separate from auxiliary fade uploads and relocates fade indices and aliases. Retained material pixels and palettes were compared against original body uploads across twelve Zangya, Broly and Caulifla head swaps. The 241-donor compatibility matrix retained its prior 230 successes and eleven unsupported combinations.

After updating, select an existing affected character and use Save all to rebuild its older generated resources without changing its recipe or selected identity. The refresh marker is published atomically with those resources. New characters use the updated builder immediately. Surface-color choices require saving pending part changes first.

F1 > Spectator offers Button passthrough while in spectate mode, off by default. In manual Free, Follow or Overview views, enabled passthrough forwards fighter buttons while analog sticks remain camera inputs. Select remains reserved when camera cycling is enabled. Shoulder buttons, triggers and stick clicks no longer also manipulate the camera. Game camera restores normal input. Frozen Photo Mode and automatic death-spectator views retain their existing input rules.

Verification covers production override assembly, runtime aura precedence, all exposed animation groups, loop-camera preservation, six-slot routing/export/install/cleanup, sanitizer-backed room lifecycle, saved-character refresh, spectator controls and live Windows rendering and transformations. These checks do not establish support for every donor combination.
