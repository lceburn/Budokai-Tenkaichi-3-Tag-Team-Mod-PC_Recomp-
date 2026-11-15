#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
sh port/tests/spectator/run.sh
cc -no-pie -std=c99 -Wall -Wextra -fsanitize=address,undefined port/tests/mods/test_camera_bridge.c -o /tmp/tag-camera-bridge
/tmp/tag-camera-bridge
cc -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all port/tests/mods/test_mods.c port/src/gs/mods_lua.c -lm -lpthread -o /tmp/tag-mods-test
/tmp/tag-mods-test
