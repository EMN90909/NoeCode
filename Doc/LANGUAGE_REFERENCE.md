# noqeri 1.0 core language reference

noqeri is statically checked before NIR execution or native code generation. The stable grammar stays intentionally small: capability grows through ordinary calls and the host ABI rather than low-level syntax.

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

The stable core supports calls, unary `!`/`-`, arithmetic `+ - * / %`, comparisons, equality and logical `&&` / `||`, with conventional precedence.

## Services without syntax growth

Built-in runtime services remain ordinary named calls:

```nqr
print(platform())
let started: int = clockMillis()
print(textLength("simple"))
```

Embedders may register additional services through ABI v1 and call them with the ordinary function-call form:

```nqr
let response = host("app.lookup", "item-42")
print(response)
```

`host` does not add pointer, address, memory, record, array or intrinsic expressions to the language. It is lowered through the existing NIR `Call` operation. See `Doc/ABI.md` for the versioned C boundary.

## Deliberately outside the stable grammar

The lexer reserves `import`, `record` and `class`, but they are not documented as stable implemented grammar until parser, type, NIR and runtime support exists. Pointer operators, address-of/dereference expressions, raw-memory access, architecture intrinsics and volatile operations are likewise not part of the 1.0 language.
