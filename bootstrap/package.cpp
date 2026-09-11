#include "noe.hpp"
#include <fstream>
#include <regex>
#include <sstream>

namespace noe {
namespace {
std::optional<std::string> field(const std::string& text,const std::string& key){
    std::regex re("\\b"+key+"\\s*:\\s*\\\"([^\\\"]*)\\\"");
    std::smatch m;
    if(std::regex_search(text,m,re)) return m[1].str();
    return std::nullopt;
}
}

std::optional<ProjectManifest> PackageManager::loadManifest(const std::filesystem::path& path, Diagnostics& diagnostics) const {
    std::ifstream in(path);
    if(!in){ diagnostics.error("NOE-PKG6000",{},"project manifest not found: "+path.string()); return std::nullopt; }
    std::ostringstream out; out<<in.rdbuf(); auto text=out.str();
    if(text.find("project") == std::string::npos || text.find('{') == std::string::npos){ diagnostics.error("NOE-PKG6001",{},"project.noe must contain a project { ... } block"); return std::nullopt; }
    ProjectManifest m;
    if(auto v=field(text,"name"))m.name=*v;
    if(auto v=field(text,"version"))m.version=*v;
    if(auto v=field(text,"entry"))m.entry=*v;
    return m;
}
} // namespace noe
