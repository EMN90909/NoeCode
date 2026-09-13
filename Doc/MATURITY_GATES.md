# Noqeri maturity gates

This file distinguishes implemented behavior from API contracts and future service deployment. A feature is not marked complete merely because a function name exists.

| Area | Current evidence | Gate status |
| --- | --- | --- |
| Explicit `unsafe` boundary | Lexer/parser `unsafe {}`, AST unsafe blocks, `UnsafeChecker`, valid/invalid conformance cases | Implemented |
| Tasks | C++17 worker scheduler, named Noqeri task entry execution, timeout join, cancellation polling, cleanup tests | Implemented reference runtime; first-class function-value task entry remains a language ergonomics improvement |
| Channels/mutex/RW-lock | Canonical std modules backed by bounded channels, timed mutexes and shared timed locks; host regression tests | Implemented reference runtime |
| Structured scopes | std scope/task contracts with cancellation/timeouts | Implemented library contract; richer nursery syntax is deliberately not required |
| Atomics | Compiler/type checker/interpreter atomic operations | Implemented reference semantics |
| Race detection | Thread-aware raw-memory conflict instrumentation; spawn/join/atomic/synchronization boundaries; direct multithreaded host regression | Implemented first-generation reference detector; vector-clock precision and large stress corpus remain hardening work |
| Memory/overflow checks | `noqeri run/test --check-memory` and `--overflow`, allocation-range checks and checked signed arithmetic | Implemented reference modes |
| Native sanitizers | ASan+UBSan and separate ThreadSanitizer CMake modes | Implemented bootstrap build modes |
| Profiling | per-Noqeri-function CPU scopes, thread/call/allocation accounting, JSON and folded-stack export, `noqeri profile` | Implemented reference profiler |
| HTTP client/server | `std.http` simple get/post/request/serve API | API implemented; production HTTPS/TLS transport required |
| JSON | validation/string/stream API | API implemented; typed record reflection and desktop provider incomplete |
| Filesystem | complete std surface plus desktop ABI services | Implemented desktop foundation; platform-specific watcher/mmap performance can improve |
| Target build tags | `// noqeri:target` / `// noqeri:not-target`, compiler target filtering and regression test | Implemented |
| Object loader/linker boundary | bounded ELF64/COFF64/Mach-O64/Wasm inspector; linker validates format/architecture before external tool launch | Implemented bootstrap hardening |
| Fuzzing | deterministic lexer/parser/type/package/.nqd/SQL/HTTP/object mutation harness; object cases go through `inspect-object` | Implemented local foundation; long-running coverage-guided campaigns remain continuous hardening |
| Benchmarks | declared CPU/text/HTTP/fs/database/matrix/allocation/string/sort/startup/compilation workloads with C/C++/Rust/Zig/Go command adapters and methodology | Implemented harness; comparisons require published reproducible inputs/results |
| Performance regression gate | median baseline comparison with release threshold | Implemented harness; baseline ownership remains release-process work |
| NoqeriDB native `.nqd` | transactional bootstrap engine plus self-host page/WAL/index/planner/recovery policy modules | Transactional bootstrap implemented; production WAL/page/index execution parity remains evidence-gated |
| NoqeriDB `.sql` | executable adapter over the same `.nqdb` store: CREATE TABLE, INSERT, UPDATE, DELETE, transactions, projection/equality WHERE, qualified inner equality JOINs, GROUP BY, COUNT/SUM/MIN/MAX and ORDER BY selected columns/aliases; rollback and relational regression tests | Implemented practical relational bootstrap subset; advanced DDL, outer joins/window functions and full planner/index execution parity remain database-engine gates and fail closed |
| PostgreSQL/MySQL-MariaDB/SQLite | official registry provider APIs | Provider contracts implemented; native/wire host adapters require deployment per platform |
| Deterministic package resolution | exact lockfile, SHA-256, transitive graph, re-hashed cache, offline mode, yanks | Implemented stage-1 foundation |
| Vendoring/workspaces | exact-lock offline vendoring and `workspace.nqr` member list/check/install tooling with path protection | Implemented stage-1 foundation |
| Package signing | Ed25519 checksum signing/verification primitives | Implemented primitive; publisher key distribution/revocation service pending |
| Namespace ownership/security reports | registry machine-readable ledgers | Implemented format/process foundation |
| Dependency vulnerability scanning | machine-readable advisory database consumed by `noqeri audit` | Implemented local foundation; advisory distribution process must stay maintained |
| Checksum transparency | deterministic log/checkpoint generator | Implemented local foundation; `sum.noqeri.dev` deployment pending |
| Reproducible builds | clean double-build verifier, deterministic environment policy, artifact tree hash comparison | Implemented verifier; all release targets must still demonstrate reproducibility |
| Signed releases/SBOM | SHA-256 manifest, SPDX 2.3 SBOM and optional Ed25519 signing generator | Implemented local release tooling; public platform artifacts/signing ceremony remain release work |
| Canonical `noqeri` tool | bootstrap frontend includes run/build/test/fmt/check/db/profile/inspect/audit/etc.; stage-1 shares command identity | Partial; full self-host command parity must be completed before 1.0 |
| Formatter | canonical formatter module | Implemented stage-1 formatter |
| LSP | completion, hover, definition, references, rename, symbols, diagnostics, signature help, semantic tokens, formatting, quick fixes | Implemented stage-1 server; compiler-backed semantic precision remains a production gate |
| Diagnostics | stable codes, source location, source line, caret span, help | Implemented compiler printer foundation |
| Documentation generation | exported API extraction + Markdown renderer | Implemented stage-1 foundation |
| Package documentation portal | registry static site/index generator with versions/source/license/deps/download/security/examples | Implemented generator; `pkg.noqeri.dev` deployment pending |
| Beginner course/usability evidence | application-first course plus standardized timing/event recorder for first program and real-app tasks | Measurement framework implemented; published external participant studies remain evidence work |
| Practical project corpus | CLI, web server, REST API, database, WebSocket, file processor, TCP, GUI, game/tool and embedded project fixtures | Implemented corpus structure; provider-dependent projects become release gates for their providers |
| FFI/bindings | documented ownership/error/layout/callback conventions plus fail-closed C header and binding generators | Implemented preview foundation; ABI conformance corpus remains required for stable label |
| Migration tooling | machine-readable rewrite rules and preview-first migration runner | Implemented foundation |
| 1.x compatibility promise | `Doc/COMPATIBILITY.md` | Defined, activates only at 1.0 gates |
| Syntax stabilization | compatibility policy forbids incompatible basic syntax changes after 1.0 | Defined policy |
| Language specification | `Doc/LANGUAGE_SPEC_2026.md` | Normative pre-1.0 foundation; provisional sections remain |
| Conformance suite | valid/invalid corpus + CTest/Node runner | Implemented foundation; corpus must grow to thousands |
| Differential testing | interpreter/native/web comparison runner | Implemented harness; backend availability/parity remains release-gated |

## 1.0 release rule

Noqeri must not describe an API-only row as production complete. 1.0 requires the compatibility gates in `Doc/COMPATIBILITY.md`, no unresolved core specification placeholders, a substantially expanded conformance corpus, backend differential parity, and runtime providers that match the standard-library contracts on every supported tier-1 platform.

No benchmark or marketing page may claim that Noqeri is faster, easier, safer or more compatible than another language merely because a harness exists. Claims require published inputs, environment metadata, results and reproducible methodology.
