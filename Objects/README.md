# noqeri object/value model

The bootstrap runtime currently represents NIR values as null, signed 64-bit integer, double, boolean or string variants. Static source types are `void`, `null`, `bool`, `int`, `float` and `string`.

This directory owns the future heap-object ABI, string/storage semantics, aggregate representation, equality rules and memory-management contracts. New runtime objects must have explicit representation and ownership tests.
