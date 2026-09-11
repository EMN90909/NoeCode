#!/usr/bin/env sh
set -eu
./scripts/build.sh
RIC=./build/ric
if [ ! -x "$RIC" ] && [ -x ./build/Release/ric ]; then RIC=./build/Release/ric; fi
"$RIC" --version
"$RIC" check examples/hello.ric
"$RIC" run examples/hello.ric
"$RIC" test tests
"$RIC" doctor .
