#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
STAGE0="${NOQERI_STAGE0:-}"
BOOTSTRAP="$ROOT/Compiler/selfhost/bootstrap.nqr"

if [ -z "$STAGE0" ]; then
  if command -v noqeri >/dev/null 2>&1; then STAGE0=$(command -v noqeri); fi
fi

# A prebuilt Noqeri seed is no longer mandatory. When none is supplied, build
# the checked-in C++ bootstrap compiler from source and use it exactly once to
# produce the portable Noqeri stage-1 module. This keeps bootstrap reproducible
# from a clean checkout while preserving an explicit provenance boundary.
if [ -z "$STAGE0" ]; then
  if ! command -v cmake >/dev/null 2>&1; then
    echo "No NOQERI_STAGE0 was supplied and CMake is unavailable for source bootstrap" >&2
    exit 2
  fi
  STAGE0_DIR="$BUILD_DIR/stage0"
  cmake -S "$ROOT" -B "$STAGE0_DIR" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
  cmake --build "$STAGE0_DIR" --config Release --target noqeri --parallel
  if [ -x "$STAGE0_DIR/noqeri" ]; then
    STAGE0="$STAGE0_DIR/noqeri"
  elif [ -x "$STAGE0_DIR/Release/noqeri" ]; then
    STAGE0="$STAGE0_DIR/Release/noqeri"
  else
    echo "source bootstrap completed but the stage-0 compiler executable was not found" >&2
    exit 2
  fi
fi

if ! command -v node >/dev/null 2>&1; then
  echo "Noqeri's portable .nqo host requires Node.js 20+ at this stage" >&2
  exit 2
fi
mkdir -p "$BUILD_DIR"
"$STAGE0" check "$BOOTSTRAP"
"$STAGE0" web "$BOOTSTRAP" "$BUILD_DIR/noqeri-stage1.nqo"
cat > "$BUILD_DIR/noqeri" <<LAUNCH
#!/usr/bin/env sh
NOQERI_STAGE1_MODULE="$BUILD_DIR/noqeri-stage1.nqo" exec node "$ROOT/Runtime/stage1-cli.mjs" "\$@"
LAUNCH
chmod +x "$BUILD_DIR/noqeri"
"$BUILD_DIR/noqeri" selftest
"$BUILD_DIR/noqeri" --version
printf 'Noqeri stage-1 built from source bootstrap at %s\n' "$BUILD_DIR/noqeri"
