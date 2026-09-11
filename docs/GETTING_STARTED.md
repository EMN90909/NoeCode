# Getting started with Noe

This guide gets you from the repository to a running `.noe` program.

## 1. Build the bootstrap compiler

Linux/macOS:

```sh
sh scripts/build.sh
```

Windows PowerShell:

```powershell
./scripts/build.ps1
```

The bootstrap compiler currently requires a C++17 compiler. No Rust, Cargo, TOML, or LLVM is used by the Noe toolchain.

## 2. Run a Noe program through the interpreter

```sh
./build/noe run examples/native_hello.noe
```

Expected output:

```text
Hello from native Noe
30
0
1
2
```

## 3. Build a native executable

```sh
./build/noe build examples/native_hello.noe build/native_hello
./build/native_hello
```

The generated program is a Linux x86-64 executable for the supported native subset.

## 4. Create a new project

```sh
./build/noe new hello-noe
cd hello-noe
../build/noe lock
../build/noe run src/main.noe
```

A Noe project uses:

```text
project.noe
src/main.noe
noe.lock
```

No TOML manifest is required.

## 5. Run all checks

From the repository root:

```sh
sh scripts/test.sh
```

This command builds the compiler, checks sample programs, runs interpreter tests, builds and executes a native binary, verifies `project.noe`, validates deterministic locks, and exercises the language-server diagnostic path.
