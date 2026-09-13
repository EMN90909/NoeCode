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
"$NQ" check tests/time_modules.nqr >/dev/null
"$NQ" run tests/time_modules.nqr >/dev/null
"$NQ" check tests/data_integrity.nqr >/dev/null
"$NQ" run tests/data_integrity.nqr >/dev/null
"$NQ" check tests/encoding_foundations.nqr >/dev/null
"$NQ" run tests/encoding_foundations.nqr >/dev/null

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

echo "maturity checks passed"
