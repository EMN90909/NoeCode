#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
STAGE0="${NOQERI_STAGE0:-}"
BOOTSTRAP="$ROOT/Compiler/selfhost/bootstrap.nqr"

if [ -z "$STAGE0" ]; then
  if command -v noqeri >/dev/null 2>&1; then STAGE0=$(command -v noqeri); fi
fi
if [ -z "$STAGE0" ]; then
  cat >&2 <<'MSG'
Noqeri's bootstrap program is written in Noqeri.
A trusted stage-0 executable is still required only to turn that source into the
first portable stage-1 object. Set NOQERI_STAGE0 to a trusted Noqeri seed.
CMake/C++ are not used by this normal build.
MSG
  exit 2
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
printf 'Noqeri stage-1 built from Noqeri bootstrap source at %s\n' "$BUILD_DIR/noqeri"
