#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
STAGE0="${NOQERI_STAGE0:-}"
COMPILER="$ROOT/Compiler/selfhost/main.nqr"

if [ -z "$STAGE0" ]; then
  if command -v noqeri >/dev/null 2>&1; then STAGE0=$(command -v noqeri); fi
fi

# Normal development remains Noqeri -> Stage-2. A clean checkout no longer
# requires downloading a prebuilt Noqeri binary: when no seed is supplied, the
# archived/auditable C++ provenance compiler is built locally and used exactly
# once to compile the checked-in self-hosted Noqeri compiler source.
if [ -z "$STAGE0" ]; then
  if ! command -v cmake >/dev/null 2>&1; then
    echo "No NOQERI_STAGE0 was supplied and CMake is unavailable for clean source bootstrap" >&2
    exit 2
  fi
  STAGE0_DIR="$BUILD_DIR/stage0-provenance"
  cmake -S "$ROOT" -B "$STAGE0_DIR" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
  cmake --build "$STAGE0_DIR" --config Release --target noqeri --parallel
  if [ -x "$STAGE0_DIR/noqeri" ]; then STAGE0="$STAGE0_DIR/noqeri"
  elif [ -x "$STAGE0_DIR/Release/noqeri" ]; then STAGE0="$STAGE0_DIR/Release/noqeri"
  else echo "source bootstrap completed but the provenance compiler executable was not found" >&2; exit 2
  fi
fi
if ! command -v node >/dev/null 2>&1; then
  echo "Noqeri's portable Stage-2 host currently requires Node.js 20+" >&2
  exit 2
fi
mkdir -p "$BUILD_DIR"
"$STAGE0" check "$COMPILER"
"$STAGE0" web "$COMPILER" "$BUILD_DIR/noqeri-stage2.nqo"
cat > "$BUILD_DIR/noqeri" <<LAUNCH
#!/usr/bin/env sh
NOQERI_STAGE2_MODULE="$BUILD_DIR/noqeri-stage2.nqo" exec node "$ROOT/Runtime/stage2-cli.mjs" "\$@"
LAUNCH
chmod +x "$BUILD_DIR/noqeri"
"$BUILD_DIR/noqeri" selftest
"$BUILD_DIR/noqeri" stage2-status
"$BUILD_DIR/noqeri" --version
printf 'Noqeri Stage-2 pipeline artifact built from Noqeri compiler source at %s\n' "$BUILD_DIR/noqeri"
printf 'Clean bootstrap seed: %s\n' "$STAGE0"
printf 'Bootstrap equivalence remains governed separately by scripts/bootstrap-stage2.mjs.\n'
