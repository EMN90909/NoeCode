# Getting started

noqeri source files end in `.nqr`.

Requirements: CMake 3.16+ and a C++17 compiler.

```sh
./scripts/build.sh
./build/noqeri --version
```

Windows uses `scripts/build.ps1`; multi-config generators generally place the executable at `build/Release/noqeri.exe`.

```nqr
function square(x: int): int {
    return x * x
}

print("Hello from noqeri")
print(square(7))
```

Save as `hello.nqr`, then run `noqeri check hello.nqr` and `noqeri run hello.nqr`. Linux x86-64 can additionally use `noqeri build hello.nqr build/hello`.
