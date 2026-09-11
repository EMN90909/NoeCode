# Ric 1.0 bootstrap grammar

This is the human-readable grammar contract for syntax implemented by the bootstrap parser.

```text
program        := declaration* EOF
declaration    := functionDecl | letDecl | statement
functionDecl   := "function" IDENT "(" parameters? ")" (":" type)? block
parameters     := parameter ("," parameter)*
parameter      := IDENT (":" type)?
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
unary          := ("!" | "-") unary | call
call           := primary ("(" arguments? ")")*
primary        := INTEGER | FLOAT | STRING | "true" | "false" | "null" | IDENT | "(" expression ")"
type           := IDENT
```

The lexer also reserves `import`, `record` and `class`; they are intentionally not presented as stable implemented grammar until parser/type/runtime support is complete.
