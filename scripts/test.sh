#!/usr/bin/env sh
set -eu

./scripts/test_core.sh

# Native codegen is freestanding: verify emitted artifacts instead of assuming
# a Linux executable format or process runtime.
./build/noqeri build examples/native_hello.nqr build/native_hello.s
test -s build/native_hello.s
grep -q '^\.global noqeri_entry$' build/native_hello.s
grep -q '^noqeri_entry:$' build/native_hello.s

./build/noqeri build tests/systems_types.nqr build/systems_types.s
test -s build/systems_types.s
grep -q '^\.global kernel_main$' build/systems_types.s
grep -q '^kernel_main:$' build/systems_types.s
grep -q 'mov DWORD PTR \[rax\], ebx' build/systems_types.s

if grep -Eq '\bsyscall\b|\b_start\b|elf_x86_64|\.note\.GNU-stack' build/native_hello.s build/systems_types.s; then
  echo "freestanding native output contains a platform-specific runtime assumption" >&2
  exit 1
fi

echo "freestanding native codegen smoke tests passed"
