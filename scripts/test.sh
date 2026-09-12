#!/usr/bin/env sh
set -eu

./scripts/test_core.sh

# Execute the general systems fixture and assert exact language semantics.
expected='4
3
42
42
43
9
-4'
actual="$(./build/noqeri run tests/general_capabilities.nqr)"
if [ "$actual" != "$expected" ]; then
  echo "general capability semantics mismatch" >&2
  echo "expected:" >&2
  printf '%s\n' "$expected" >&2
  echo "actual:" >&2
  printf '%s\n' "$actual" >&2
  exit 1
fi

# Invalid programs must be rejected by the static checker.
cat > build/invalid_type.nqr <<'EOF'
let count: u32 = "not a number"
EOF
if ./build/noqeri check build/invalid_type.nqr >/dev/null 2>&1; then
  echo "type checker accepted an invalid assignment" >&2
  exit 1
fi

# Native codegen is freestanding: verify emitted artifacts instead of assuming
# an OS executable format or process runtime.
./build/noqeri build examples/native_hello.nqr build/native_hello.s
test -s build/native_hello.s
grep -q '^\.global noqeri_entry$' build/native_hello.s
grep -q '^noqeri_entry:$' build/native_hello.s

./build/noqeri build tests/systems_types.nqr build/systems_types.s
test -s build/systems_types.s
grep -q '^\.global kernel_main$' build/systems_types.s
grep -q '^kernel_main:$' build/systems_types.s
grep -q 'mov DWORD PTR \[rax\], ebx' build/systems_types.s

./build/noqeri build examples/general_systems.nqr build/general_systems.s
test -s build/general_systems.s
grep -q 'lock xchg' build/general_systems.s
grep -q 'pause' build/general_systems.s
grep -q '^exercise:$' build/general_systems.s

if grep -Eq '\bsyscall\b|\b_start\b|elf_x86_64|\.note\.GNU-stack' build/native_hello.s build/systems_types.s build/general_systems.s; then
  echo "freestanding native output contains a platform-specific runtime assumption" >&2
  exit 1
fi

# On the current x86-64 GAS target, prove the emitted text is real assembler,
# not merely plausible-looking output. Linking stays outside the core compiler.
case "$(uname -s)-$(uname -m)" in
  Linux-x86_64)
    ${CXX:-c++} -c build/native_hello.s -o build/native_hello.o
    ${CXX:-c++} -c build/systems_types.s -o build/systems_types.o
    ${CXX:-c++} -c build/general_systems.s -o build/general_systems.o
    test -s build/native_hello.o
    test -s build/systems_types.o
    test -s build/general_systems.o
    ;;
esac

echo "semantic, type-safety, and freestanding native codegen tests passed"
