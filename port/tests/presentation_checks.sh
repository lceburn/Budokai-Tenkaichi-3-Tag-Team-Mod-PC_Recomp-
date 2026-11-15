#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
sh port/tests/spectator/run.sh
cc -std=gnu99 -w -ffunction-sections -fdata-sections -Iinclude -Iport/include port/tests/test_battle_damage.c -Wl,--gc-sections -o /tmp/tag-battle-damage
/tmp/tag-battle-damage
cc -std=gnu99 -w -ffunction-sections -fdata-sections -Iinclude -Iport/include port/tests/test_blast_deflect.c -Wl,--gc-sections -lm -o /tmp/tag-blast-deflect
/tmp/tag-blast-deflect
cc -no-pie -std=c99 -Wall -Wextra -fsanitize=address,undefined port/tests/mods/test_camera_bridge.c -o /tmp/tag-camera-bridge
/tmp/tag-camera-bridge
cc -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all port/tests/mods/test_mods.c port/src/gs/mods_lua.c -lm -lpthread -o /tmp/tag-mods-test
/tmp/tag-mods-test
c++ -std=c++17 -Iport/third_party/imgui $(pkg-config --cflags sdl3) port/tests/tag_hud_render.cpp \
    port/third_party/imgui/imgui.cpp port/third_party/imgui/imgui_draw.cpp \
    port/third_party/imgui/imgui_tables.cpp port/third_party/imgui/imgui_widgets.cpp \
    $(pkg-config --libs sdl3) -pthread -o /tmp/tag-hud-render
/tmp/tag-hud-render
c++ -std=c++17 -ffunction-sections -fdata-sections -Iport/third_party/imgui $(pkg-config --cflags sdl3) \
    port/tests/mods/test_hud_visibility.cpp port/third_party/imgui/imgui.cpp \
    port/third_party/imgui/imgui_draw.cpp port/third_party/imgui/imgui_tables.cpp port/third_party/imgui/imgui_widgets.cpp \
    -Wl,--gc-sections $(pkg-config --libs sdl3) -pthread -o /tmp/tag-script-hud
/tmp/tag-script-hud
