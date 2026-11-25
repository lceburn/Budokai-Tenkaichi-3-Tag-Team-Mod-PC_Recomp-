# Stock taunts and emergency wave controls

The F1 Cheats Emote control has an enable checkbox that retains the selected
animation when disabled. The existing L1 + Circle / LB + B chord stays the same.
The list includes the added emotes, the current fighter's stock idle animation,
and named stock-character idle taunts from the prepared local disc catalogue.
Stock selections use both spare controller bytes so donor IDs above 255 are
shared without truncation. Both computers must use the updated build.

Stock motions are extracted from the original idle-taunt slot (0x185), bounded
before decompression, checked before sampling, and drawn through the existing
presentation-only emote path. Fighter matrices are restored after drawing.
No stock game animations are shipped in source or release binaries. The native
motion cache has sixteen reusable 64 KiB buffers, limiting allocation when
players repeatedly change their selection. Missing local data fails closed.

The emergency wave uses two separate physical L1 / LB presses within the
original seven simulation-tick window. Holding the shoulder button is not a
second press. Face-button or other shoulder-button combinations invalidate
the pending tap, keeping the emote chord separate. The input state is included
in rollback. CPU injection and replay playback retain the original command
path. This only changes the physical input route: the original action 0x42,
damage-reaction eligibility and 5,000-health cost remain unchanged.

Ultimate movement and cinematic pause rules are outside this update. Existing
canonical targeting and deterministic network loading are unchanged.

Focused input checks passed double taps, holds, late taps, emote chords,
independent actors, disabled/inactive state and rollback replay. Real local
data decoded all 241 stock idle taunts; truncated payloads were rejected.
The frozen Windows build passed rendered stock-taunt activation/cancellation
and Square Photo Mode camera/view/menu/resume checks. The live CPU check observed the original wave action and an exact 5,000-health
deduction. A paired run displayed the guest's independently selected stock
taunt on both clients for 3,600 synchronized frames. A separate 4,800-frame
CPU-combat run observed the physical double-tap wave on both clients without
a desync. The different-camera targeting regression also passed. Both signed
archives and an isolated upgrade passed, preserving all 85 checked character,
save, settings, mod and deletion-recovery files.
