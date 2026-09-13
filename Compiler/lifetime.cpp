#include "noe.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace noe {
namespace {

std::string calleeName(const ExprPtr& expression){if(auto name=std::dynamic_pointer_cast<NameExpr>(expression))return name->name;if(auto member=std::dynamic_pointer_cast<MemberExpr>(expression))if(auto base=std::dynamic_pointer_cast<NameExpr>(member->object))return base->name+"."+member->member;return{};}

std::optional<std::int64_t> literalInteger(const ExprPtr&expression){
    if(auto literal=std::dynamic_pointer_cast<LiteralExpr>(expression))if(auto value=std::get_if<std::int64_t>(&literal->value))return*value;
    if(auto unary=std::dynamic_pointer_cast<UnaryExpr>(expression)){auto value=literalInteger(unary->operand);if(!value)return std::nullopt;if(unary->op==TokenKind::Plus)return*value;if(unary->op==TokenKind::Minus)return-*value;}
    return std::nullopt;
}

bool nullExpression(const ExprPtr&expression){
    if(auto literal=std::dynamic_pointer_cast<LiteralExpr>(expression))return std::holds_alternative<std::monostate>(literal->value);
    if(auto cast=std::dynamic_pointer_cast<CastExpr>(expression))return nullExpression(cast->value);
    return false;
}

std::optional<std::size_t> fixedArrayCount(const std::optional<std::string>&annotation){
    if(!annotation||annotation->size()<4||annotation->front()!='['||annotation->rfind("[]",0)==0)return std::nullopt;
    auto semi=annotation->rfind(';');auto close=annotation->rfind(']');if(semi==std::string::npos||close==std::string::npos||semi>=close)return std::nullopt;
    try{return static_cast<std::size_t>(std::stoull(annotation->substr(semi+1,close-semi-1),nullptr,0));}catch(...){return std::nullopt;}
}

struct BorrowInfo{int sourceDepth=0;std::string source;};

class LifetimeInspector {
public:
    LifetimeInspector(Diagnostics&diagnostics,bool returnsBorrowable):diagnostics_(diagnostics),returnsBorrowable_(returnsBorrowable){}

    void parameter(const Parameter&parameter){declaredDepth_[parameter.name]=0;if(auto count=fixedArrayCount(parameter.annotation))arraySizes_[parameter.name]=*count;}
    void run(const std::shared_ptr<BlockStmt>&body){if(body)for(const auto&statement:body->statements)statement(statement,0);}

private:
    std::optional<BorrowInfo> borrowOf(const ExprPtr&expression){
        if(!expression)return std::nullopt;
        if(auto name=std::dynamic_pointer_cast<NameExpr>(expression)){auto it=borrowed_.find(name->name);if(it!=borrowed_.end())return it->second;return std::nullopt;}
        if(auto unary=std::dynamic_pointer_cast<UnaryExpr>(expression);unary&&unary->op==TokenKind::Ampersand){if(auto source=localSource(unary->operand))return source;}
        if(auto call=std::dynamic_pointer_cast<CallExpr>(expression);call&&calleeName(call->callee)=="slice"&&!call->args.empty()){if(auto source=localSource(call->args.front()))return source;}
        if(auto cast=std::dynamic_pointer_cast<CastExpr>(expression))return borrowOf(cast->value);
        return std::nullopt;
    }

    std::optional<BorrowInfo> localSource(const ExprPtr&expression){
        if(!expression)return std::nullopt;
        if(auto name=std::dynamic_pointer_cast<NameExpr>(expression)){auto it=declaredDepth_.find(name->name);if(it!=declaredDepth_.end())return BorrowInfo{it->second,name->name};return std::nullopt;}
        if(auto index=std::dynamic_pointer_cast<IndexExpr>(expression))return localSource(index->object);
        if(auto member=std::dynamic_pointer_cast<MemberExpr>(expression))return localSource(member->object);
        if(auto unary=std::dynamic_pointer_cast<UnaryExpr>(expression))return localSource(unary->operand);
        return std::nullopt;
    }

    void checkIndex(const std::shared_ptr<IndexExpr>&index){
        auto literal=literalInteger(index->index);if(!literal)return;
        std::optional<std::size_t> count;
        if(auto name=std::dynamic_pointer_cast<NameExpr>(index->object)){auto it=arraySizes_.find(name->name);if(it!=arraySizes_.end())count=it->second;}
        else if(auto array=std::dynamic_pointer_cast<ArrayExpr>(index->object))count=array->elements.size();
        if(count&&(*literal<0||static_cast<std::uint64_t>(*literal)>=static_cast<std::uint64_t>(*count)))diagnostics_.error("NQR-L4104",index->span,"fixed-array index is out of bounds","ordinary fixed-array accesses are checked statically when the index is known");
    }

    void expression(const ExprPtr&expr){
        if(!expr)return;
        if(auto name=std::dynamic_pointer_cast<NameExpr>(expr)){if(dangling_.count(name->name))diagnostics_.error("NQR-L4102",name->span,"use of reference whose borrowed storage has left scope","rebind the reference to caller-owned or longer-lived storage before using it");return;}
        if(auto unary=std::dynamic_pointer_cast<UnaryExpr>(expr)){if(unary->op==TokenKind::Star&&nullExpression(unary->operand))diagnostics_.error("NQR-L4103",unary->span,"provable null pointer dereference","guard nullable pointers before dereferencing");expression(unary->operand);return;}
        if(auto index=std::dynamic_pointer_cast<IndexExpr>(expr)){if(nullExpression(index->object))diagnostics_.error("NQR-L4103",index->span,"provable null pointer indexing","guard nullable pointers before indexing");checkIndex(index);expression(index->object);expression(index->index);return;}
        if(auto member=std::dynamic_pointer_cast<MemberExpr>(expr)){expression(member->object);return;}
        if(auto cast=std::dynamic_pointer_cast<CastExpr>(expr)){expression(cast->value);return;}
        if(auto call=std::dynamic_pointer_cast<CallExpr>(expr)){expression(call->callee);for(const auto&arg:call->args)expression(arg);return;}
        if(auto binary=std::dynamic_pointer_cast<BinaryExpr>(expr)){expression(binary->left);expression(binary->right);return;}
        if(auto array=std::dynamic_pointer_cast<ArrayExpr>(expr)){for(const auto&item:array->elements)expression(item);return;}
    }

    void assignBorrow(const std::string&name,const ExprPtr&value){
        dangling_.erase(name);auto borrowed=borrowOf(value);if(borrowed)borrowed_[name]=*borrowed;else borrowed_.erase(name);
    }

    void expireScope(int depth){
        std::vector<std::string> locals;for(const auto&[name,declared]:declaredDepth_)if(declared==depth)locals.push_back(name);
        for(const auto&[name,borrow]:borrowed_){auto declared=declaredDepth_.find(name);if(borrow.sourceDepth>=depth&&declared!=declaredDepth_.end()&&declared->second<depth)dangling_.insert(name);}
        for(const auto&name:locals){declaredDepth_.erase(name);arraySizes_.erase(name);borrowed_.erase(name);dangling_.erase(name);}
    }

    void statement(const StmtPtr&stmt,int depth){
        if(!stmt)return;
        if(auto let=std::dynamic_pointer_cast<LetStmt>(stmt)){
            expression(let->initializer);declaredDepth_[let->name]=depth;if(auto count=fixedArrayCount(let->annotation))arraySizes_[let->name]=*count;else if(auto array=std::dynamic_pointer_cast<ArrayExpr>(let->initializer))arraySizes_[let->name]=array->elements.size();assignBorrow(let->name,let->initializer);return;
        }
        if(auto expr=std::dynamic_pointer_cast<ExprStmt>(stmt)){
            if(auto assignment=std::dynamic_pointer_cast<BinaryExpr>(expr->expr);assignment&&assignment->op==TokenKind::Equal){expression(assignment->right);if(auto target=std::dynamic_pointer_cast<NameExpr>(assignment->left)){assignBorrow(target->name,assignment->right);return;}}
            expression(expr->expr);return;
        }
        if(auto returned=std::dynamic_pointer_cast<ReturnStmt>(stmt)){
            expression(returned->value);if(returnsBorrowable_){if(auto borrowed=borrowOf(returned->value))diagnostics_.error("NQR-L4101",returned->span,"cannot return a reference borrowed from function-local storage ('"+borrowed->source+"')","return owned data or borrow caller-owned storage instead");}return;
        }
        if(auto thrown=std::dynamic_pointer_cast<ThrowStmt>(stmt)){expression(thrown->value);return;}
        if(auto block=std::dynamic_pointer_cast<BlockStmt>(stmt)){for(const auto&child:block->statements)statement(child,depth+1);expireScope(depth+1);return;}
        if(auto branch=std::dynamic_pointer_cast<IfStmt>(stmt)){expression(branch->condition);statement(branch->thenBranch,depth+1);expireScope(depth+1);statement(branch->elseBranch,depth+1);expireScope(depth+1);return;}
        if(auto loop=std::dynamic_pointer_cast<WhileStmt>(stmt)){expression(loop->condition);statement(loop->body,depth+1);expireScope(depth+1);return;}
    }

    Diagnostics&diagnostics_;
    bool returnsBorrowable_=false;
    std::unordered_map<std::string,int> declaredDepth_;
    std::unordered_map<std::string,std::size_t> arraySizes_;
    std::unordered_map<std::string,BorrowInfo> borrowed_;
    std::unordered_set<std::string> dangling_;
};

} // namespace

bool BorrowChecker::check(const Program&program,Diagnostics&diagnostics)const{
    for(const auto&statement:program.statements){
        auto function=std::dynamic_pointer_cast<FunctionStmt>(statement);if(!function||function->isExtern||!function->body)continue;
        const bool returnsBorrowable=function->returnType&&(function->returnType->rfind("[]",0)==0||function->returnType->rfind("*",0)==0);
        LifetimeInspector inspector(diagnostics,returnsBorrowable);for(const auto&parameter:function->params)inspector.parameter(parameter);inspector.run(function->body);
    }
    return!diagnostics.hasErrors();
}

} // namespace noe
