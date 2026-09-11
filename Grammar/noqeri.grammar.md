# noqeri 1.4 grammar

```text
program        := declaration* EOF
declaration    := modifiers? functionDecl
                | moduleDecl
                | importDecl
                | recordDecl
                | letDecl
                | statement

modifiers      := ("export" | "extern")+
moduleDecl     := "module" qualifiedName ";"?
importDecl     := "import" STRING ";"?
qualifiedName  := IDENT ("." IDENT)*

functionDecl   := "function" IDENT genericParams? "(" parameters? ")" (":" type)? (block | ";"?)
genericParams  := "<" IDENT ("," IDENT)* ">"
recordDecl     := "record" IDENT "{" recordField* "}" ";"?
recordField    := IDENT ":" type ("," | ";")?
parameters     := parameter ("," parameter)*
parameter      := IDENT (":" type)?

type           := "*" "volatile"? type
                | "[" "]" type
                | "[" type ";" INTEGER "]"
                | IDENT

letDecl        := ("let" | "const") IDENT (":" type)? "=" expression ";"?
statement      := ifStmt | whileStmt | returnStmt | throwStmt | block | exprStmt
ifStmt         := "if" expression block ("else" statement)?
whileStmt      := "while" expression block
returnStmt     := "return" expression? ";"?
throwStmt      := "throw" expression ";"?
block          := "{" declaration* "}"
exprStmt       := expression ";"?

expression     := assignment
assignment     := logicalOr ("=" assignment)?
logicalOr      := logicalAnd ("||" logicalAnd)*
logicalAnd     := equality ("&&" equality)*
equality       := comparison (("==" | "!=") comparison)*
comparison     := term (("<" | "<=" | ">" | ">=") term)*
term           := factor (("+" | "-") factor)*
factor         := unary (("*" | "/" | "%") unary)*
unary          := ("!" | "-" | "+" | "*" | "&" | "try") unary | cast
cast           := postfix ("as" type)*
postfix        := primary (callSuffix | indexSuffix | memberSuffix)*
callSuffix     := "(" arguments? ")"
indexSuffix    := "[" expression "]"
memberSuffix   := "." IDENT
primary        := INTEGER
                | FLOAT
                | STRING
                | "true"
                | "false"
                | "null"
                | IDENT
                | arrayLiteral
                | "(" expression ")"
arrayLiteral   := "[" (expression ("," expression)*)? "]"
```

Stable integer names are `u8`, `u16`, `u32`, `u64`, `i8`, `i16`, `i32`, `i64`, `usize`, and `isize`. Existing `int`, `float`, `bool`, `string`, `null`, and `void` remain available.

Pointers are written `*T`. A volatile pointee access is written `*volatile T`. `&value` takes an address, `*pointer` dereferences, and indexing scales by the element size.

Fixed arrays use `[T; N]`; slices use `[]T`. Array literals infer their element type from their elements. `slice(array)` or `slice(pointer, length)` creates a pointer-and-length view, and `len(value)` accepts arrays and slices.

`module name` gives a source file an organizational identity. `import "path.nqr"` resolves recursively relative to the importing file. The current bootstrap module model merges imported declarations into one compilation graph; namespacing can evolve later without changing file import syntax.

Generic functions use inferred register-value type parameters, for example `function identity<T>(value: T): T`. The bootstrap implementation deliberately limits generics to values that fit the current register-oriented ABI; generic aggregate specialization is not yet implied by this grammar.

`throw code` and `try expression` provide allocation-free status propagation for integer-returning functions. A thrown non-negative code is encoded as a negative status; `try` returns immediately when it sees a negative status.

Atomics, CPU intrinsics and inline assembly are ordinary calls rather than new grammar families: `atomic.load(ptr)`, `atomic.store(ptr, value)`, `atomic.exchange(...)`, `atomic.compareExchange(...)`, `atomic.fence()`, `intrinsic("x86.pause")`, and `asm("nop")`.

`record` uses declaration-order, naturally aligned field layout. `extern function` declares a symbol supplied by the surrounding program. `export function` emits the declared name as an externally visible native symbol.

These are general language facilities for applications, libraries, embedded software, runtimes, concurrency, FFI, kernels and other performance-sensitive programs. They are not tied to an operating-system profile.
