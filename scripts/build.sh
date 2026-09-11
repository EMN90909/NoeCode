#!/usr/bin/env sh
set -eu
CXX="${CXX:-c++}"
OUT="${NOE_OUT:-build/noe}"
mkdir -p "$(dirname "$OUT")"
"$CXX" -std=c++17 -O2 -Wall -Wextra -pedantic -Iinclude/noe compiler/bootstrap/*.cpp -o "$OUT"
printf 'Built %s\n' "$OUT"
