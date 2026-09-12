#!/usr/bin/env sh
set -eu
required='README.md LICENSE LEGAL.md SECURITY.md CONTRIBUTING.md CMakeLists.txt project.nqr noqeri.lock Brand/noqeri-logo.webp Brand/noqeri-mark.png Grammar/noqeri.grammar.md Include/noqeri/noqeri.hpp Parser/lexer.cpp Parser/parser.cpp Compiler/core.cpp Runtime/interpreter.cpp Programs/noqeri.cpp Tools/build/production.cpp Modules/builtin.nqr Modules/native.nqr Objects/value.nqr Objects/text.nqr Database/noqeridb.nqr Tools/security/policy.nqr Lib/test/assert.nqr tests/noqeri_owned_components.nqr editors/vscode/package.json site/index.html'
for path in $required; do
  if [ ! -e "$path" ]; then echo "missing required repository path: $path" >&2; exit 1; fi
done
legacy=$(find . -path './.git' -prune -o -type f \( -name '*.noe' -o -name '*.ric' \) -print)
if [ -n "$legacy" ]; then
  echo "legacy source extensions found:" >&2
  echo "$legacy" >&2
  exit 1
fi
native_library_source=$(find Modules Objects Lib Database Tools/security -type f \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) -print 2>/dev/null || true)
if [ -n "$native_library_source" ]; then
  echo "C/C++ source found in Noqeri-owned library/runtime directories:" >&2
  echo "$native_library_source" >&2
  echo "move host/bootstrap adapters under Compiler/bootstrap and keep reusable implementation in .nqr" >&2
  exit 1
fi
printf '%s\n' 'repository structure verified: compiler bootstrap + Noqeri-owned runtime/library components'
