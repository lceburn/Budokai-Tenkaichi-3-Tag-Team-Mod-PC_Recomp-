#!/bin/sh
set -eu
cd "$(dirname "$0")/../../.."
cc -std=c99 -Wall -Wextra -Werror port/tests/spectator/test_camera.c -lm -o /tmp/tag-spec-camera
/tmp/tag-spec-camera
cc -std=c99 -Wall -Wextra -Wno-unused-function port/tests/spectator/test_host.c $(pkg-config --cflags --libs sdl3) -lm -o /tmp/tag-spec-host
/tmp/tag-spec-host
