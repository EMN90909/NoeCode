# Noe Documentation

Welcome to the Noe documentation.

Noe is a standalone, statically typed, native-oriented general-purpose programming language. This repository contains the current bootstrap compiler and toolchain.

## Start here

1. [Getting started](GETTING_STARTED.md)
2. [Language reference](LANGUAGE_REFERENCE.md)
3. [Toolchain reference](TOOLCHAIN.md)
4. [Projects and packages](PROJECTS.md)
5. [Native backend](NATIVE_BACKEND.md)
6. [Implementation status](STATUS.md)
7. [Roadmap](ROADMAP.md)

## What Noe can do today

The current bootstrap can lex, parse, type-check, lower to NIR, optimize, interpret, and natively build a supported `.noe` subset into a Linux x86-64 executable.

## Important status note

NoeCode is usable as a bootstrap compiler and language experiment. It is not yet the full future Noe language described by the long-term specification. See [STATUS.md](STATUS.md) for the exact supported feature set.

## Noe project layout

```text
myapp/
├── project.noe
├── src/
│   └── main.noe
└── noe.lock
```

## Example

```noe
function add(a: int, b: int): int {
    return a + b
}

print("Hello from Noe")
print(add(10, 20))
```

Run it:

```sh
./build/noe run src/main.noe
```

Build it natively on Linux x86-64:

```sh
./build/noe build src/main.noe build/myapp
./build/myapp
```
