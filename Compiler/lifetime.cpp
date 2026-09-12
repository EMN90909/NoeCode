#include "noe.hpp"
#include <memory>
#include <string>
#include <unordered_set>

namespace noe {
namespace {

std::string calleeName(const ExprPtr& expression) {
    if(auto name=std::dynamic_pointer_cast<NameExpr>(expression))return name->name;
    if(auto member=std::dynamic_pointer_cast<MemberExpr>(expression))if(auto base=std::dynamic_pointer_cast<NameExpr>(member->object))return base->name+"."+member->member;
    return {};
}

bool referencesLocalArray(const ExprPtr& expression,const std::unordered_set<std::string>& arrays) {
    if(!expression)return false;
    if(auto name=std::dynamic_pointer_cast<NameExpr>(expression))return arrays.count(name->name)>0;
    if(auto index=std::dynamic_pointer_cast<IndexExpr>(expression))return referencesLocalArray(index->object,arrays);
    if(auto member=std::dynamic_pointer_cast<MemberExpr>(expression))return referencesLocalArray(member->object,arrays);
    if(auto unary=std::dynamic_pointer_cast<UnaryExpr>(expression))return referencesLocalArray(unary->operand,arrays);
    return false;
}

bool borrowedExpression(const ExprPtr& expression,const std::unordered_set<std::string>& arrays,const std::unordered_set<std::string>& borrowed) {
    if(!expression)return false;
    if(auto name=std::dynamic_pointer_cast<NameExpr>(expression))return borrowed.count(name->name)>0;
    if(auto call=std::dynamic_pointer_cast<CallExpr>(expression)) {
        if(calleeName(call->callee)=="slice"&&!call->args.empty())return referencesLocalArray(call->args.front(),arrays);
    }
    return false;
}

void inspectStatement(const StmtPtr& statement,bool returnsSlice,std::unordered_set<std::string>& arrays,std::unordered_set<std::string>& borrowed,Diagnostics& diagnostics) {
    if(!statement)return;
    if(auto let=std::dynamic_pointer_cast<LetStmt>(statement)) {
        const bool arrayAnnotation=let->annotation&&let->annotation->size()>3&&let->annotation->front()=='['&&let->annotation->rfind("[]",0)!=0;
        if(arrayAnnotation||std::dynamic_pointer_cast<ArrayExpr>(let->initializer))arrays.insert(let->name);
        if(borrowedExpression(let->initializer,arrays,borrowed))borrowed.insert(let->name);
        return;
    }
    if(auto expression=std::dynamic_pointer_cast<ExprStmt>(statement)) {
        if(auto assignment=std::dynamic_pointer_cast<BinaryExpr>(expression->expr);assignment&&assignment->op==TokenKind::Equal) {
            if(auto target=std::dynamic_pointer_cast<NameExpr>(assignment->left)) {
                if(borrowedExpression(assignment->right,arrays,borrowed))borrowed.insert(target->name);else borrowed.erase(target->name);
            }
        }
        return;
    }
    if(auto returned=std::dynamic_pointer_cast<ReturnStmt>(statement)) {
        if(returnsSlice&&borrowedExpression(returned->value,arrays,borrowed))diagnostics.error("NQR-T3055",returned->span,"cannot return a slice borrowed from function-local storage","return an owned [T; N] value, borrow caller-owned storage, or introduce an owned container instead");
        return;
    }
    if(auto block=std::dynamic_pointer_cast<BlockStmt>(statement)){for(const auto&child:block->statements)inspectStatement(child,returnsSlice,arrays,borrowed,diagnostics);return;}
    if(auto branch=std::dynamic_pointer_cast<IfStmt>(statement)){inspectStatement(branch->thenBranch,returnsSlice,arrays,borrowed,diagnostics);inspectStatement(branch->elseBranch,returnsSlice,arrays,borrowed,diagnostics);return;}
    if(auto loop=std::dynamic_pointer_cast<WhileStmt>(statement)){inspectStatement(loop->body,returnsSlice,arrays,borrowed,diagnostics);return;}
}

} // namespace

bool BorrowChecker::check(const Program& program,Diagnostics& diagnostics) const {
    for(const auto& statement:program.statements) {
        auto function=std::dynamic_pointer_cast<FunctionStmt>(statement);
        if(!function||function->isExtern||!function->body)continue;
        const bool returnsSlice=function->returnType&&function->returnType->rfind("[]",0)==0;
        if(!returnsSlice)continue;
        std::unordered_set<std::string> arrays;
        std::unordered_set<std::string> borrowed;
        for(const auto& child:function->body->statements)inspectStatement(child,true,arrays,borrowed,diagnostics);
    }
    return !diagnostics.hasErrors();
}

} // namespace noe
