#!/usr/bin/env sh
set -eu
sh scripts/build.sh
./build/noe --version

# Frontend + type checker + NIR + optimizer + interpreter.
./build/noe check examples/native_hello.noe
./build/noe nir examples/native_hello.noe > build/native_hello.nir
./build/noe run examples/native_hello.noe > build/interpreter.out
diff -u examples/native_hello.expected build/interpreter.out

# Negative semantic diagnostic.
printf 'let value: int = "wrong"\n' > build/type_error.noe
if ./build/noe check build/type_error.noe > build/type_error.out 2>&1; then
    echo 'expected type checker to reject invalid program' >&2
    exit 1
fi
grep -q 'NOE-T3001' build/type_error.out

# Test runner.
./build/noe test tests

# Formatter.
printf 'let   x=1+2;\nprint(x);\n' > build/format.noe
./build/noe format build/format.noe
grep -q 'let x = 1 + 2;' build/format.noe

# Custom x86-64 backend + assembler/linker driver + produced ELF executable.
./build/noe build examples/native_hello.noe build/native_hello
./build/native_hello > build/native.out
diff -u examples/native_hello.expected build/native.out

# Custom project.noe workflow + deterministic lock file.
rm -rf build/smoke_project
./build/noe new smoke build/smoke_project
./build/noe manifest build/smoke_project/project.noe > build/manifest.out
./build/noe lock build/smoke_project/project.noe
test -f build/smoke_project/noe.lock
grep -q '^noe-lock 1$' build/smoke_project/noe.lock

# JSON-RPC/LSP initialize and live compiler diagnostics.
init='{"jsonrpc":"2.0","id":1,"method":"initialize","params":{}}'
open='{"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":"file:///bad.noe","languageId":"noe","version":1,"text":"let value: int = \"wrong\"\n"}}}'
frame() { m="$1"; n=$(printf '%s' "$m" | wc -c | tr -d ' '); printf 'Content-Length: %s\r\n\r\n%s' "$n" "$m"; }
{ frame "$init"; frame "$open"; } | ./build/noe lsp > build/lsp.out
grep -q 'Noe Language Server' build/lsp.out
grep -q 'NOE-T3001' build/lsp.out

printf 'Noe end-to-end tests passed\n'
