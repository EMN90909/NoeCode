# Noqeri beginner course

This course teaches ordinary application programming before systems programming. You should be able to build useful software without learning pointers, atomics, ABI layout or assembly.

Each chapter has one outcome. Type every example, change it, break it, read the diagnostic, and restore it.

## 1. Print

Goal: run/check a tiny program and see output.

```nqr
function main(): int {
    print("Hello, Noqeri")
    return 0
}
```

Exercise: print your name and one number. Do not introduce variables yet.

## 2. Variables

Use `let` for a value that may change and `const` when the binding should not change.

```nqr
function main(): int {
    let age: int = 28
    const next: int = 29
    print(age)
    print(next)
    return 0
}
```

Exercise: keep a running score and print it after two updates.

## 3. Decisions

```nqr
function label(score: int): string {
    if score >= 50 {
        return "pass"
    }
    return "try again"
}
```

Exercise: classify a temperature as cold, comfortable or hot.

## 4. Loops

```nqr
function sum_to(limit: int): int {
    let i: int = 1
    let total: int = 0
    while i <= limit {
        total = total + i
        i = i + 1
    }
    return total
}
```

Exercise: print the numbers 1 through 10 and calculate their total.

## 5. Functions

Keep functions small and name them for the result or action they provide.

```nqr
function area(width: int, height: int): int {
    return width * height
}
```

Exercise: split a price calculator into subtotal, discount and final-total functions.

## 6. Records

Records group related values without introducing inheritance or framework concepts.

```nqr
record User {
    id: int,
    name: string
}
```

Exercise: define a `Task` record with an id, title and completed flag.

## 7. Collections

Start with arrays/slices and the standard collection packages. Prefer collection APIs over manual memory management.

```nqr
function first(values: [int;3]): int {
    return values[0]
}
```

Exercise: store five scores, read them safely and calculate a total. Ordinary indexing is bounds checked where the runtime cannot prove the index statically.

## 8. Files

Use the safe filesystem wrapper. OS handles remain behind the implementation boundary.

```nqr
import "../Lib/std/fs.nqr"

function main(): int {
    let status = fs_write_text("note.txt", "Noqeri file example")
    if status < 0 as isize { return 1 }
    print(fs_read_text("note.txt"))
    return 0
}
```

Exercise: write a small journal entry, read it back and check that the file exists.

## 9. JSON

Use the standard JSON package for validation and streaming. Generic record reflection is still a maturity gate, so this course does not pretend `json.decode<User>` exists before it does.

```nqr
import "../Lib/std/json.nqr"

function main(): int {
    let text = "{\"ok\":true}"
    if !json_validate(text) { return 1 }
    print(text)
    return 0
}
```

Exercise: validate three JSON documents, including one deliberately malformed document.

## 10. HTTP

Application code should use high-level HTTP functions rather than sockets.

```nqr
import "../Lib/std/http.nqr"

function main(): int {
    let body = get("https://example.com")
    print(body)
    return 0
}
```

The call requires a hosted runtime with an HTTP provider. If your current host does not supply one, treat that as a provider capability boundary rather than writing socket code as a beginner.

Exercise: fetch a JSON endpoint and validate the response before using it.

## 11. Database

NoqeriDB uses `.nqd` as its native concise data language. SQL is an interoperability surface and should use parameters rather than string concatenation.

Example `.nqd`:

```text
table tasks { id: int key, title: text required, done: bool required }
insert tasks { id: 1, title: "Learn Noqeri", done: false }
select tasks where done = false
```

Exercise: create, update and query a small task database. Then repeat the operation through the parameterized SQL API when your host provides SQL execution.

## 12. Concurrency

Begin with tasks/scopes/channels. Shared-memory synchronization is a later tool, not the first concurrency lesson.

Conceptual shape:

```nqr
// A scope owns its children. Cancellation and join happen through the scope.
// See Lib/std/scope.nqr, task.nqr and channel.nqr for the current API.
```

Exercise: split two independent pieces of work into child tasks and collect their results. Run concurrency tests with `noqeri test --race` as runtime task scheduling reaches full parity.

---

# Advanced track

Do not begin here unless your program actually needs systems-level control.

## 13. Pointers and memory

Raw pointer dereference requires a narrow `unsafe` block.

```nqr
function read_raw(pointer: *int): int {
    unsafe {
        return *pointer
    }
}
```

Use `noqeri run program.nqr --check-memory` while developing low-level code.

## 14. Atomics

Atomics establish explicit synchronization ordering. Prefer channels/scopes/locks when they express the problem clearly. Use atomics when measurement or low-level requirements justify them.

## 15. FFI

FFI is an unsafe capability boundary. Keep ownership, lifetime, error and struct-layout rules explicit. See `Doc/ABI.md` and the evolving FFI contract before binding native libraries.

## 16. Assembly and intrinsics

Assembly/intrinsics are opt-in escape hatches, not ordinary optimization tools. Profile first. Prefer library/compiler improvements over application-specific assembly.

# Graduation projects

Build these in order:

1. a command-line notes or todo tool;
2. a JSON file transformer;
3. a small HTTP client/API;
4. a database CRUD application;
5. only then a concurrent service.

A beginner-friendly language is measured by what you can build before encountering its advanced machinery. If a chapter requires an unsafe feature to accomplish an ordinary task, treat that as a library/runtime design bug and report it.
