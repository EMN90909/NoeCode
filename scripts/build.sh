#!/usr/bin/env sh
set -eu

cat <<'FOX'
        /\       /\
       /  \_____/  \
      /   / </> \   \
     /   /   ^   \   \
    (    \  ___  /    )
     \    '.___.'    /
      '._         _.'
         '-.___.-'
           NOQERI
FOX

BUILD_DIR="${BUILD_DIR:-build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
printf 'building Noqeri bootstrap (%s)\n' "$BUILD_TYPE"
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" --parallel
printf 'noqeri bootstrap built in %s\n' "$BUILD_DIR"
