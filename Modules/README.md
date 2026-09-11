# Built-in and native modules

This area owns Ric's built-in/native module boundary. The lexer reserves `import`, but stable import semantics and module loading are not yet implemented by the bootstrap parser/runtime.

Native modules must eventually expose a versioned ABI, capability/error model and deterministic discovery rules. Until then this directory documents the boundary rather than pretending unsupported modules are available.
