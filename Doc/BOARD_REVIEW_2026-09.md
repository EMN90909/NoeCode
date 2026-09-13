# Noqeri Product Board Review — September 2026

This review turns the language, runtime and ecosystem into one product problem instead of treating compiler, library and documentation work as unrelated tasks.

## Board roles

- **Planning:** scans the repository, compares mature language ergonomics, prioritises the highest-leverage work and rejects cosmetic line-count targets.
- **Kid tester:** approaches Noqeri like a curious nine-year-old. The test is not whether every feature is child-oriented; it is whether names, errors, examples and mental models can be understood without tribal knowledge.
- **Design:** owns syntax, naming, error clarity, documentation hierarchy and teachability.
- **Development:** owns compiler semantics, runtime guarantees, standard-library behaviour and ecosystem implementation.
- **Testing:** owns positive, negative, safety, portability and performance evidence.
- **Marketing:** only makes claims that testing can substantiate. Safety and speed are evidence products, not slogans.

## Repository scan

The standard library contains a strong middle tier (`math`, `json`, `bytes`, `text`, `utf8`, `base64`, `csv`, `date`, `random`, collections and IO), but many modules were still placeholder-sized. Examples at the start of this review included `arena`, `ascii`, `backoff`, `binary`, `bitset`, `bloom`, `cache`, `calendar`, `channel`, `clock`, `crc`, `deque`, `duration`, `encoding`, `event`, `future`, `hash`, `heap`, `html`, `ini`, `iterator`, `lexer`, `markdown`, `metrics`, `mime`, `numeric`, `parser`, `pool` and `priority_queue`.

The board explicitly rejects a rule that every module must exceed 30 KB. A file can reach 30 KB by repetition and still be useless. The maturity gate is instead:

1. a clear abstraction and vocabulary;
2. useful core operations;
3. validation and defined failure behaviour;
4. safe defaults;
5. examples that show the normal path first;
6. focused tests, including failure tests;
7. benchmarks for performance-sensitive code;
8. no fake aliases added only to increase line count.

For broad modules 200+ lines is a useful warning threshold, not a goal by itself. Narrow modules may legitimately be smaller when the entire useful domain is covered.

## Kid-tester session

The kid tester tried to answer five questions for each module: “What is this?”, “How do I make one?”, “What is the first useful thing I can do?”, “What happens when I make a mistake?”, and “Can I copy a tiny example and change it?”

### Friction found

- Several module names existed but the files contained only a handful of generic helpers, so the file did not teach the abstraction it named.
- Low-level names such as `checked_end` are useful to experienced systems programmers but need a surrounding story: capacity, allocation, marks, alignment and failure.
- Safety behaviour was split between compile-time analysis and backend behaviour. A beginner should not need to know which backend they are using to understand whether `items[i]` is checked.
- Pointer/null behaviour was not represented as explicit NIR operations, making the guarantee difficult to audit and optimise consistently.
- “Fast” had benchmarks, but the repository did not yet have one evidence document connecting benchmark method, safety mode and claims.
- The standard library catalogue was wide enough to look mature before every individual module was deep enough to feel mature.

### Kid-tester suggestions

1. Keep the first program tiny: values, conditions, loops, functions and collections should stay readable before introducing systems concepts.
2. Teach one mental model per module. `arena` should teach “a cursor inside a capacity”; `backoff` should teach “attempt -> delay -> stop”; `ascii` should teach “one byte, one classification”.
3. Put advanced power behind explicit words, not punctuation puzzles. Unsafe memory operations should eventually live inside `unsafe { ... }` while ordinary arrays/slices remain checked.
4. Make runtime errors say the two numbers that matter. Bounds failures should include the index and length.
5. Keep safe code short. A checked index should still be written `items[i]`, not `checked_get(items, i)` everywhere.
6. Teach failures with runnable examples, not prose alone.

## Planning research

The planning board compared two useful precedents:

- Go keeps indexing syntax ordinary while specifying that out-of-range array/slice indices panic at runtime when they cannot be rejected statically. Nil pointer indirection also has defined failure behaviour.
- Rust makes the boundary for operations requiring programmer proof explicit with `unsafe { ... }`, while keeping the borrow checker and other safety checks active around that boundary.

The board vote is to combine those ideas without copying either language wholesale: **ordinary Noqeri stays compact and checked; operations that cannot be made safe by the compiler get an explicit unsafe boundary; NIR carries checks explicitly so optimisation can remove checks only when it proves them redundant.**

## Board vote

| Proposal | Planning | Kid tester | Design | Development | Testing | Marketing | Decision |
|---|---:|---:|---:|---:|---:|---:|---|
| Inflate every std file to 30 KB | No | No | No | No | No | No | Rejected |
| Deepen modules by real API coverage | Yes | Yes | Yes | Yes | Yes | Yes | Adopted |
| Bounds checks for dynamic slice/fixed-array indices | Yes | Yes | Yes | Yes | Yes | Yes | Adopted |
| Runtime null checks before dereference | Yes | Yes | Yes | Yes | Yes | Yes | Adopted |
| Explicit NIR safety operations | Yes | Neutral | Yes | Yes | Yes | Yes | Adopted |
| Optional checked-overflow execution | Yes | Yes | Yes | Yes | Yes | Yes | Adopted |
| Explicit `unsafe {}` syntax | Yes | Yes | Yes | Yes | Yes | Yes | Approved, compiler enforcement remains a gated follow-up |
| Aggregate/interprocedural borrow summaries | Yes | Neutral | Yes | Yes | Yes | Yes | Approved, staged implementation required |
| Publish performance claims without measurements | No | No | No | No | No | No | Rejected |

## Implemented in this tranche

- Added explicit NIR `CheckBounds` and `CheckNonNull` operations.
- Lowered slice and fixed-bound indexing through runtime bounds checks in the reference execution pipeline when the compiler cannot simply rely on a literal/static rejection.
- Lowered raw pointer/slice access through runtime null checks in the reference execution pipeline.
- Added `NOQERI_CHECKED_OVERFLOW=1` to the reference interpreter. In that mode integer add/subtract/multiply/divide/negate overflow traps instead of silently continuing. Default integer execution is implemented with defined wrapping arithmetic rather than C++ signed-overflow undefined behaviour.
- Deepened `std/ascii`, `std/arena` and `std/backoff` from placeholder-style helper files into cohesive modules.

## Safety staging

The board does **not** call the following complete yet:

### Explicit unsafe blocks

The target surface is deliberately small:

```noqeri
unsafe {
    *device_register = value
}
```

Safe code should not need `unsafe` for ordinary indexing, slices, records, strings, collections or FFI wrappers that expose a checked API. Raw pointer dereference, unchecked pointer arithmetic, volatile/raw device access and inline assembly are candidate unsafe operations. Parser support and enforcement must land together with migration of internal low-level code; adding a decorative keyword that does not enforce anything is not acceptable.

### Full aggregate/interprocedural borrow analysis

Current lifetime analysis already catches several local dangling/null/fixed-bound cases. The next maturity gate is function summaries containing at least:

- which parameters can escape through the return value;
- which parameters can escape into aggregates or longer-lived storage;
- which aggregate fields contain borrows and their provenance;
- invalidation after scope end or mutation;
- call-site checking using summaries rather than treating a call as opaque;
- conservative recursion/fixpoint handling;
- diagnostics that name the value, field, call and lifetime that conflict.

This must be tested before it is advertised as “full”.

## Standard-library depth queue

Priority A: `binary`, `bitset`, `bloom`, `cache`, `deque`, `heap`, `priority_queue`, `parser`, `lexer`, `encoding`.

Priority B: `calendar`, `clock`, `duration`, `event`, `future`, `channel`, `pool`, `iterator`, `numeric`, `metrics`.

Priority C: format/domain modules such as `html`, `markdown`, `mime`, `ini`, plus specialist checksum/hash wrappers.

Each module exits the queue only with API tests and at least one real example. Modules involving cryptography, concurrency, parsing or networking require stricter adversarial tests than pure arithmetic helpers.

## Performance evidence policy

Every performance claim must record:

- commit SHA;
- compiler/build profile;
- target OS/architecture;
- CPU where available;
- checked-overflow and other safety modes;
- warmup count;
- sample count;
- median plus a tail statistic;
- benchmark source file;
- input size;
- whether the result is interpreter, native or web backend;
- comparison baseline, if a comparison is claimed.

Bounds/null checks are explicit NIR instructions so later optimisation can eliminate a check only with a proof (for example, a dominating range check or statically known index). The benchmark suite should measure checked code before and after elimination rather than disabling safety globally to win a chart.

## Marketing language allowed today

Allowed: “Noqeri is adding explicit runtime safety checks to the reference execution pipeline and a reproducible performance-evidence process.”

Not yet allowed: “all backends are memory safe”, “zero-cost bounds checks”, “full borrow checker”, “Rust-equivalent safety”, or any fixed speedup percentage without corresponding evidence.

## Release gate

A safety/performance tranche can be called complete only when:

1. compiler builds cleanly with warnings-as-errors on a supported host;
2. positive language tests pass;
3. OOB/null/overflow negative tests fail in the expected way;
4. native and web backends either implement the new NIR checks or explicitly reject unsupported checked NIR instead of silently dropping it;
5. std modules touched in the tranche pass focused tests;
6. benchmark output records the metadata above;
7. documentation and website claims match what testing actually proved.
