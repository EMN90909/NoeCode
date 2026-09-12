#pragma once
#include "compiler.hpp"
#include <filesystem>
#include <string>
namespace noe {
class WorkspaceCompiler {
public:
    CompileResult compileOverlay(const std::filesystem::path& rootFile,std::string source,const CompileOptions& options={}) const;
};
} // namespace noe
