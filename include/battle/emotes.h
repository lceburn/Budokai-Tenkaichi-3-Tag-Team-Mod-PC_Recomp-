/* Adapted from NauTaro/DBZ-BT4-Tag-Team-Decomp, a5e13c9a1dcd4b5847746ca330e55f9796b3941f, AGPL-3.0. */
#ifndef BTL_EMOTES_H
#define BTL_EMOTES_H

enum { BTL_EMOTE_COUNT = 27, BTL_EMOTE_MIXAMO_FIRST = 23, BTL_EMOTE_PAD_BYTE = 16 };
#define BTL_EMOTE_SETTING "battle_emote"

#ifdef __cplusplus
extern "C" {
#endif
void BtlEmote_Reset(void);
int BtlEmote_Input(int object, unsigned buttons, int idle, int selected);
int BtlEmote_Pose(int object, float *frame);
#ifdef __cplusplus
}
#endif
#endif
