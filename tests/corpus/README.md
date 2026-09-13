# Practical project corpus

This directory defines the release-level application corpus. Unit tests are necessary but not sufficient: a release should also compile/run representative projects through the public toolchain and package model.

Required project classes:

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
| game/tool | update loop, collections, timing, deterministic test mode |
| embedded application | freestanding target contract, bounded memory, no accidental hosted dependency |

A corpus entry is not considered active because a filename exists. It becomes active only when it has: source, expected behavior, a local test command, negative/error cases, and a named supported target/profile. The release report must list active vs pending entries explicitly.

The corpus should grow by converting real examples and dogfooded tools into tested applications, not by creating skeletal files to hit a count.
