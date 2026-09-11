# Built-in and native modules

This area owns noqeri's built-in/native module boundary. The lexer reserves `import`, but stable import semantics and module loading are not yet implemented by the bootstrap parser/runtime.

Native modules will require a versioned ABI, capability/error model and deterministic discovery rules. Until that exists, this directory documents the boundary rather than advertising unsupported modules.
