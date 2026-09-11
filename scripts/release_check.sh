#!/usr/bin/env sh
set -eu
./scripts/test.sh
./build/noqeri release-check .
