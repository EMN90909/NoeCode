#!/usr/bin/env sh
set -eu
sh scripts/test.sh
./build/noe doctor .
printf 'Noe release check passed\n'
