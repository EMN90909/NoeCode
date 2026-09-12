#include "noe.hpp"
namespace noe {
CompileResult WorkspaceCompiler::compileOverlay(const std::filesystem::path&rootFile,std::string source,const CompileOptions&options)const{
    CompileResult result;result.modules=std::make_shared<ModuleGraph>();if(!result.modules->loadWithOverlay(rootFile,rootFile,std::move(source),result.diagnostics))return result;result.ast=result.modules->mergedProgram();TypeChecker checker(result.diagnostics);if(!checker.check(result.ast))return result;BorrowChecker borrow;if(!borrow.check(result.ast,result.diagnostics))return result;GenericEngine generics;if(!generics.monomorphize(result.ast,result.diagnostics))return result;Lowerer lowerer;result.nir=lowerer.lower(result.ast);Optimizer{}.optimize(result.nir,options.optimization);NirAnalyzer{}.analyze(result.ast,result.nir);return result;
}
} // namespace noe
