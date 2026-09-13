#pragma once

#include "diagnostics.hpp"
#include <string>

namespace noe {

struct UnsafePreprocessResult {
    std::string source;
    bool foundUnsafeBlock = false;
};

// Recognizes lexical `unsafe { ... }` blocks, preserves byte offsets by
// blanking only the `unsafe` keyword, and emits diagnostics for operations that
// require an explicit unsafe boundary.
UnsafePreprocessResult preprocessUnsafeSyntax(const std::string& source,
                                              Diagnostics& diagnostics,
                                              SourceId sourceId = 0);

} // namespace noe
