# Ric 1.0 core language reference

Ric is statically checked before NIR execution or native code generation.

## Lexical form

Identifiers use ASCII letters/underscore followed by letters, digits or underscore. Line comments use `//`. Strings use double quotes. Integer, floating-point, boolean (`true`, `false`) and `null` literals are recognized.

## Declarations

```ric
let count: int = 0
const name: string = "Ric"

function add(a: int, b: int): int {
    return a + b
}
```

`const` bindings cannot be reassigned. Type annotations use `:`. Core type names are `void`, `null`, `bool`, `int`, `float` and `string`.

## Control flow

```ric
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

## Built-ins

`print(...)` is available in the current runtime. Additional standard-library and module APIs in `Lib/` are staged conservatively and must not be documented as implemented until parser/runtime support exists.

## Files and projects

Program files use `.ric`. Project metadata is `project.ric`; deterministic dependency state is `ric.lock`.
