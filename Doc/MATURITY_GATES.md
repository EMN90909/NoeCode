# Noqeri maturity gates

This file distinguishes implemented behavior from API contracts and future service deployment. A feature is not marked complete merely because a function name exists.

| Area | Current evidence | Gate status |
| --- | --- | --- |
| Explicit `unsafe` boundary | Lexer/parser `unsafe {}`, AST unsafe blocks, `UnsafeChecker`, valid/invalid conformance cases | Implemented |
| Tasks/channels/mutex/RW-lock/structured scopes | Canonical std modules with cancellation/timeouts and explicit host boundaries | API implemented; desktop scheduler services still required |
| Atomics | Compiler/type checker/interpreter atomic operations | Implemented reference semantics |
| Race detection | Memory-model rule specified | Planned instrumentation |
| HTTP client/server | `std.http` simple get/post/request/serve API | API implemented; production HTTPS/TLS transport required |
| JSON | validation/string/stream API | API implemented; typed record reflection and desktop provider incomplete |
| Filesystem | complete std surface plus desktop ABI services | Implemented desktop foundation; platform-specific watcher/mmap performance can improve |
| NoqeriDB native `.nqd` | transactional bootstrap engine plus self-host page/WAL/index/planner/recovery modules | Partial execution parity |
| NoqeriDB `.sql` | parameterized SQL compatibility/prepared-query contract | Contract implemented; SQL parser/executor parity incomplete |
| PostgreSQL/MySQL-MariaDB/SQLite | official registry provider APIs | Provider contracts implemented; native/wire host adapters require deployment per platform |
| Deterministic package resolution | exact lockfile, SHA-256, transitive graph, re-hashed cache, offline mode, yanks | Implemented stage-1 foundation |
| Package signing | Ed25519 checksum signing/verification primitives | Implemented primitive; publisher key distribution/revocation service pending |
| Namespace ownership/security reports | registry machine-readable ledgers | Implemented format/process foundation |
| Checksum transparency | deterministic log/checkpoint generator | Implemented local foundation; `sum.noqeri.dev` deployment pending |
| Canonical `noqeri` tool | bootstrap/stage-1 frontends use the same command identity | Partial; command parity must be completed before 1.0 |
| Formatter | canonical formatter module | Implemented stage-1 formatter |
| LSP | completion, hover, definition, references, rename, symbols, diagnostics, signature help, semantic tokens, formatting, quick fixes | Implemented stage-1 server; compiler-backed semantic precision remains a production gate |
| Diagnostics | stable codes, source location, source line, caret span, help | Implemented compiler printer foundation |
| Documentation generation | exported API extraction + Markdown renderer | Implemented stage-1 foundation |
| Package documentation portal | registry static site/index generator with versions/source/license/deps/download/security/examples | Implemented generator; `pkg.noqeri.dev` deployment pending |
| 1.x compatibility promise | `Doc/COMPATIBILITY.md` | Defined, activates only at 1.0 gates |
| Syntax stabilization | compatibility policy forbids incompatible basic syntax changes after 1.0 | Defined policy |
| Language specification | `Doc/LANGUAGE_SPEC_2026.md` | Normative pre-1.0 foundation; provisional sections remain |
| Conformance suite | valid/invalid corpus + CTest/Node runner | Implemented foundation; corpus must grow to thousands |
| Differential testing | interpreter/native/web comparison runner | Implemented harness; backend availability/parity remains release-gated |

## 1.0 release rule

Noqeri must not describe an API-only row as production complete. 1.0 requires the compatibility gates in `Doc/COMPATIBILITY.md`, no unresolved core specification placeholders, a substantially expanded conformance corpus, backend differential parity, and runtime providers that match the standard-library contracts on every supported tier-1 platform.
