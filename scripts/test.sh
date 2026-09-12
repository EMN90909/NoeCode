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

reject_file() {
  file="$1"
  label="$2"
  if ./build/noqeri check "$file" >/dev/null 2>&1; then
    echo "type checker accepted invalid program: $label" >&2
    exit 1
  fi
}

# The checker must reject category mismatches, mutation violations, unsafe
# pointer conversions, invalid atomic use, generic disagreement and integer
# narrowing/signedness hazards.
cat > build/invalid_type.nqr <<'EOF'
let count: u32 = "not a number"
EOF
reject_file build/invalid_type.nqr "string assigned to u32"

cat > build/invalid_const.nqr <<'EOF'
const answer: int = 42
answer = 43
EOF
reject_file build/invalid_const.nqr "const reassignment"

cat > build/invalid_condition.nqr <<'EOF'
if 1 {
    print(1)
}
EOF
reject_file build/invalid_condition.nqr "integer used as bool condition"

cat > build/invalid_pointer.nqr <<'EOF'
let word: u32 = 1
let wordPtr: *u32 = &word
let bytePtr: *u8 = wordPtr
EOF
reject_file build/invalid_pointer.nqr "incompatible pointer assignment"

cat > build/invalid_atomic.nqr <<'EOF'
let value: int = atomic.load(7)
EOF
reject_file build/invalid_atomic.nqr "atomic operation without pointer"

cat > build/invalid_generic.nqr <<'EOF'
function same<T>(left: T, right: T): T {
    return left
}
print(same(1, "one"))
EOF
reject_file build/invalid_generic.nqr "generic type disagreement"

cat > build/invalid_narrowing.nqr <<'EOF'
let wide: u64 = 300
let byte: u8 = wide
EOF
reject_file build/invalid_narrowing.nqr "implicit u64 to u8 narrowing"

cat > build/invalid_literal_range.nqr <<'EOF'
let byte: u8 = 300
EOF
reject_file build/invalid_literal_range.nqr "out-of-range integer literal"

cat > build/invalid_signedness.nqr <<'EOF'
let signed: i32 = -1
let unsigned: u32 = signed
EOF
reject_file build/invalid_signedness.nqr "implicit signed to unsigned conversion"

# Proven-safe literal initialization and widening remain concise.
cat > build/valid_integer_safety.nqr <<'EOF'
let port: u16 = 8080
let small: u8 = 42
let wider: u32 = small
let signedWide: i64 = wider
print(port)
print(signedWide)
EOF
./build/noqeri check build/valid_integer_safety.nqr >/dev/null
valid_output="$(./build/noqeri run build/valid_integer_safety.nqr)"
if [ "$valid_output" != '8080
42' ]; then
  echo "safe integer conversion regression" >&2
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
# x86 memory xchg is implicitly atomic; no lock prefix is required.
grep -q 'xchg QWORD PTR \[rax\], rbx' build/general_systems.s
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
