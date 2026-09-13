# Application standard library contracts

Noqeri's application-facing standard library aims to make common work boring: HTTP, JSON, filesystems, concurrency and databases should have one obvious path.

## HTTP

The public API is expected to support simple client calls (`http_get`, `http_post`, `http_request`) and hosted servers (`http_serve`) without exposing sockets. TLS, redirects, proxy behavior and certificate validation belong to the runtime transport implementation, not user code.

## JSON

The canonical API is `json_encode` / `json_decode` with streaming reader/writer support for large payloads. Reflection/generic decoding is part of the language-runtime roadmap; until generic record reflection is fully available, typed adapters must be generated rather than implemented with unsafe casts.

## Filesystem

The complete filesystem contract includes file read/write, directories, recursive walking, metadata, permissions, links, temporary files, watching, atomic replacement and memory mapping where the target supports it. Unsupported platform features must return an explicit status rather than silently degrade.

## Databases

NoqeriDB remains the native embedded database. `.nqd` is the concise native schema/query format and `.sql` is the standards-oriented interface. External database drivers should implement a common connection/query/transaction contract for PostgreSQL, MySQL/MariaDB and SQLite without leaking driver-specific connection state into application code.
