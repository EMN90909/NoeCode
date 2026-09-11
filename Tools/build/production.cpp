#include "noe.hpp"
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace noe {
int runProductionDoctor(const std::filesystem::path& root) {
    struct Check { std::filesystem::path path; std::string label; };
    const std::vector<Check> required = {
        {"README.md","public README"},{"LICENSE","MIT license"},{"LEGAL.md","legal notice"},
        {"SECURITY.md","security policy"},{"CONTRIBUTING.md","contribution guide"},{"CHANGELOG.md","change log"},
        {"project.nqr","noqeri project manifest"},{"noqeri.lock","noqeri lock file"},
        {"Brand/noqeri-logo.webp","canonical noqeri logo"},{"Brand/noqeri-mark.png","IDE/file mark"},
        {"Doc/_static/noqeri-logo.webp","documentation logo"},{"Include/noqeri/noqeri.hpp","public bootstrap header"},
        {"Include/noqeri/abi.h","public ABI v1 header"},{"Doc/ABI.md","ABI contract documentation"},
        {"Grammar/noqeri.grammar.md","grammar contract"},{"InternalDocs/CPYTHON_STRUCTURE.md","repository architecture study"},
        {"Compiler/core.cpp","compiler core"},{"Compiler/type_checker.cpp","type checker"},{"Compiler/nir.cpp","NIR implementation"},
        {"Compiler/optimizer.cpp","optimizer"},{"Compiler/native_backend.cpp","native backend"},{"Compiler/linker.cpp","linker driver"},
        {"Parser/lexer.cpp","lexer implementation"},{"Parser/parser.cpp","parser implementation"},
        {"Runtime/abi.cpp","ABI runtime bridge"},{"Runtime/interpreter.cpp","reference interpreter"},{"Programs/noqeri.cpp","CLI entry point"},
        {"Programs/lsp.cpp","language server"},{"Programs/formatter.cpp","formatter"},{"Programs/package.cpp","project tooling"},
        {"Programs/test_runner.cpp","test runner"},{"Lib/std/core.nqr","standard library core"},
        {"Objects/README.md","object model"},{"Modules/README.md","module boundary"},{"Platforms/README.md","platform matrix"},
        {"PCbuild/README.md","Windows build notes"},{"Mac/README.md","macOS build notes"},{"Android/README.md","Android target notes"},
        {"Benchmarks/README.md","benchmark policy"},{"Tools/fuzz/README.md","fuzzing policy"},
        {"editors/vscode/package.json","VS Code language package"},{"site/index.html","public site"},
        {"scripts/build.sh","POSIX build script"},{"scripts/build.ps1","Windows build script"},
        {"scripts/verify_repository.sh","repository verifier"},{"examples/hello.nqr","smoke example"},{"tests/arithmetic.nqr","test program"},
        {"tests/abi_services.nqr","ABI service test"}
    };
    int failures = 0;
    std::cout << "noqeri production-track doctor\ncompiler=" << NOQERI_COMPILER_VERSION
              << "\nlanguage=" << NOQERI_LANGUAGE_VERSION << "\nabi=" << NOQERI_ABI_VERSION
              << "\ntarget=" << NOQERI_SUPPORTED_TARGET << "\n";
    for (const auto& check : required) {
        auto full = root / check.path;
        if (std::filesystem::exists(full)) std::cout << "ok   " << check.label << " (" << check.path.string() << ")\n";
        else { ++failures; std::cerr << "fail " << check.label << " missing: " << full.string() << "\n"; }
    }
    const std::vector<std::filesystem::path> forbidden = {
        root/"project.noe", root/"noe.lock", root/"project.ric", root/"ric.lock", root/"Include/noe", root/"Include/ric",
        root/"Grammar/noe.grammar.md", root/"Grammar/ric.grammar.md"
    };
    for (const auto& path : forbidden) if (std::filesystem::exists(path)) {
        ++failures; std::cerr << "fail legacy public language path present: " << path.string() << "\n";
    }
    if (std::filesystem::exists(root)) {
        for (const auto& e : std::filesystem::recursive_directory_iterator(root)) {
            if (!e.is_regular_file()) continue;
            auto ext = e.path().extension().string();
            if (ext == ".noe" || ext == ".ric") {
                ++failures; std::cerr << "fail legacy source extension present: " << e.path().string() << "\n";
            }
        }
    }
    if (failures == 0) {
        std::cout << "noqeri production-track repository checks passed for language 1.0 and ABI v1.\n";
        return 0;
    }
    std::cerr << "noqeri production-track repository checks failed: " << failures << " issue(s).\n";
    return 1;
}
} // namespace noe
