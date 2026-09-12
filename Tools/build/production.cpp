#include "noe.hpp"
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace noe {

int runProductionDoctor(const std::filesystem::path&root){
    struct Check{std::filesystem::path path;std::string label;};
    const std::vector<Check>required={
        {"README.md","public README"},{"LICENSE","GPL-3.0-only license"},{"LEGAL.md","legal notice"},{"SECURITY.md","security policy"},{"CONTRIBUTING.md","contribution guide"},{"CHANGELOG.md","change log"},
        {"project.nqr","noqeri project manifest"},{"noqeri.lock","noqeri lock file"},{"Brand/noqeri-logo.webp","canonical noqeri logo"},{"Brand/noqeri-mark.png","IDE/file mark"},
        {"Include/noqeri/noqeri.hpp","public umbrella header"},{"Include/noqeri/version.hpp","version API"},{"Include/noqeri/diagnostics.hpp","diagnostics API"},{"Include/noqeri/types.hpp","types API"},{"Include/noqeri/ast.hpp","AST API"},{"Include/noqeri/nir.hpp","NIR API"},{"Include/noqeri/modules.hpp","modules API"},{"Include/noqeri/packages.hpp","packages API"},{"Include/noqeri/targets.hpp","targets API"},{"Include/noqeri/runtime.hpp","runtime API"},{"Include/noqeri/compiler.hpp","compiler API"},{"Include/noqeri/tools.hpp","tooling API"},{"Include/noqeri/abi.h","public ABI v2 header"},
        {"Doc/ABI.md","ABI contract documentation"},{"Doc/FREESTANDING.md","freestanding target documentation"},{"Doc/LANGUAGE_REFERENCE.md","language reference"},{"Grammar/noqeri.grammar.md","grammar contract"},{"InternalDocs/ARCHITECTURE.md","repository architecture"},
        {"Compiler/core.cpp","compiler core"},{"Compiler/modules.cpp","module graph"},{"Compiler/type_checker.cpp","type checker"},{"Compiler/nir.cpp","NIR implementation"},{"Compiler/optimizer.cpp","optimizer"},{"Compiler/native_backend.cpp","native backend"},{"Compiler/linker.cpp","tool process/target adapter"},
        {"Parser/lexer.cpp","lexer implementation"},{"Parser/parser.cpp","parser implementation"},{"Runtime/abi.cpp","ABI runtime bridge"},{"Runtime/interpreter.cpp","reference interpreter"},{"Programs/noqeri.cpp","CLI entry point"},{"Programs/lsp.cpp","language server"},{"Programs/formatter.cpp","formatter"},{"Programs/package.cpp","project tooling"},{"Programs/test_runner.cpp","test runner"},
        {"Modules/native.nqr","Noqeri native-module contract"},{"Objects/value.nqr","Noqeri value model"},{"Objects/text.nqr","Noqeri text/byte model"},{"Database/noqeridb.nqr","Noqeri database core"},{"Tools/security/policy.nqr","Noqeri security policy helpers"},{"Lib/test/assert.nqr","Noqeri test assertions"},
        {"Platforms/README.md","target matrix"},{"Benchmarks/README.md","benchmark policy"},{"Tools/fuzz/README.md","fuzzing policy"},{"editors/vscode/package.json","VS Code language package"},
        {"scripts/build.sh","POSIX build script"},{"scripts/build.ps1","Windows build script"},{"scripts/verify_repository.sh","repository verifier"},{"examples/hello.nqr","smoke example"},{"examples/general_systems.nqr","general systems example"},
        {"tests/arithmetic.nqr","test program"},{"tests/abi_services.nqr","ABI service test"},{"tests/systems_types.nqr","systems types test"},{"tests/general_capabilities.nqr","general capabilities test"},{"tests/module_math.nqr","module fixture"},{"tests/modules_imports.nqr","import graph test"},{"tests/noqeri_owned_components.nqr","Noqeri-owned component integration test"}
    };
    int failures=0;
    std::cout<<"noqeri production-track doctor\ncompiler="<<NOQERI_COMPILER_VERSION<<"\nlanguage="<<NOQERI_LANGUAGE_VERSION<<"\nedition="<<NOQERI_EDITION<<"\nabi="<<NOQERI_ABI_VERSION<<"\ndefault-target="<<NOQERI_DEFAULT_TARGET<<"\ntargets="<<NOQERI_SUPPORTED_TARGETS<<"\n";
    for(const auto&check:required){auto full=root/check.path;if(std::filesystem::exists(full))std::cout<<"ok   "<<check.label<<" ("<<check.path.string()<<")\n";else{++failures;std::cerr<<"fail "<<check.label<<" missing: "<<full.string()<<"\n";}}
    const std::vector<std::filesystem::path>forbidden={root/"project.noe",root/"noe.lock",root/"project.ric",root/"ric.lock",root/"Include/noe",root/"Include/ric",root/"Grammar/noe.grammar.md",root/"Grammar/ric.grammar.md"};
    for(const auto&path:forbidden)if(std::filesystem::exists(path)){++failures;std::cerr<<"fail legacy public language path present: "<<path.string()<<"\n";}
    if(std::filesystem::exists(root))for(const auto&e:std::filesystem::recursive_directory_iterator(root)){if(!e.is_regular_file())continue;auto ext=e.path().extension().string();if(ext==".noe"||ext==".ric"){++failures;std::cerr<<"fail legacy source extension present: "<<e.path().string()<<"\n";}}
    if(failures==0){std::cout<<"noqeri production-track repository checks passed for language "<<NOQERI_LANGUAGE_VERSION<<" and ABI v"<<NOQERI_ABI_VERSION<<".\n";return 0;}
    std::cerr<<"noqeri production-track repository checks failed: "<<failures<<" issue(s).\n";return 1;
}

} // namespace noe
