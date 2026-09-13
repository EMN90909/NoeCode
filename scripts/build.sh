#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
STAGE0="${NOQERI_STAGE0:-}"

if [ -z "$STAGE0" ]; then
  if command -v noqeri >/dev/null 2>&1; then STAGE0=$(command -v noqeri); fi
fi
if [ -z "$STAGE0" ]; then
  cat >&2 <<'MSG'
Noqeri needs a trusted stage-0 compiler only for the bootstrap step.
Set NOQERI_STAGE0 to an existing Noqeri compiler, or install a seed built from
branch bootstrap/cpp17-stage0. CMake/C++ are not used by this normal build.
MSG
  exit 2
fi
if ! command -v node >/dev/null 2>&1; then
  echo "Noqeri stage-1 portable host requires Node.js 20+" >&2
  exit 2
fi
mkdir -p "$BUILD_DIR"
"$STAGE0" check "$ROOT/Compiler/selfhost/main.nqr"
"$STAGE0" web "$ROOT/Compiler/selfhost/main.nqr" "$BUILD_DIR/noqeri-stage1.nqo"
cat > "$BUILD_DIR/noqeri" <<LAUNCH
#!/usr/bin/env sh
NOQERI_STAGE1_MODULE="$BUILD_DIR/noqeri-stage1.nqo" exec node "$ROOT/Runtime/stage1-cli.mjs" "\$@"
LAUNCH
chmod +x "$BUILD_DIR/noqeri"
"$BUILD_DIR/noqeri" selftest
"$BUILD_DIR/noqeri" --version
printf 'Noqeri stage-1 built at %s\n' "$BUILD_DIR/noqeri"
