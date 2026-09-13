#pragma once
#include "ast.hpp"
#include "diagnostics.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace noe {
using GenericConstraintMap=std::unordered_map<std::string,std::unordered_map<std::string,std::vector<std::string>>>;
using GenericRecordParamMap=std::unordered_map<std::string,std::vector<std::string>>;
struct GenericSyntaxInfo {
    std::string source;
    GenericConstraintMap constraints;
    GenericRecordParamMap recordParams;
};
GenericSyntaxInfo preprocessGenericSyntax(const std::string& source);
void applyGenericSyntax(Program& program,const GenericSyntaxInfo& syntax);
class GenericEngine { public: bool monomorphize(Program& program,Diagnostics& diagnostics) const; };
} // namespace noe
