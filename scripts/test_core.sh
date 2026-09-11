#!/usr/bin/env sh
set -eu
NOE_BIN="${NOE_BIN:-./build/noe}"
mkdir -p build
"$NOE_BIN" --version

# Frontend + type checker + NIR + optimizer + interpreter.
"$NOE_BIN" check examples/native_hello.noe
"$NOE_BIN" nir examples/native_hello.noe > build/native_hello.nir
"$NOE_BIN" run examples/native_hello.noe > build/interpreter.out
diff -u examples/native_hello.expected build/interpreter.out

# Negative semantic diagnostic.
printf 'let value: int = "wrong"\n' > build/type_error.noe
if "$NOE_BIN" check build/type_error.noe > build/type_error.out 2>&1; then
    echo 'expected type checker to reject invalid program' >&2
    exit 1
fi
grep -q 'NOE-T3001' build/type_error.out

# Const safety diagnostic.
printf 'const value = 1\nvalue = 2\n' > build/const_error.noe
if "$NOE_BIN" check build/const_error.noe > build/const_error.out 2>&1; then
    echo 'expected type checker to reject const reassignment' >&2
    exit 1
fi
grep -q 'NOE-T3014' build/const_error.out

# Test runner.
"$NOE_BIN" test tests

# Formatter.
printf 'let   x=1+2;\nprint(x);\n' > build/format.noe
"$NOE_BIN" format build/format.noe
grep -q 'let x = 1 + 2;' build/format.noe

# Custom project.noe workflow + deterministic lock file.
rm -rf build/smoke_project
"$NOE_BIN" new smoke build/smoke_project
"$NOE_BIN" manifest build/smoke_project/project.noe > build/manifest.out
"$NOE_BIN" lock build/smoke_project/project.noe
test -f build/smoke_project/noe.lock
grep -q '^noe-lock 1$' build/smoke_project/noe.lock

# JSON-RPC/LSP initialize and live compiler diagnostics.
init='{"jsonrpc":"2.0","id":1,"method":"initialize","params":{}}'
open='{"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":"file:///bad.noe","languageId":"noe","version":1,"text":"let value: int = \"wrong\"\n"}}}'
frame() { m="$1"; n=$(printf '%s' "$m" | wc -c | tr -d ' '); printf 'Content-Length: %s\r\n\r\n%s' "$n" "$m"; }
{ frame "$init"; frame "$open"; } | "$NOE_BIN" lsp > build/lsp.out
grep -q 'Noe Language Server' build/lsp.out
grep -q 'NOE-T3001' build/lsp.out

"$NOE_BIN" doctor .
printf 'Noe cross-platform core tests passed\n'
