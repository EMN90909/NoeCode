# Getting started

Ric source files end in `.ric`.

## Build

Requirements: CMake 3.16+ and a C++17 compiler.

```sh
./scripts/build.sh
./build/ric --version
```

On Windows use `scripts/build.ps1`; multi-config generators usually place the binary at `build/Release/ric.exe`.

## First program

```ric
function square(x: int): int {
    return x * x
}

print("Hello from Ric")
print(square(7))
```

Save it as `hello.ric`, then run:

```sh
ric check hello.ric
ric run hello.ric
```

On Linux x86-64 the current bootstrap can also emit a native executable:

```sh
ric build hello.ric build/hello
./build/hello
```
