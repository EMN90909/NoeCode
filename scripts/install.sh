#!/usr/bin/env sh
set -eu
BUILD_DIR=${BUILD_DIR:-build}
PREFIX=${PREFIX:-}
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --config Release --parallel
if [ -n "$PREFIX" ]; then
  cmake --install "$BUILD_DIR" --config Release --prefix "$PREFIX"
else
  cmake --install "$BUILD_DIR" --config Release
fi
