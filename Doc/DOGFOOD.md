# Dogfooding Noqeri

Dogfooding is a release criterion, not a branding statement. A tool counts as Noqeri-owned only when its behavior is implemented in `.nqr` and the normal production path executes that implementation without silently falling back to bootstrap C++/JavaScript.

## Current ownership map

| Capability | Noqeri-owned source | Host/bootstrap role | Gate to claim fully dogfooded |
| --- | --- | --- | --- |
| compiler stage 1 | `Compiler/selfhost/*.nqr` | `.nqo` host launcher and remaining parity helpers | parser/type/lowering/backend/package parity and conformance |
| parser policy | `Parser/*.nqr`, self-host compiler modules | trusted bootstrap parser remains for parity/reference | differential corpus parity |
| standard library | `Lib/**/*.nqr` | host services provide OS capabilities | supported providers execute advertised extern contracts |
| NoqeriDB architecture | `Database/*.nqr` | bootstrap `.nqd` engine still executes reference path | storage/WAL/planner/SQL parity |
| test helpers | `Lib/std/test.nqr`, official test package | bootstrap directory runner remains | self-host test discovery/execution parity |
| registry server | `noqeri-registry/src/registry.nqr` | hosting adapter supplies HTTP/filesystem | production server deployed through Noqeri path |
| package ecosystem libraries | `noqeri-registry/packages/**/*.nqr` | transport/native providers where required | per-package quality gates |
| platform policy | `Platforms/**/*.nqr` | boot/OS adapter supplies capability table | platform conformance |

## Required migrations

The following are deliberate bootstrap/host boundaries and should shrink rather than become permanent shadow implementations:

- formatter: canonical behavior must move from the stage-1 JavaScript helper into Noqeri while retaining byte-for-byte formatter tests;
- package manager: resolution, lock, cache verification, vendoring and advisories should be compiled from Noqeri-owned source;
- documentation generator: API extraction/rendering should move into a Noqeri tool;
- LSP: protocol transport may remain a host adapter, but semantic analysis should use compiler-owned Noqeri code;
- package site generator and checksum log: generation logic should migrate to Noqeri after filesystem/JSON support is runtime-complete;
- migration tool: rule engine should become an official Noqeri tool once source editing APIs stabilize;
- benchmark/test corpus runners: orchestration can use host process APIs initially, but workload logic must be language-neutral or implemented equivalently.

## Rules

1. Never route a missing stage-1 command to the historical C++ compiler without telling the user.
2. Keep the C++17 bootstrap branch as provenance/seed material, not a parallel feature implementation target.
3. Every migrated tool gets differential tests against the previous trusted implementation before the previous implementation is retired.
4. Host adapters may provide operating-system capabilities; business/language/tool policy belongs in Noqeri.
5. A `.nqr` wrapper around an unimplemented extern does not count as dogfooding.

The goal is not “zero host code.” The goal is that Noqeri owns the decisions while small audited hosts provide the capabilities that necessarily touch an OS/runtime.
