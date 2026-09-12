#pragma once
#include "ast.hpp"
#include "diagnostics.hpp"
#include <filesystem>
#include <string>

namespace noe {
class WebBackend {
public:
    std::string emitModule(const Program& program,Diagnostics& diagnostics) const;
    bool emitModule(const Program& program,const std::filesystem::path& output,Diagnostics& diagnostics) const;
};
} // namespace noe
