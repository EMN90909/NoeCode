#!/usr/bin/env sh
set -eu
CXX="${CXX:-c++}"
mkdir -p build
"$CXX" -std=c++17 -O2 -Wall -Wextra -pedantic -Ibootstrap bootstrap/*.cpp -o build/noe
printf 'Built build/noe\n'
