# Fuzzing plan

Priority fuzz targets are lexer tokenization, parser recovery, type-checker diagnostics, formatter idempotence and LSP message framing. A fuzz target is not considered active until CI or a documented local harness runs it with reproducible seeds/corpus handling.
