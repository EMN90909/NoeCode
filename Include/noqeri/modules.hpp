#pragma once
#include "ast.hpp"
#include "diagnostics.hpp"
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace noe {
using ModuleId = std::uint32_t;
struct SourceFile { SourceId id=0; std::filesystem::path path; std::string text; };
struct ModuleUnit { ModuleId id=0; SourceId source=0; std::filesystem::path path; std::string declaredName; Program ast; std::vector<ModuleId> imports; };
class ModuleGraph {
public:
    void setBuildTarget(std::string target){ buildTarget_=std::move(target); }
    const std::string& buildTarget() const { return buildTarget_; }
    bool load(const std::filesystem::path& root,Diagnostics& diagnostics);
    bool loadWithOverlay(const std::filesystem::path& root,const std::filesystem::path& overlayPath,std::string overlayText,Diagnostics& diagnostics);
    const std::vector<ModuleUnit>& modules() const { return modules_; }
    const std::vector<SourceFile>& sources() const { return sources_; }
    Program mergedProgram() const;
private:
    bool loadOne(const std::filesystem::path& path,Diagnostics& diagnostics,ModuleId* out);
    std::vector<ModuleUnit> modules_;
    std::vector<SourceFile> sources_;
    std::unordered_map<std::string,ModuleId> byCanonicalPath_;
    std::unordered_map<std::string,std::string> overlays_;
    std::string buildTarget_;
};
} // namespace noe
