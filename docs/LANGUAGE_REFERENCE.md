# Noe Language Reference

This page documents the implemented bootstrap Noe syntax. Roadmap-only features are listed separately in `STATUS.md` and `ROADMAP.md`.

## Files

Noe source files use `.noe`. Project metadata uses `project.noe`.

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

## Functions

```noe
function add(a: int, b: int): int {
    return a + b
}
```

## Calls

```noe
print("Hello")
print(add(10, 20))
```

`print` is available as a bootstrap intrinsic.

## Operators

Arithmetic: `+ - * / %`

Comparison: `== != < <= > >=`

Logic: `! && ||`

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
