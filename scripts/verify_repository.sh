#!/usr/bin/env sh
set -eu

required='README.md LICENSE LEGAL.md SECURITY.md CONTRIBUTING.md CMakeLists.txt project.nqr noqeri.lock bootstrap.ps1 bootstrap.sh Brand/noqeri-logo.webp Brand/noqeri-mark.png Brand/noqeri-banner.txt Grammar/noqeri.grammar.md Include/noqeri/noqeri.hpp Compiler/core.cpp Compiler/semantic_rules.nqr Compiler/bootstrap/lexer_host.cpp Compiler/bootstrap/parser_host.cpp Compiler/bootstrap/abi_runtime_host.cpp Compiler/bootstrap/interpreter_host.cpp Compiler/bootstrap/cli_host.cpp Compiler/bootstrap/formatter_host.cpp Compiler/bootstrap/lsp_host.cpp Compiler/bootstrap/package_host.cpp Compiler/bootstrap/test_runner_host.cpp Compiler/bootstrap/production_doctor_host.cpp Compiler/bootstrap/noqeridb_host.cpp Compiler/bootstrap/security_audit_host.cpp Parser/ascii.nqr Parser/token_rules.nqr Runtime/status.nqr Runtime/memory.nqr Runtime/limits.nqr Programs/selftest.nqr Programs/diagnostics.nqr Programs/version.nqr Modules/builtin.nqr Modules/native.nqr Objects/value.nqr Objects/text.nqr Database/noqeridb.nqr Tools/security/policy.nqr Tools/fuzz/generator.nqr Tools/build/policy.nqr Tools/scripts/source_floor.nqr Lib/security/memory.nqr Lib/test/assert.nqr Lib/std/int.nqr Lib/std/slice_i64.nqr Lib/std/search.nqr Lib/std/sort.nqr Lib/std/stats.nqr Lib/std/bytes.nqr Lib/std/utf8.nqr Lib/std/checksum.nqr Lib/std/memory.nqr Lib/std/status.nqr Lib/std/time.nqr Lib/std/random.nqr Lib/std/matrix_i64.nqr Lib/std/range.nqr Lib/std/text.nqr Lib/std/set_i64.nqr Lib/std/map_i64.nqr Lib/std/stack_i64.nqr Lib/std/queue_i64.nqr Lib/std/algorithm.nqr Lib/std/bit.nqr Lib/std/validation.nqr Lib/std/window.nqr Lib/std/pair_i64.nqr Lib/std/counter.nqr Lib/std/compare.nqr tests/noqeri_owned_components.nqr tests/noqeri_library_suite.nqr examples/ecosystem_smoke.nqr editors/vscode/package.json site/index.html'
for path in $required; do
  if [ ! -e "$path" ]; then echo "missing required repository path: $path" >&2; exit 1; fi
done

for path in Modules/README.md Objects/README.md Lib/README.md Lib/test/README.md Android/README.md Mac/README.md PC/README.md PCbuild/README.md Platforms/README.md Parser/README.md Runtime/README.md Programs/README.md Benchmarks/README.md Compiler/README.md Tools/README.md Tools/build/README.md Tools/fuzz/README.md Parser/lexer.cpp Parser/parser.cpp Runtime/abi.cpp Runtime/interpreter.cpp Programs/noqeri.cpp Programs/formatter.cpp Programs/lsp.cpp Programs/package.cpp Programs/test_runner.cpp Tools/build/production.cpp; do
  if [ -e "$path" ]; then echo "obsolete placeholder/bootstrap-leak path present: $path" >&2; exit 1; fi
done

legacy=$(find . -path './.git' -prune -o -type f \( -name '*.noe' -o -name '*.ric' \) -print)
if [ -n "$legacy" ]; then
  echo "legacy source extensions found:" >&2
  echo "$legacy" >&2
  exit 1
fi

native_library_source=$(find Modules Objects Lib Database Parser Runtime Programs Tools/security Tools/fuzz Tools/build Platforms PC Mac Android PCbuild -type f \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) -print 2>/dev/null || true)
if [ -n "$native_library_source" ]; then
  echo "C/C++ source found in Noqeri-owned implementation directories:" >&2
  echo "$native_library_source" >&2
  echo "move bootstrap/host C++ under Compiler/bootstrap and keep product-facing implementation in .nqr" >&2
  exit 1
fi

noqeri_count=$(find . -path './.git' -prune -o -type f -name '*.nqr' -print | wc -l | tr -d ' ')
stdlib_count=$(find Lib/std -type f -name '*.nqr' -print | wc -l | tr -d ' ')
if [ "$noqeri_count" -lt 30 ]; then echo "Noqeri source floor failed: $noqeri_count < 30" >&2; exit 1; fi
if [ "$stdlib_count" -lt 16 ]; then echo "Noqeri stdlib floor failed: $stdlib_count < 16" >&2; exit 1; fi
printf '%s\n' "repository structure verified: Noqeri source=$noqeri_count stdlib=$stdlib_count; non-compiler C++ excluded from Noqeri-owned areas"
