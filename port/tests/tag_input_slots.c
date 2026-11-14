/* Compile with -ffunction-sections -fdata-sections -Wl,--gc-sections and
 * -fsanitize=address. Exercise the production input history for all four local
 * seats without a window, controller or SDL library. */
#include "../src/gs/gs_input.c"

bool SDL_AddEventWatch(SDL_EventFilter filter, void *userdata) {
    (void)filter; (void)userdata;
    return true;
}
Uint64 SDL_GetTicksNS(void) { return 0; }
void Port_LagNote(int what, unsigned long long value) { (void)what; (void)value; }
const char *Port_GetEnv(const char *name) { (void)name; return "1"; }

int main(void) {
    int seat, frame;
    for (frame = 0; frame < 100; frame++) {
        for (seat = 0; seat < 4; seat++) {
            press_age(seat, frame & 1 ? B_CROSS : B_SQUARE);
        }
    }
    puts("PASS four-seat input history with address sanitizer");
    return 0;
}
