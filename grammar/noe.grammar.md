# Noe grammar notes

This document tracks the currently implemented bootstrap grammar. It is intentionally smaller than the future Noe language specification.

```text
program        = declaration* EOF

declaration    = function_decl
               | let_decl
               | const_decl
               | statement

function_decl  = "function" IDENTIFIER "(" parameters? ")" return_type? block
parameters     = parameter ("," parameter)*
parameter      = IDENTIFIER (":" IDENTIFIER)?
return_type    = ":" IDENTIFIER

let_decl       = "let" IDENTIFIER type_annotation? "=" expression ";"?
const_decl     = "const" IDENTIFIER type_annotation? "=" expression ";"?
type_annotation = ":" IDENTIFIER

statement      = if_stmt
               | while_stmt
               | return_stmt
               | block
               | expression_stmt

if_stmt        = "if" expression statement ("else" statement)?
while_stmt     = "while" expression statement
return_stmt    = "return" expression? ";"?
block          = "{" declaration* "}"
expression_stmt = expression ";"?

expression     = assignment
assignment     = logical_or ("=" assignment)?
logical_or     = logical_and ("||" logical_and)*
logical_and    = equality ("&&" equality)*
equality       = comparison (("==" | "!=") comparison)*
comparison     = term (("<" | "<=" | ">" | ">=") term)*
term           = factor (("+" | "-") factor)*
factor         = unary (("*" | "/" | "%") unary)*
unary          = ("!" | "-" | "+") unary | call
call           = primary ("(" arguments? ")")*
primary        = INTEGER | FLOAT | STRING | "true" | "false" | "null" | IDENTIFIER | "(" expression ")"
```

## Current primitive types

```text
int
float
bool
string
void
null
```

## Future grammar work

Records, classes, interfaces, traits, modules/import resolution, pattern matching, generics, async/await, FFI declarations, attributes and system-profile constructs are planned after the current bootstrap subset.
