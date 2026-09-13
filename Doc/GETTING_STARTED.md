# Getting started

Noqeri source files end in `.nqr`.

## Normal path

Use an official Noqeri release or a previously built Noqeri compiler when one is available for your platform. The normal language workflow is intentionally small:

```sh
noqeri --version
noqeri check hello.nqr
noqeri run hello.nqr
```

Save this as `hello.nqr`:

```nqr
function square(x: int): int {
    return x * x
}

print("Hello from Noqeri")
print(square(7))
```

For a project, use `noqeri new`, `noqeri check`, `noqeri test`, `noqeri run`, and `noqeri build`. Beginners do not need pointers, atomics, ABI layouts, native object formats, assembly, or FFI for ordinary application work.

## Bootstrap from source

`main` is moving toward the staged self-hosting model documented in `Doc/SELF_HOSTING.md`. `Compiler/selfhost/` is the Noqeri-owned compiler source. The historical C++17 seed is bootstrap provenance, not the desired long-term compiler implementation.

Until the stage-1 -> stage-2 reproducibility gate in `SELF_HOSTING.md` has been demonstrated end-to-end on release machines, a source bootstrap may still require the archival host/bootstrap toolchain. Do not interpret that temporary bootstrap dependency as the normal language programming model.

Current local bootstrap scripts remain:

```sh
./scripts/build.sh
./build/noqeri --version
```

On Windows use `scripts/build.ps1`; multi-config generators may place the executable at `build/Release/noqeri.exe`.

## Safety/diagnostic modes

The quality driver exposes opt-in heavy diagnostics while keeping normal Noqeri simple:

```sh
node Runtime/noqeri-quality.mjs run --check-memory hello.nqr
node Runtime/noqeri-quality.mjs test --race tests
node Runtime/noqeri-quality.mjs test --overflow tests
```

`--check-memory` uses an AddressSanitizer/UBSan instrumented host build; `--race` uses a ThreadSanitizer host build; `--overflow` enables Noqeri's checked integer execution path. Sanitizer presets currently require a compiler/toolchain that supports the corresponding sanitizer.

Noqeri should not claim performance leadership from anecdotes. See `Benchmarks/METHODOLOGY.md` for the evidence rules.
