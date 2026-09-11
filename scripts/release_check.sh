#!/usr/bin/env sh
set -eu
./scripts/test.sh
./build/ric release-check .
