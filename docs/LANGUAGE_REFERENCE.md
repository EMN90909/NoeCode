# Noe Language Reference

This page documents the implemented bootstrap Noe syntax. Roadmap-only features are listed separately in `STATUS.md` and `ROADMAP.md`.

## Files

Noe source files use the `.noe` extension.

```text
src/main.noe
```

Project metadata uses `project.noe`. Noe projects do not use TOML.

## Comments

```noe
// line comment

/* block comment */
```

## Variables

```noe
let name = "Noe"
let count: int = 10
const enabled = true
```

`let` declares a local variable. `const` is parsed and tracked as a declaration form; stronger mutation enforcement is a future refinement.

## Primitive types

Implemented bootstrap primitives:

```text
int
float
bool
string
void
null
```

The type checker infers obvious primitive values:

```noe
let a = 10       // int
let b = 1.5      // float
let ok = true    // bool
let msg = "hi"   // string
```

## Functions

```noe
function add(a: int, b: int): int {
    return a + b
}
```

Function parameters should be typed. Return types are checked when provided.

## Calls

```noe
print("Hello")
print(add(10, 20))
```

`print` is available as a bootstrap intrinsic.

## Operators

Arithmetic:

```text
+  -  *  /  %
```

Comparison:

```text
==  !=  <  <=  >  >=
```

Logic:

```text
!  &&  ||
```

Assignment:

```noe
let i = 0
i = i + 1
```

## Control flow

```noe
if (score > 10) {
    print("high")
} else {
    print("low")
}
```

Parentheses around conditions are accepted. Brace blocks are canonical.

## While loops

```noe
let i = 0

while (i < 3) {
    print(i)
    i = i + 1
}
```

## Current limitations

The long-term Noe design includes records, classes, interfaces, generics, pattern matching, async, FFI, unsafe systems features, visual programming and more. Those are not complete in the bootstrap compiler yet.
