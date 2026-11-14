#!/bin/sh
# Build the Windows engine, shaders and launcher without disc assets.
# This does not produce a usable BT4 installer: startup recipes are a separate gate.
set -eu
cd "$(dirname "$0")/../.."
mkdir -p port/build
if [ "${BT3_NATIVE_BUILD_CONTAINER:-0}" != 1 ]; then
    docker build -t bt3-port-release:jammy port/release
    docker run --rm -u "$(id -u):$(id -g)" -e HOME=/tmp \
        -e BT3_NATIVE_BUILD_CONTAINER=1 -e BT3_VERSION -e BT3_BUILD_KEY \
        -v "$PWD":/src -w /src bt3-port-release:jammy \
        sh port/release/build_native_windows.sh
    exit 0
fi
export BT3_CC=win64 BT3_SKELETON=1
echo 'Compiling Windows game and GPU shaders'
python3 port/tools/undefined.py > port/build/integration-compile-win64.log 2>&1 || {
    tail -60 port/build/integration-compile-win64.log
    exit 1
}
if grep -q '^FAILED' port/build/integration-compile-win64.log; then
    grep '^FAILED' port/build/integration-compile-win64.log
    exit 1
fi
echo 'Linking Windows game'
python3 port/tools/link.py > port/build/integration-link-win64.log 2>&1 || {
    cat port/build/integration-link-win64.log
    exit 1
}
cat port/build/integration-link-win64.log
python3 port/tools/build_native_launcher.py
cp "$BT3_SDL3/bin/SDL3.dll" port/build/SDL3.dll
echo 'Built engine and launcher. BT4 installer compatibility remains a separate check.'
