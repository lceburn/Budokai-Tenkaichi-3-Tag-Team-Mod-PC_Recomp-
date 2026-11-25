/* Adapted from NauTaro/DBZ-BT4-Tag-Team-Decomp, a5e13c9a1dcd4b5847746ca330e55f9796b3941f, AGPL-3.0. */
#include "common.h"
#include "battle/emotes.h"
#include "battle/btl_input.h"
#include "battle/btl_obj.h"

#ifdef PORT
/* This state follows the synchronized controller samples and is restored by rollback. */
typedef struct BtlEmoteState {
    unsigned previous;
    unsigned steps;
    int selected;
    unsigned waveHeld, waveAge;
    int waveArmed, waveSampled, waveTriggered;
} BtlEmoteState;

static BtlEmoteState sEmotes[BTL_OBJ_MAX];

void BtlEmote_Reset(void) {
    for (unsigned i = 0; i < sizeof(sEmotes) / sizeof(sEmotes[0]); ++i) {
        sEmotes[i] = (BtlEmoteState){0};
    }
}

int BtlEmote_Input(int object, unsigned buttons, int idle, int selected) {
    const unsigned chord = BTLB_LOCKON | BTLB_GUARD;
    const unsigned cancel = BTLB_DIR_MASK | BTLB_DASH | BTLB_ASCEND | BTLB_DESCEND |
                            BTLB_RUSH | BTLB_BLAST | BTLB_CHARGE;
    const int available = idle && BtlEmote_ValidSelection(selected) && !(buttons & cancel);
    const int claimed = available && (buttons & chord) == chord;
    BtlEmoteState *state;

    if (object < 0 || object >= BTL_OBJ_MAX) return 0;
    state = &sEmotes[object];
    if (!available) {
        state->selected = 0;
    } else if (claimed && (state->previous & chord) != chord) {
        state->selected = selected;
        state->steps = 0;
    } else if (state->selected) {
        ++state->steps;
    }
    state->previous = buttons;
    return claimed;
}

/* Physical synchronized pad bits, before target-button and key-layout filters.
 * Keep the original seven-tick escape window and action/health requirements.
 * A shoulder/face-button chord invalidates both taps, including the emote chord. */
void BtlEmergency_Input(int object, unsigned buttons, int live) {
    BtlEmoteState *s;
    const unsigned l1=0x400, chord=0xF000|0xB00;
    unsigned held=buttons&l1;
    if(object<0 || object>=BTL_OBJ_MAX)return;
    s=&sEmotes[object];s->waveTriggered=0;s->waveSampled=live;
    if(!live || (buttons&chord)) {
        s->waveArmed=0;s->waveAge=0;s->waveHeld=held;return;
    }
    if(s->waveArmed && ++s->waveAge>=BTL_TAP_FRAMES)s->waveArmed=0;
    if(held && !s->waveHeld) {
        if(s->waveArmed) {s->waveTriggered=1;s->waveArmed=0;}
        else {s->waveArmed=1;s->waveAge=0;}
    }
    s->waveHeld=held;
}
int BtlEmergency_Sampled(int object) {
    return object>=0 && object<BTL_OBJ_MAX && sEmotes[object].waveSampled;
}
int BtlEmergency_Triggered(int object) {
    return BtlEmergency_Sampled(object) && sEmotes[object].waveTriggered;
}

int BtlEmote_Pose(int object, float *frame) {
    extern int Port_SpeedMilli(void);
    BtlEmoteState *state;
    if (object < 0 || (unsigned)object >= sizeof(sEmotes) / sizeof(sEmotes[0])) return 0;
    state = &sEmotes[object];
    if (!state->selected) return 0;
    *frame = state->steps * (state->selected >= BTL_EMOTE_MIXAMO_FIRST && state->selected <= BTL_EMOTE_COUNT ?
                            1000.0f / (float)Port_SpeedMilli() : 2.0f);
    return state->selected;
}
#endif
