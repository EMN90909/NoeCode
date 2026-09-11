# noqeri 1.3 grammar

```text
program        := declaration* EOF
declaration    := modifiers? functionDecl | recordDecl | letDecl | statement
modifiers      := ("export" | "extern")+
functionDecl   := "function" IDENT "(" parameters? ")" (":" type)? (block | ";"?)
recordDecl     := "record" IDENT "{" recordField* "}" ";"?
recordField    := IDENT ":" type ("," | ";")?
parameters     := parameter ("," parameter)*
parameter      := IDENT (":" type)?
type           := ("*" "volatile"?)? IDENT

letDecl        := ("let" | "const") IDENT (":" type)? "=" expression ";"?
statement      := ifStmt | whileStmt | returnStmt | block | exprStmt
ifStmt         := "if" expression block ("else" statement)?
whileStmt      := "while" expression block
returnStmt     := "return" expression? ";"?
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
unary          := ("!" | "-" | "+" | "*" | "&") unary | cast
cast           := postfix ("as" type)*
postfix        := primary (callSuffix | indexSuffix | memberSuffix)*
callSuffix     := "(" arguments? ")"
indexSuffix    := "[" expression "]"
memberSuffix   := "." IDENT
primary        := INTEGER | FLOAT | STRING | "true" | "false" | "null" | IDENT | "(" expression ")"
```

Stable integer names are `u8`, `u16`, `u32`, `u64`, `i8`, `i16`, `i32`, `i64`, `usize`, and `isize`. Existing `int`, `float`, `bool`, `string`, `null`, and `void` remain available.

Pointers are written `*T`. A volatile pointee access is written `*volatile T`. `&value` takes an address, `*pointer` dereferences, and `pointer[index]` scales by the pointee size.

`record` uses declaration-order, naturally aligned field layout. `extern function` declares a symbol supplied by the surrounding program. `export function` emits the declared name as an externally visible native symbol.

These are general language facilities for systems, embedded, FFI, runtimes, libraries and performance-sensitive programs. They are not tied to an operating-system profile.
