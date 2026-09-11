# Core types

The bootstrap type checker recognizes `void`, `null`, `bool`, `int`, `float` and `string`. `int` values may be assigned to `float`; other implicit widening/conversion rules are intentionally conservative. Unknown type names are diagnosed through the type-checking path rather than silently becoming dynamic values.
