#include "noe.hpp"
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace noe {

int runProductionDoctor(const std::filesystem::path& root) {
    struct Check { std::filesystem::path path; std::string label; };
    const std::vector<Check> required = {
        {"README.md", "public README"},
        {"LICENSE", "MIT license"},
        {"LEGAL.md", "legal notice"},
        {"SECURITY.md", "security policy"},
        {"CONTRIBUTING.md", "contribution guide"},
        {"CHANGELOG.md", "change log"},
        {"project.noe", "Noe-native project manifest"},
        {"bootstrap/noe.hpp", "compiler API header"},
        {"bootstrap/main.cpp", "CLI entry point"},
        {"bootstrap/native_backend.cpp", "custom native backend"},
        {"bootstrap/linker.cpp", "linker driver"},
        {"scripts/build.sh", "build script"},
        {"scripts/test.sh", "test script"},
        {"docs/README.md", "documentation index"},
        {"docs/LANGUAGE_REFERENCE.md", "language reference"},
        {"docs/STATUS.md", "implementation status"},
        {"docs/PRODUCTION_READINESS.md", "production readiness notes"},
        {"examples/native_hello.noe", "native smoke example"},
        {"examples/native_hello.expected", "native smoke expected output"}
    };

    int failures = 0;
    std::cout << "Noe production-track doctor\n";
    std::cout << "compiler=" << NOE_COMPILER_VERSION << "\n";
    std::cout << "language=" << NOE_LANGUAGE_VERSION << "\n";
    std::cout << "target=" << NOE_SUPPORTED_TARGET << "\n";

    for (const auto& check : required) {
        auto full = root / check.path;
        if (std::filesystem::exists(full)) {
            std::cout << "ok   " << check.label << " (" << check.path.string() << ")\n";
        } else {
            ++failures;
            std::cerr << "fail " << check.label << " missing: " << full.string() << "\n";
        }
    }

    const std::vector<std::filesystem::path> forbidden = {
        root / "Cargo.toml",
        root / "package.json",
        root / "tsconfig.json"
    };
    for (const auto& path : forbidden) {
        if (std::filesystem::exists(path)) {
            ++failures;
            std::cerr << "fail unexpected non-Noe build manifest present: " << path.string() << "\n";
        }
    }

    if (failures == 0) {
        std::cout << "Noe production-track repository checks passed for the supported 1.0 subset.\n";
        return 0;
    }
    std::cerr << "Noe production-track repository checks failed: " << failures << " issue(s).\n";
    return 1;
}

} // namespace noe
