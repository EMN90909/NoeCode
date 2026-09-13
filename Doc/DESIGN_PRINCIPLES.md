# Noqeri design principles

Noqeri's goal is not to maximize language features. It is to make ordinary software predictable while retaining opt-in systems power.

## Complexity is a budget

Every proposal that changes syntax, type-system rules or core semantics must answer these questions before implementation:

1. **Can this be a library?** If yes, prefer the library unless syntax materially improves safety, analyzability or interoperability.
2. **Does Noqeri already have a way to express this?** If yes, extend the existing mechanism instead of adding a synonym.
3. **Will a beginner meet this in their first real application?** If yes, the mental model must be explainable without pointers, ABI rules, ownership jargon or compiler internals.
4. **Does the feature require a permanent new interaction rule with generics, errors, concurrency, FFI or tooling?** Count that interaction as part of the cost.
5. **Can the feature be removed before 1.0 if it proves unnecessary?** Experimental syntax needs an explicit exit path.
6. **What conformance tests freeze its semantics?** A stable feature without tests is not stable.

## One obvious way

Noqeri should not accumulate three mechanisms for the same ordinary task. New APIs should converge on one conventional spelling and deprecations should ship with migration tooling where practical.

## Advanced power is opt-in

Application developers should not need raw pointers, atomics, native layouts, FFI, inline assembly or unrestricted host capabilities for files, JSON, HTTP, databases, collections or normal concurrency. Those operations live behind safe standard-library/provider APIs.

`unsafe` is a narrow audit boundary, not a general programming mode.

## Borrow safety ambition, not learning-curve complexity

Noqeri should pursue strong safe-code guarantees without reproducing Rust's entire language surface. Prefer compiler inference, bounded rules and library abstractions over additional user-visible annotation systems.

## Do not accumulate C++-style alternatives

Compatibility does not justify preserving every experiment forever. Before 1.0, remove weak redundant mechanisms. After 1.0, evolve additively and use editions/migration tools when a breaking improvement is genuinely necessary.

## Boring is good

Maturity means that mundane code is unsurprising:

- one project/lock workflow;
- one formatter;
- one test command;
- one ordinary error model;
- predictable file/JSON/HTTP/database APIs;
- stable diagnostics and compatibility rules;
- deterministic package and build inputs.

Novel syntax is not a maturity metric.

## Feature review scorecard

A proposal should include a small scorecard:

| Question | Required evidence |
| --- | --- |
| Why language, not library? | concrete safety/analyzability/interoperability reason |
| Existing mechanism reused? | yes, or explanation why impossible |
| Beginner cost | concepts added to first-real-app path |
| Tooling cost | parser/type checker/formatter/LSP/doc/migration impact |
| Runtime cost | ABI/runtime/optimizer/concurrency impact |
| Compatibility cost | stable behavior and edition implications |
| Test plan | conformance + fuzz + practical corpus cases |
| Removal/migration plan | required for experimental features |

A feature that cannot justify its complexity budget should not enter the language merely because it is technically possible.
