#!/usr/bin/env sh
set -eu

required='README.md LICENSE LEGAL.md SECURITY.md CONTRIBUTING.md CMakeLists.txt project.nqr noqeri.lock Brand/noqeri-logo.webp Brand/noqeri-mark.png Grammar/noqeri.grammar.md Include/noqeri/noqeri.hpp Parser/lexer.cpp Parser/parser.cpp Parser/ascii.nqr Parser/token_rules.nqr Compiler/core.cpp Compiler/semantic_rules.nqr Runtime/interpreter.cpp Runtime/status.nqr Runtime/memory.nqr Runtime/limits.nqr Programs/noqeri.cpp Programs/selftest.nqr Programs/diagnostics.nqr Programs/version.nqr Modules/builtin.nqr Modules/native.nqr Objects/value.nqr Objects/text.nqr Database/noqeridb.nqr Tools/security/policy.nqr Tools/fuzz/generator.nqr Tools/build/policy.nqr Tools/scripts/source_floor.nqr Lib/security/memory.nqr Lib/test/assert.nqr Lib/std/int.nqr Lib/std/slice_i64.nqr Lib/std/search.nqr Lib/std/sort.nqr Lib/std/stats.nqr Lib/std/bytes.nqr Lib/std/utf8.nqr Lib/std/checksum.nqr Lib/std/memory.nqr Lib/std/status.nqr Lib/std/time.nqr Lib/std/random.nqr Lib/std/matrix_i64.nqr Lib/std/range.nqr Lib/std/text.nqr Lib/std/set_i64.nqr Lib/std/map_i64.nqr Lib/std/stack_i64.nqr Lib/std/queue_i64.nqr Lib/std/algorithm.nqr Lib/std/bit.nqr tests/noqeri_owned_components.nqr tests/noqeri_library_suite.nqr editors/vscode/package.json site/index.html'
for path in $required; do
  if [ ! -e "$path" ]; then echo "missing required repository path: $path" >&2; exit 1; fi
done

for path in Modules/README.md Objects/README.md Lib/README.md Lib/test/README.md Android/README.md Mac/README.md PC/README.md PCbuild/README.md Platforms/README.md Parser/README.md Runtime/README.md Programs/README.md Benchmarks/README.md Compiler/README.md Tools/README.md Tools/build/README.md Tools/fuzz/README.md; do
  if [ -e "$path" ]; then echo "implementation placeholder must be executable .nqr, not $path" >&2; exit 1; fi
done

legacy=$(find . -path './.git' -prune -o -type f \( -name '*.noe' -o -name '*.ric' \) -print)
if [ -n "$legacy" ]; then
  echo "legacy source extensions found:" >&2
  echo "$legacy" >&2
  exit 1
fi

native_library_source=$(find Modules Objects Lib Database Tools/security Tools/fuzz Platforms PC Mac Android PCbuild -type f \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) -print 2>/dev/null || true)
if [ -n "$native_library_source" ]; then
  echo "C/C++ source found in Noqeri-owned implementation directories:" >&2
  echo "$native_library_source" >&2
  echo "move host/bootstrap adapters under Compiler/bootstrap and keep reusable implementation in .nqr" >&2
  exit 1
fi

noqeri_count=$(find . -path './.git' -prune -o -type f -name '*.nqr' -print | wc -l | tr -d ' ')
stdlib_count=$(find Lib/std -type f -name '*.nqr' -print | wc -l | tr -d ' ')
if [ "$noqeri_count" -lt 30 ]; then echo "Noqeri source floor failed: $noqeri_count < 30" >&2; exit 1; fi
if [ "$stdlib_count" -lt 16 ]; then echo "Noqeri stdlib floor failed: $stdlib_count < 16" >&2; exit 1; fi
printf '%s\n' "repository structure verified: Noqeri source=$noqeri_count stdlib=$stdlib_count; C++ confined away from Noqeri-owned library areas"
