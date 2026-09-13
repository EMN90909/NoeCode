# Practical project corpus

Unit tests are necessary but not sufficient. A release should also compile or run representative projects through the public toolchain and package model.

`corpus.json` is the machine-readable inventory and `Tools/corpus/run.mjs` is the evidence runner. A filename alone never makes an application active.

## Required classes

| Class | Minimum evidence |
|---|---|
| CLI | parse arguments, format output, non-zero failure path |
| web server | route, request parsing, response, shutdown/error path |
| REST API | JSON input/output, validation, status codes |
| database app | schema, insert/update/query, persistence/reopen |
| websocket server | handshake/frame path, malformed frame rejection |
| file processor | bounded file IO, malformed input, output verification |
| TCP service | connect/listen/read/write/timeout/error path |
| GUI application | event/input/render boundary or supported host adapter |
| game/tool | update loop, collections/timing as relevant, deterministic test mode |
| embedded application | freestanding target contract, bounded memory, no accidental hosted dependency |

An entry is `active` only when it has non-skeletal source, expected behavior, a local executable check, a negative/error path, and a named supported target/profile. The runner verifies every active check and writes `build/corpus-results.json`.

```sh
node Tools/corpus/run.mjs --noqeri=build/noqeri
```

A release that intends to claim the complete ten-class corpus must use the stricter gate:

```sh
node Tools/corpus/run.mjs --noqeri=build/noqeri --require-complete
```

`--require-complete` fails when any required class is still pending. Missing host capabilities are therefore visible release blockers rather than silently treated as passing tests.

## Current activation state

The deterministic game/tool workload is active and is expected to compile, run ten update ticks, produce a stable result, and exercise an invalid-score path. The embedded workload is active as a freestanding compile/check workload with fixed iteration bounds, an explicit divide-by-zero-safe path, and a source policy that rejects host imports, externs, output and allocator calls.

The remaining classes are deliberately pending in `corpus.json`. The current reference desktop ABI does not yet expose application arguments, filesystem, HTTP/WebSocket/TCP, GUI, or ordinary application database services. Those classes become active when a real supported adapter can execute their positive and negative paths; parser-only or mocked examples do not qualify.

The corpus should grow by converting real examples and dogfooded tools into tested applications, not by creating skeletal files to hit a count.
