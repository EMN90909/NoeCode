#!/usr/bin/env sh
set -eu
./scripts/build.sh
NQ=./build/noqeri
if [ ! -x "$NQ" ] && [ -x ./build/Release/noqeri ]; then NQ=./build/Release/noqeri; fi
"$NQ" --version
"$NQ" check examples/hello.nqr
"$NQ" run examples/hello.nqr
"$NQ" test tests
NQ="$NQ" sh ./scripts/test_maturity.sh
"$NQ" doctor .
