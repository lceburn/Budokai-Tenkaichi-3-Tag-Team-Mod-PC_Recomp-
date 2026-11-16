#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
cc -DPORT=1 -DPORT_FIGHTER_CAPACITY=15 -include port/tests/effect_diag_noasm.h -std=gnu99 -w -ffunction-sections -fdata-sections -Iinclude -Iport/include -Iport/src -c src/battle/eft_ae.c -o /tmp/effect-diag-tasks.o
cc -std=gnu99 -w -ffunction-sections -fdata-sections -Iinclude -Iport/include -Iport/src port/tests/effect_diag.c /tmp/effect-diag-tasks.o -Wl,--gc-sections -lm -o /tmp/effect-diag-test
mkdir -p port/build/effect-diag-tests
cd port/build/effect-diag-tests
/tmp/effect-diag-test
