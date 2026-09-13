# Getting started

Noqeri source files end in `.nqr`. A normal Noqeri user should not need CMake, C++ or knowledge of the bootstrap compiler.

## 1. Install or provide the trusted seed

Noqeri is moving through a staged self-host. A source build needs one trusted Noqeri stage-0 executable to produce the portable stage-1 compiler. The historical C++17 seed is preserved on `bootstrap/cpp17-stage0` for provenance; it is not the normal compiler path.

The reference stage-1 launcher currently uses Node.js 20+ to host the OS-neutral `.nqo` compiler kernel.

POSIX:

```sh
NOQERI_STAGE0=/path/to/noqeri ./scripts/build.sh
./build/noqeri selftest
```

Windows PowerShell:

```powershell
$env:NOQERI_STAGE0 = 'C:\path\to\noqeri.exe'
.\scripts\build.ps1
.\build\noqeri.cmd selftest
```

If you installed an official binary distribution, skip the source-bootstrap step and use the installed `noqeri` command directly.

## 2. Your first program

Create `hello.nqr`:

```nqr
function square(x: int): int {
    return x * x
}

function main(): int {
    print("Hello from Noqeri")
    print(square(7))
    return 0
}
```

Check it:

```sh
noqeri check hello.nqr
```

Stage-1 commands are intentionally added only after their Noqeri-owned implementations reach parity. If a command documented elsewhere is unavailable in your stage-1 build, that is a maturity boundary—not an invitation to silently fall back to the C++ bootstrap.

## 3. Create a project

A Noqeri project uses `project.nqr` and an exact `noqeri.lock` for dependencies. Keep the lockfile in version control. Package resolution verifies immutable cached package content and supports offline operation once dependencies are available locally.

## 4. Where to learn next

Follow `Doc/BEGINNER_COURSE.md`. It teaches ordinary application programming first: values, decisions, loops, functions, records, collections, files, JSON, HTTP, databases and concurrency. Pointers, atomics, FFI and assembly are deliberately later topics.

For the bootstrap trust boundary and parity status, see `Doc/SELF_HOSTING.md`. For stable-language intent, see `Doc/LANGUAGE_SPEC_2026.md` and `Doc/COMPATIBILITY.md`.
