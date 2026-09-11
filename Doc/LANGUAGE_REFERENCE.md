# noqeri 1.0 core language reference

noqeri is statically checked before NIR execution or native code generation.

## Lexical form

Identifiers use ASCII letters/underscore followed by letters, digits or underscore. Line comments use `//`. Strings use double quotes. Integer, floating-point, boolean (`true`, `false`) and `null` literals are recognized.

## Declarations

```nqr
let count: int = 0
const name: string = "noqeri"

function add(a: int, b: int): int {
    return a + b
}
```

`const` bindings cannot be reassigned. Core type names are `void`, `null`, `bool`, `int`, `float` and `string`.

## Control flow

```nqr
if count < 10 {
    print(count)
} else {
    print("done")
}

while count < 10 {
    count = count + 1
}
```

## Expressions

The bootstrap supports calls, unary `!`/`-`, arithmetic `+ - * / %`, comparisons, equality and logical `&&` / `||`, with conventional precedence.

The lexer reserves `import`, `record` and `class`, but they are not documented as stable implemented grammar until parser/type/runtime support exists.
