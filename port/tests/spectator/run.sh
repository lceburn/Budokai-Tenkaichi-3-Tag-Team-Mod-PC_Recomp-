#!/bin/sh
set -eu
cd "$(dirname "$0")/../../.."
cc -std=c99 -Wall -Wextra -Werror port/tests/spectator/test_camera.c -lm -o /tmp/tag-spec-camera
/tmp/tag-spec-camera
cc -std=c99 -Wall -Wextra -Wno-unused-function port/tests/spectator/test_host.c $(pkg-config --cflags --libs sdl3) -lm -o /tmp/tag-spec-host
/tmp/tag-spec-host
cc -std=c99 -Wall -Wextra -Wno-unused-function port/tests/spectator/test_photo_mode.c $(pkg-config --cflags --libs sdl3) -lm -o /tmp/tag-spec-photo
/tmp/tag-spec-photo
cc -std=c99 -Wall -Wextra port/tests/spectator/test_controls.c -lm -o /tmp/tag-spec-controls
/tmp/tag-spec-controls
cc -std=gnu99 -w -ffunction-sections -fdata-sections -Iinclude -Iport/include port/tests/spectator/test_layout.c -Wl,--gc-sections -lm -o /tmp/tag-spec-layout
/tmp/tag-spec-layout
cc -std=gnu99 -w -ffunction-sections -fdata-sections -Iinclude -Iport/include port/tests/spectator/test_unlocked.c -Wl,--gc-sections -lm -o /tmp/tag-spec-unlocked
/tmp/tag-spec-unlocked
