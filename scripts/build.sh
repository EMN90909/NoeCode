#!/usr/bin/env sh
set -eu

STARTED_AT=$(date +%s)
REPO_ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
BUILD_DIR="${BUILD_DIR:-build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
ASSUME_YES="${NOQERI_INSTALL_MISSING:-0}"

show_banner() {
  if [ -f "$REPO_ROOT/Brand/noqeri-banner.txt" ]; then
    cat "$REPO_ROOT/Brand/noqeri-banner.txt"
  fi
}

format_elapsed() {
  seconds="$1"
  hours=$((seconds / 3600))
  minutes=$(((seconds % 3600) / 60))
  secs=$((seconds % 60))
  printf '%02d:%02d:%02d' "$hours" "$minutes" "$secs"
}

show_next_steps() {
  compiler="$1"
  printf '\nHow to use:\n'
  printf '  %s --version\n' "$compiler"
  printf '  %s run examples/hello.nqr\n' "$compiler"
  printf '  %s check examples/hello.nqr\n' "$compiler"
  printf '  ctest --test-dir %s -C %s --output-on-failure\n' "$BUILD_DIR" "$BUILD_TYPE"
}

have() { command -v "$1" >/dev/null 2>&1; }

missing_items() {
  missing=""
  if ! have cmake; then missing="CMake"; fi
  if ! have c++ && ! have g++ && ! have clang++; then
    if [ -n "$missing" ]; then missing="$missing, C++ build toolchain"; else missing="C++ build toolchain"; fi
  fi
  printf '%s' "$missing"
}

run_as_root() {
  if [ "$(id -u)" -eq 0 ]; then
    "$@"
  elif have sudo; then
    sudo "$@"
  else
    echo "administrator privileges are required to install build prerequisites" >&2
    return 1
  fi
}

install_linux_prerequisites() {
  if have apt-get; then
    run_as_root apt-get update
    run_as_root apt-get install -y cmake build-essential
  elif have dnf; then
    run_as_root dnf install -y cmake gcc-c++ make
  elif have zypper; then
    run_as_root zypper --non-interactive install cmake gcc-c++ make
  elif have pacman; then
    run_as_root pacman -Sy --noconfirm cmake base-devel
  elif have apk; then
    run_as_root apk add cmake build-base
  else
    echo "no supported package manager was found; install CMake and a C++ compiler manually" >&2
    return 1
  fi
}

install_macos_prerequisites() {
  need_cmake=0
  need_cxx=0
  have cmake || need_cmake=1
  if ! have c++ && ! have clang++; then need_cxx=1; fi

  if [ "$need_cxx" -eq 1 ]; then
    echo "macOS C++ tools are missing; opening the Apple Command Line Tools installer..."
    xcode-select --install || true
    echo "finish the Apple Command Line Tools installation, then rerun ./scripts/build.sh" >&2
    return 1
  fi

  if [ "$need_cmake" -eq 1 ]; then
    if have brew; then
      brew install cmake
    else
      echo "CMake is missing and Homebrew is not available. Install CMake, then rerun ./scripts/build.sh." >&2
      return 1
    fi
  fi
}

show_banner
missing="$(missing_items)"
if [ -n "$missing" ]; then
  echo "Noqeri needs the following missing build prerequisite(s): $missing"

  answer=""
  if [ "$ASSUME_YES" = "1" ]; then
    answer="Y"
  elif [ -t 0 ]; then
    printf 'Do you want to install all missing prerequisites now? [Y/N] '
    IFS= read -r answer
  else
    echo "non-interactive shell detected; set NOQERI_INSTALL_MISSING=1 to allow installation" >&2
    exit 2
  fi

  case "$answer" in
    y|Y|yes|YES|Yes)
      case "$(uname -s)" in
        Darwin) install_macos_prerequisites ;;
        Linux) install_linux_prerequisites ;;
        *)
          echo "automatic prerequisite installation is not supported on this operating system" >&2
          exit 2
          ;;
      esac
      ;;
    *)
      echo "No installation was performed. Prerequisite detection completed; the build is skipped until the missing tools are installed."
      exit 2
      ;;
  esac
fi

remaining="$(missing_items)"
if [ -n "$remaining" ]; then
  echo "still missing after installation: $remaining" >&2
  exit 2
fi

cd "$REPO_ROOT"
printf 'building Noqeri bootstrap (%s)\n' "$BUILD_TYPE"
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" --parallel

compiler=""
if [ -x "$BUILD_DIR/noqeri" ]; then
  compiler="$BUILD_DIR/noqeri"
elif [ -x "$BUILD_DIR/$BUILD_TYPE/noqeri" ]; then
  compiler="$BUILD_DIR/$BUILD_TYPE/noqeri"
fi

if [ -z "$compiler" ]; then
  echo "build completed but noqeri was not found in $BUILD_DIR" >&2
  exit 1
fi

elapsed=$(format_elapsed $(($(date +%s) - STARTED_AT)))
printf '\n'
show_banner
printf 'Noqeri build worked.\n'
printf 'Time taken: %s\n' "$elapsed"
printf 'Compiler: %s\n\n' "$compiler"
printf 'Version:\n'
"$compiler" --version
show_next_steps "$compiler"
