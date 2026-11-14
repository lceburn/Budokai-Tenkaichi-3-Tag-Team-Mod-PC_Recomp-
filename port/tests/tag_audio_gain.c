/* Both audio backends must use the voice slider for every battle/practice
 * actor, and the effects slider for the following streamed-effect players.
 * Compile with -ffunction-sections -fdata-sections -Wl,--gc-sections. */
#include "../src/gs/snd_adx.c"
#include <assert.h>

static float sdlGain, maGain;
float PortA11y_StreamGain(const void *player) { (void)player; return 0.5f; }
bool SDL_SetAudioStreamGain(SDL_AudioStream *stream, float gain) {
    (void)stream; sdlGain = gain; return true;
}
void PortMa_SetVolume(void *handle, float gain) { (void)handle; maGain = gain; }

int main(void) {
    int ch;
    gPortMusicPercent = 80;
    gPortStagePercent = 60;
    gPortVoicePercent = 40;
    gPortSePercent = 20;
    for (ch = 0; ch < MAX_PLAYERS; ch++) {
        Player *p = &sPlayers[ch];
        float expected = ch == 0 ? 0.4f : ch == 1 ? 0.3f :
                         ch >= 4 && ch < 4 + TAG_ACTOR_CAPACITY ? 0.2f : 0.1f;
        p->stream = (SDL_AudioStream *)1;
        p->ma = (void *)1;
        p->vol = 0;
        apply_volume(p);
        assert(fabsf(sdlGain - expected) < 0.00001f);
        assert(fabsf(maGain - expected) < 0.00001f);
        p->vol = -960;
        apply_volume(p);
        assert(sdlGain == 0.0f && maGain == 0.0f);
    }
    puts("PASS music, stage, all actor voices, effects and mute on both audio backends");
    return 0;
}
