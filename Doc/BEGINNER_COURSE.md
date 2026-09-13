# Noqeri beginner course

The beginner track deliberately teaches ordinary application programming before systems mechanisms. A learner should be able to build useful software without first understanding pointers, ABI details, atomics, assembly, or native object formats.

## Core track

1. **Print** — run one file, print text and numbers, read an error message.
2. **Variables** — `let`, `const`, basic types, names, simple expressions.
3. **Decisions** — booleans, comparisons, `if`/`else`.
4. **Loops** — `while`, counters, bounded iteration, avoiding accidental infinite loops.
5. **Functions** — parameters, return values, small reusable units, clear failure messages.
6. **Records** — model a user/domain object; construct and read fields.
7. **Collections** — lists/maps/sets/queues through general-purpose APIs, not type-specific file trivia.
8. **Files** — read, transform, and write a text file safely.
9. **JSON** — validate/parse a small document and produce JSON output.
10. **HTTP** — make a request, then build a minimal endpoint.
11. **Database** — create a table, insert/update/select records, handle errors.
12. **Concurrency** — tasks/channels first; race safety and cancellation before atomics.

Each chapter should end with one runnable program, one deliberate mistake exercise, and one small extension. Course examples are compatibility-test inputs: a language change that breaks them requires either a migration or a conscious edition boundary.

## Advanced track

Only after the core track introduce:

1. pointers and address lifetimes;
2. explicit `unsafe` blocks and why they are exceptional;
3. atomics and memory ordering;
4. C ABI/FFI ownership and error conventions;
5. native layouts and object formats;
6. inline assembly and target-specific code.

## Usability benchmarks

Beginner usability is measured, not asserted. Record anonymously aggregated task timing and blockers for:

- time to install + first successful `print`;
- time to diagnose the first syntax/type error;
- time to build a CLI;
- time to build a JSON transformer;
- time to build an HTTP API;
- time to build a small database CRUD application.

When comparing Python, Go, Lua and JavaScript, use equivalent tasks, fresh participants where possible, identical help constraints, and publish the study protocol. Do not call Noqeri "#1 easiest" without evidence.

## Language complexity budget

Every proposed language feature answers, in order:

1. Can this be a library feature? If yes, prefer the library.
2. Does an existing construct already express the idea clearly? If yes, do not add a synonym.
3. Does it force ordinary users to learn pointers, atomics, ABI, native layout, assembly or another advanced mechanism? If yes, keep it opt-in.
4. Does it make diagnostics, formatting, tooling or migration materially harder? Include that cost in the proposal.

Safety ambition may be high while the language surface remains small. Predictable, boring syntax is a maturity feature.
