#!/usr/bin/env sh
set -eu

NQ=${NQ:-./build/noqeri}
if [ ! -x "$NQ" ] && [ -x ./build/Release/noqeri ]; then NQ=./build/Release/noqeri; fi

reject() {
  file="$1"
  label="$2"
  if "$NQ" check "$file" >/dev/null 2>&1; then
    echo "maturity checker accepted invalid program: $label" >&2
    exit 1
  fi
}

"$NQ" check tests/generic_records.nqr >/dev/null
"$NQ" check tests/generic_collections.nqr >/dev/null
"$NQ" check tests/stdlib_catalog.nqr >/dev/null
"$NQ" check tests/stdlib_maturity.nqr >/dev/null
"$NQ" run tests/stdlib_maturity.nqr >/dev/null
"$NQ" check tests/stdlib_structures.nqr >/dev/null
"$NQ" run tests/stdlib_structures.nqr >/dev/null
"$NQ" check tests/storage_structures.nqr >/dev/null
"$NQ" run tests/storage_structures.nqr >/dev/null
"$NQ" check tests/stdlib_foundations2.nqr >/dev/null
"$NQ" run tests/stdlib_foundations2.nqr >/dev/null
"$NQ" check tests/time_modules.nqr >/dev/null
"$NQ" run tests/time_modules.nqr >/dev/null
"$NQ" check tests/data_integrity.nqr >/dev/null
"$NQ" run tests/data_integrity.nqr >/dev/null
"$NQ" check tests/encoding_foundations.nqr >/dev/null
"$NQ" run tests/encoding_foundations.nqr >/dev/null
"$NQ" check tests/repeat_loop.nqr >/dev/null
"$NQ" run tests/repeat_loop.nqr >/dev/null

cat > build/invalid_pointer_escape.nqr <<'EOF'
function bad_pointer(): *u32 {
    let local: u32 = 7 as u32
    return &local
}
EOF
reject build/invalid_pointer_escape.nqr "pointer borrowed from local storage escapes function"

cat > build/invalid_dangling_inner_reference.nqr <<'EOF'
function bad_inner_reference(): u32 {
    let outer: u32 = 0 as u32
    let reference: *u32 = &outer
    {
        let inner: u32 = 9 as u32
        reference = &inner
    }
    return *reference
}
EOF
reject build/invalid_dangling_inner_reference.nqr "reference outlives inner borrowed storage"

cat > build/invalid_static_array_index.nqr <<'EOF'
function invalid_index(): u32 {
    let values: [u32; 2] = [1 as u32, 2 as u32]
    return values[2]
}
EOF
reject build/invalid_static_array_index.nqr "known fixed-array index is out of bounds"

cat > build/invalid_direct_null_dereference.nqr <<'EOF'
function invalid_null(): u32 {
    return *(null as *u32)
}
EOF
reject build/invalid_direct_null_dereference.nqr "provable null dereference"

cat > build/invalid_use_after_move.nqr <<'EOF'
function consume(values: [int; 2]): int { return values[0] }
function invalid_move(): int {
    let values: [int; 2] = [4, 8]
    consume(values)
    return values[0]
}
EOF
reject build/invalid_use_after_move.nqr "owned fixed array used after move into function"

cat > build/invalid_move_while_borrowed.nqr <<'EOF'
function consume(values: [int; 2]): int { return values[0] }
function invalid_borrow_move(): int {
    let values: [int; 2] = [4, 8]
    let borrowed: *[int; 2] = &values
    consume(values)
    return 0
}
EOF
reject build/invalid_move_while_borrowed.nqr "owned value moved while shared borrow remains live"

cat > build/valid_copy_scalar.nqr <<'EOF'
function consume(value: int): int { return value }
function valid_copy(): int {
    let value: int = 9
    consume(value)
    return value
}
EOF
"$NQ" check build/valid_copy_scalar.nqr >/dev/null

cat > build/valid_comptime.nqr <<'EOF'
function fib(n: int): int {
    let a: int = 0
    let b: int = 1
    let i: int = 0
    while i < n {
        let next: int = a + b
        a = b
        b = next
        i = i + 1
    }
    return a
}
const folded: int = comptime(fib(10))
comptime_assert(folded == 55)
function main(): int { print(folded) return 0 }
EOF
"$NQ" check build/valid_comptime.nqr >/dev/null
comptime_output="$("$NQ" run build/valid_comptime.nqr)"
if [ "$comptime_output" != '55' ]; then
  echo "compile-time execution changed expected value: $comptime_output" >&2
  exit 1
fi
"$NQ" nir build/valid_comptime.nqr > build/valid_comptime.nir
if grep -q 'comptime' build/valid_comptime.nir; then
  echo "comptime call survived lowering instead of being erased" >&2
  exit 1
fi

cat > build/invalid_comptime_host.nqr <<'EOF'
const impossible = comptime(host("clockMillis"))
EOF
reject build/invalid_comptime_host.nqr "compile-time execution attempted host I/O"

cat > build/valid_generic_record_nested.nqr <<'EOF'
record Cell<T> {
    value: T
}
record Envelope<T> {
    cell: Cell<T>
    revision: u64
}
function read_cell<T>(envelope: *Envelope<T>): T {
    return envelope.cell.value
}
export function read_int(envelope: *Envelope<int>): int {
    return read_cell(envelope)
}
EOF
"$NQ" check build/valid_generic_record_nested.nqr >/dev/null

cat > build/valid_generic_record_recursive.nqr <<'EOF'
record Link<T> {
    value: T
    next: *Link<T>
}
function next_link<T>(link: *Link<T>): *Link<T> {
    return link.next
}
export function next_u64(link: *Link<u64>): *Link<u64> {
    return next_link(link)
}
EOF
"$NQ" check build/valid_generic_record_recursive.nqr >/dev/null

echo "maturity checks passed (ownership, borrow safety, comptime, generics, bounds and null safety)"
