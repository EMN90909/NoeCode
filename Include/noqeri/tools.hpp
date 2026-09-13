#pragma once
#include "diagnostics.hpp"
#include "runtime_checks.hpp"
#include <filesystem>
#include <string>

namespace noe {
class Formatter { public: std::string format(const std::string& source,Diagnostics& diagnostics) const; };
class TestRunner { public: int runDirectory(const std::filesystem::path& dir,RuntimeCheckConfig checks={}) const; };
class LanguageServer { public: int run(); };
int runProductionDoctor(const std::filesystem::path& root);
int runSecurityAudit(const std::filesystem::path& root);
} // namespace noe
