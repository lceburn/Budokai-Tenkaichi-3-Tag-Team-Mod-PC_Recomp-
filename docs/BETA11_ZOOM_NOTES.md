# Beta 11: zoomed effect coverage

Work is on `integration/z3xox-main`; main is unchanged.

## Report and cause

Modern Camera at 70% zoom showed a smaller rectangle containing effects, and impact sprites appeared cut at its edges. Script zoom updates the View's projection matrix, while the native `screenDist` field remains unchanged. `BtlCam_GetFrustumSize` used that old field to build the effect clip planes: after projection, the original frustum occupies only 70% of the displayed viewport in each dimension. The shrinking rectangle is an effect clipping mismatch, not a texture-pack border or intended camera frame.

## Correction

`Port_CamScreenDist` derives the displayed lens distance from `view2screen[1][1] / (aspectY * unk258)`, falling back to the native field for invalid values. The effect frustum and the three native perspective billboard paths use this distance. Native camera state and the zoom projection itself are unchanged. The query reads the current View, including authored and spectator views, rather than assuming the current fighter view owns the projection.

The regression executes production projection and zoom functions at 70%, 150%, repeated zoom and reset. It verifies that the effect frustum projects to the full viewport edges and that repeated reads do not compound zoom. Existing camera tests pass. BT4 gameplay acceptance still needs a repeat of the reported blast/skill at 70% zoom, plus 100%, split-screen and authored transitions.

## Damage tiers: still unresolved

The current BT4 resource profile maps an intact costume and one damaged costume file per outfit. `PortGame_Model` treats every nonzero variant as the same damaged file. TeamBT4's changelog names Tier 1 damaged appearances, but the available source and absence of disc assets here do not establish how light scuffs and heavier damage are encoded for each B14 REV2 costume. They could require internal model/texture states or different costume mappings; selecting variant 2 blindly would still return the same model in this build.

Beta 10's 50%-health damaged-costume swap is unchanged in this release. Progressive light/heavy health thresholds are not claimed implemented. A known character/costume with both appearances and inspection of that local disc resource are needed before wiring additional tiers. No new game assets or guessed file IDs are introduced.
