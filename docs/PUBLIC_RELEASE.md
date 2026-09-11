# Public release checklist

Use this checklist before tagging a Noe release.

## Required

- `sh scripts/test.sh` passes.
- README explains what is implemented and what is not.
- `LICENSE` exists.
- `LEGAL.md` exists.
- `SECURITY.md` exists.
- `CONTRIBUTING.md` exists.
- Documentation index exists at `docs/README.md`.
- Example program runs through the interpreter.
- Example program builds as a native Linux x86-64 executable.
- CI passes on `main`.

## Nice to have

- GitHub repository description is set.
- Topics are set: `noe`, `compiler`, `programming-language`, `native`, `x86-64`, `interpreter`, `language-server`.
- First GitHub Release is created from a tested commit.
- Known limitations are listed in `docs/STATUS.md`.

## Current recommended description

Noe is an experimental statically typed general-purpose programming language and custom compiler bootstrap: `.noe` source, `project.noe` manifests, NIR, interpreter, native Linux x86-64 build, formatter, tests, and LSP diagnostics — no Rust, TOML, Cargo, or LLVM.
