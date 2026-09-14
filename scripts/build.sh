#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
STAGE0="${NOQERI_STAGE0:-}"
BOOTSTRAP="$ROOT/Compiler/selfhost/bootstrap.nqr"

if ! command -v node >/dev/null 2>&1; then
  echo "Noqeri's portable bootstrap tooling requires Node.js 20+" >&2
  exit 2
fi
mkdir -p "$BUILD_DIR"

if [ -z "$STAGE0" ]; then
  if command -v noqeri >/dev/null 2>&1; then STAGE0=$(command -v noqeri); fi
fi

# A preinstalled Noqeri binary is no longer required. When no trusted binary is
# supplied/found, build the repository's auditable C++ stage-0 source locally.
# Stage-2 reproducibility remains the trust-removal gate; this is a source
# bootstrap, not a claim that the C++ trust root has disappeared.
if [ -z "$STAGE0" ]; then
  if [ "${NOQERI_ALLOW_SOURCE_BOOTSTRAP:-1}" != "1" ]; then
    echo "Noqeri seed is missing and source bootstrap is disabled (NOQERI_ALLOW_SOURCE_BOOTSTRAP=0)" >&2
    exit 2
  fi
  if ! command -v cmake >/dev/null 2>&1; then
    echo "No Noqeri stage-0 was found. Install CMake/C++ for automatic source bootstrap or set NOQERI_STAGE0." >&2
    exit 2
  fi
  PATH_FILE="$BUILD_DIR/stage0-seed.path"
  node "$ROOT/scripts/source-bootstrap.mjs" --root="$ROOT" --build-dir="$BUILD_DIR/stage0-seed" --path-file="$PATH_FILE" --proof="$BUILD_DIR/stage0-seed.json"
  STAGE0=$(sed -n '1p' "$PATH_FILE")
fi

if [ ! -x "$STAGE0" ] && [ ! -f "$STAGE0" ]; then
  echo "Noqeri stage-0 is not executable or does not exist: $STAGE0" >&2
  exit 2
fi

"$STAGE0" check "$BOOTSTRAP"
"$STAGE0" web "$BOOTSTRAP" "$BUILD_DIR/noqeri-stage1.nqo"
cat > "$BUILD_DIR/noqeri" <<LAUNCH
#!/usr/bin/env sh
NOQERI_STAGE1_MODULE="$BUILD_DIR/noqeri-stage1.nqo" exec node "$ROOT/Runtime/stage1-cli.mjs" "\$@"
LAUNCH
chmod +x "$BUILD_DIR/noqeri"
"$BUILD_DIR/noqeri" selftest
"$BUILD_DIR/noqeri" --version
printf 'Noqeri stage-1 built from Noqeri bootstrap source at %s\n' "$BUILD_DIR/noqeri"
printf 'Stage-0 provenance: %s\n' "${NOQERI_STAGE0:+external seed}${NOQERI_STAGE0:-$BUILD_DIR/stage0-seed.json}"
