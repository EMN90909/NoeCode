#!/usr/bin/env sh
set -eu
sh scripts/build.sh
sh scripts/test_core.sh

# Custom x86-64 backend + assembler/linker driver + produced ELF executable.
./build/noe build examples/native_hello.noe build/native_hello
./build/native_hello > build/native.out
diff -u examples/native_hello.expected build/native.out

printf 'Noe Linux native end-to-end tests passed\n'
