#include "noe.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <functional>
#include <sstream>
#include <system_error>
#include <unordered_set>

namespace noe {
namespace {
std::string trim(std::string s){auto notSpace=[](unsigned char c){return !std::isspace(c);};s.erase(s.begin(),std::find_if(s.begin(),s.end(),notSpace));s.erase(std::find_if(s.rbegin(),s.rend(),notSpace).base(),s.end());return s;}
bool targetAtomMatches(const std::string&atom,const std::string&target){
    if(atom=="all"||atom==target)return true;
    if(atom=="x86_64")return target.rfind("x86_64-",0)==0;
    if(atom=="aarch64"||atom=="arm64")return target.rfind("aarch64-",0)==0;
    if(atom=="wasm"||atom=="wasm32")return target.rfind("wasm32-",0)==0;
    if(atom=="windows")return target.find("windows")!=std::string::npos||target.find("msvc")!=std::string::npos;
    if(atom=="linux")return target.find("linux")!=std::string::npos||target.find("gnu")!=std::string::npos;
    if(atom=="macos"||atom=="darwin")return target.find("apple")!=std::string::npos||target.find("darwin")!=std::string::npos||target.find("macos")!=std::string::npos;
    if(atom=="freestanding")return target.size()>=5&&(target.rfind("-none")==target.size()-5||target.find("-unknown-none")!=std::string::npos);
    if(atom=="hosted")return !targetAtomMatches("freestanding",target);
    return false;
}
bool tagListMatches(std::string list,const std::string&target){std::replace(list.begin(),list.end(),',',' ');std::istringstream in(list);std::string atom;while(in>>atom)if(targetAtomMatches(atom,target))return true;return false;}
bool sourceEnabledForTarget(const std::string&text,const std::string&target,Diagnostics&d,SourceId sourceId){
    if(target.empty())return true;
    std::istringstream input(text);std::string line;std::size_t lineNo=0;bool includeSeen=false,includeMatch=false;
    while(lineNo<32&&std::getline(input,line)){
        ++lineNo;auto t=trim(line);if(t.empty())continue;
        if(t.rfind("//",0)!=0)break;
        constexpr const char* includePrefix="// noqeri:target ";
        constexpr const char* excludePrefix="// noqeri:not-target ";
        if(t.rfind(includePrefix,0)==0){includeSeen=true;if(tagListMatches(trim(t.substr(std::char_traits<char>::length(includePrefix))),target))includeMatch=true;continue;}
        if(t.rfind(excludePrefix,0)==0){if(tagListMatches(trim(t.substr(std::char_traits<char>::length(excludePrefix))),target))return false;continue;}
        if(t.rfind("// noqeri:",0)==0){d.error("NQR-M6010",{sourceId,lineNo,1},"unknown Noqeri build tag","use // noqeri:target <tag...> or // noqeri:not-target <tag...>");return false;}
    }
    return !includeSeen||includeMatch;
}
}

bool ModuleGraph::load(const std::filesystem::path&root,Diagnostics&diagnostics){modules_.clear();sources_.clear();byCanonicalPath_.clear();overlays_.clear();ModuleId rootId=0;return loadOne(root,diagnostics,&rootId)&&!diagnostics.hasErrors();}
bool ModuleGraph::loadWithOverlay(const std::filesystem::path&root,const std::filesystem::path&overlayPath,std::string overlayText,Diagnostics&diagnostics){modules_.clear();sources_.clear();byCanonicalPath_.clear();overlays_.clear();std::error_code ec;auto canonical=std::filesystem::weakly_canonical(overlayPath,ec);if(ec)canonical=std::filesystem::absolute(overlayPath,ec);overlays_[canonical.generic_string()]=std::move(overlayText);ModuleId rootId=0;return loadOne(root,diagnostics,&rootId)&&!diagnostics.hasErrors();}
bool ModuleGraph::loadOne(const std::filesystem::path&input,Diagnostics&diagnostics,ModuleId*out){std::error_code ec;auto canonical=std::filesystem::weakly_canonical(input,ec);if(ec){canonical=std::filesystem::absolute(input,ec);if(ec){diagnostics.error("NQR-M6000",{},"module source not found: "+input.string(),"check import paths relative to the importing file");return false;}}const auto key=canonical.generic_string();if(auto found=byCanonicalPath_.find(key);found!=byCanonicalPath_.end()){if(out)*out=found->second;return true;}const auto overlay=overlays_.find(key);if(overlay==overlays_.end()&&!std::filesystem::exists(canonical)){diagnostics.error("NQR-M6000",{},"module source not found: "+input.string(),"check import paths relative to the importing file");return false;}const ModuleId id=static_cast<ModuleId>(modules_.size());const SourceId sourceId=static_cast<SourceId>(sources_.size()+1);byCanonicalPath_[key]=id;std::string text;if(overlay!=overlays_.end())text=overlay->second;else try{text=readTextFile(canonical);}catch(const std::exception&error){diagnostics.error("NQR-M6001",{},error.what());return false;}diagnostics.registerSource(sourceId,canonical.string());sources_.push_back(SourceFile{sourceId,canonical,text});if(!sourceEnabledForTarget(text,buildTarget_,diagnostics,sourceId)){if(id==0){diagnostics.error("NQR-M6011",{sourceId,1,1},"entry module is excluded for target '"+buildTarget_+"'","choose a matching --target/project target or remove the entry module build tag");return false;}ModuleUnit skipped;skipped.id=id;skipped.source=sourceId;skipped.path=canonical;modules_.push_back(std::move(skipped));if(out)*out=id;return true;}auto syntax=preprocessGenericSyntax(text);Lexer lexer(syntax.source,diagnostics,sourceId,false);auto tokens=lexer.lex();if(diagnostics.hasErrors())return false;Parser parser(std::move(tokens),diagnostics);Program ast=parser.parse();applyGenericSyntax(ast,syntax);if(diagnostics.hasErrors())return false;ModuleUnit unit;unit.id=id;unit.source=sourceId;unit.path=canonical;unit.ast=ast;for(const auto&statement:ast.statements)if(auto module=std::dynamic_pointer_cast<ModuleStmt>(statement)){unit.declaredName=module->name;break;}modules_.push_back(std::move(unit));for(const auto&statement:ast.statements){auto imported=std::dynamic_pointer_cast<ImportStmt>(statement);if(!imported)continue;std::filesystem::path child=imported->path;if(child.extension().empty())child+=".nqr";if(child.is_relative())child=canonical.parent_path()/child;ModuleId childId=0;if(!loadOne(child,diagnostics,&childId))return false;modules_[id].imports.push_back(childId);}if(out)*out=id;return true;}
Program ModuleGraph::mergedProgram()const{Program merged;if(modules_.empty())return merged;std::unordered_set<ModuleId>visited;std::function<void(ModuleId)>append=[&](ModuleId id){if(visited.count(id)||id>=modules_.size())return;visited.insert(id);for(const auto dep:modules_[id].imports)append(dep);for(const auto&statement:modules_[id].ast.statements){if(std::dynamic_pointer_cast<ImportStmt>(statement))continue;merged.statements.push_back(statement);}};append(0);return merged;}
} // namespace noe
