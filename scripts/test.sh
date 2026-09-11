#!/usr/bin/env sh
set -eu
./scripts/test_core.sh
case "$(uname -s)-$(uname -m)" in
  Linux-x86_64)
    ./build/noqeri build examples/native_hello.nqr build/native_hello
    ./build/native_hello
    ;;
  *)
    echo "native executable smoke test skipped: current native backend is Linux x86-64"
    ;;
esac
