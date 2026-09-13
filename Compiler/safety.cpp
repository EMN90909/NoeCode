#include "noe.hpp"
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace noe {
namespace {

std::optional<std::size_t> fixedArrayCount(const std::string&type){
    if(type.size()<4||type.front()!='['||type.rfind("[]",0)==0)return std::nullopt;
    auto semi=type.rfind(';'),close=type.rfind(']');if(semi==std::string::npos||close==std::string::npos||semi>=close)return std::nullopt;
    try{return static_cast<std::size_t>(std::stoull(type.substr(semi+1,close-semi-1),nullptr,0));}catch(...){return std::nullopt;}
}

std::string expressionCalleeName(const ExprPtr&e){
    if(auto name=std::dynamic_pointer_cast<NameExpr>(e))return name->name;
    if(auto member=std::dynamic_pointer_cast<MemberExpr>(e)){
        if(auto base=std::dynamic_pointer_cast<NameExpr>(member->object))return base->name+"."+member->member;
    }
    return {};
}

class Annotator {
public:
    void function(const std::shared_ptr<FunctionStmt>&fn){
        bounds_.clear();for(const auto&p:fn->params)if(p.annotation)if(auto count=fixedArrayCount(*p.annotation))bounds_[p.name]=*count;
        if(fn->body)statement(fn->body);
    }

private:
    void expression(const ExprPtr&e){
        if(!e)return;
        if(auto index=std::dynamic_pointer_cast<IndexExpr>(e)){
            expression(index->object);expression(index->index);
            if(auto name=std::dynamic_pointer_cast<NameExpr>(index->object)){auto it=bounds_.find(name->name);if(it!=bounds_.end())index->fixedBound=it->second;}
            else if(auto array=std::dynamic_pointer_cast<ArrayExpr>(index->object))index->fixedBound=array->elements.size();
            return;
        }
        if(auto unary=std::dynamic_pointer_cast<UnaryExpr>(e)){expression(unary->operand);return;}
        if(auto binary=std::dynamic_pointer_cast<BinaryExpr>(e)){expression(binary->left);expression(binary->right);return;}
        if(auto call=std::dynamic_pointer_cast<CallExpr>(e)){expression(call->callee);for(const auto&a:call->args)expression(a);return;}
        if(auto cast=std::dynamic_pointer_cast<CastExpr>(e)){expression(cast->value);return;}
        if(auto member=std::dynamic_pointer_cast<MemberExpr>(e)){expression(member->object);return;}
        if(auto array=std::dynamic_pointer_cast<ArrayExpr>(e)){for(const auto&item:array->elements)expression(item);return;}
    }

    void statement(const StmtPtr&s){
        if(!s)return;
        if(auto let=std::dynamic_pointer_cast<LetStmt>(s)){
            expression(let->initializer);
            if(let->annotation){if(auto count=fixedArrayCount(*let->annotation))bounds_[let->name]=*count;else bounds_.erase(let->name);}
            else if(auto array=std::dynamic_pointer_cast<ArrayExpr>(let->initializer))bounds_[let->name]=array->elements.size();
            else bounds_.erase(let->name);
            return;
        }
        if(auto expr=std::dynamic_pointer_cast<ExprStmt>(s)){expression(expr->expr);return;}
        if(auto ret=std::dynamic_pointer_cast<ReturnStmt>(s)){expression(ret->value);return;}
        if(auto thrown=std::dynamic_pointer_cast<ThrowStmt>(s)){expression(thrown->value);return;}
        if(auto block=std::dynamic_pointer_cast<BlockStmt>(s)){auto saved=bounds_;for(const auto&child:block->statements)statement(child);bounds_=std::move(saved);return;}
        if(auto branch=std::dynamic_pointer_cast<IfStmt>(s)){expression(branch->condition);auto saved=bounds_;statement(branch->thenBranch);bounds_=saved;statement(branch->elseBranch);bounds_=std::move(saved);return;}
        if(auto loop=std::dynamic_pointer_cast<WhileStmt>(s)){expression(loop->condition);auto saved=bounds_;statement(loop->body);bounds_=std::move(saved);return;}
    }

    std::unordered_map<std::string,std::size_t> bounds_;
};

class UnsafeAudit {
public:
    UnsafeAudit(const Program&program,Diagnostics&diagnostics):diagnostics_(diagnostics){
        for(const auto&stmt:program.statements){
            if(auto fn=std::dynamic_pointer_cast<FunctionStmt>(stmt);fn&&fn->isExtern)externFunctions_.insert(fn->name);
        }
    }

    bool run(const Program&program){
        for(const auto&stmt:program.statements)statement(stmt,false);
        return !diagnostics_.hasErrors();
    }

private:
    void require(bool unsafe,Span span,const std::string&operation){
        if(unsafe)return;
        diagnostics_.error("NQR-S3200",span,operation+" requires an unsafe block","wrap the operation in `unsafe { ... }` and keep the unsafe region as small as practical");
    }

    bool pointerTarget(const std::string&typeName)const{return !typeName.empty()&&typeName.front()=='*';}

    void expression(const ExprPtr&e,bool unsafe){
        if(!e)return;
        if(auto unary=std::dynamic_pointer_cast<UnaryExpr>(e)){
            expression(unary->operand,unsafe);
            if(unary->op==TokenKind::Star)require(unsafe,unary->span,unary->volatileAccess?"volatile raw pointer dereference":"raw pointer dereference");
            return;
        }
        if(auto binary=std::dynamic_pointer_cast<BinaryExpr>(e)){expression(binary->left,unsafe);expression(binary->right,unsafe);return;}
        if(auto cast=std::dynamic_pointer_cast<CastExpr>(e)){
            expression(cast->value,unsafe);
            if(pointerTarget(cast->typeName))require(unsafe,cast->span,"cast to a raw pointer");
            return;
        }
        if(auto index=std::dynamic_pointer_cast<IndexExpr>(e)){
            expression(index->object,unsafe);expression(index->index,unsafe);
            if(index->volatileAccess)require(unsafe,index->span,"volatile memory indexing");
            return;
        }
        if(auto member=std::dynamic_pointer_cast<MemberExpr>(e)){
            expression(member->object,unsafe);
            if(member->baseIsPointer||member->volatileAccess)require(unsafe,member->span,member->volatileAccess?"volatile pointer field access":"raw pointer field access");
            return;
        }
        if(auto array=std::dynamic_pointer_cast<ArrayExpr>(e)){for(const auto&item:array->elements)expression(item,unsafe);return;}
        if(auto call=std::dynamic_pointer_cast<CallExpr>(e)){
            expression(call->callee,unsafe);for(const auto&arg:call->args)expression(arg,unsafe);
            const auto name=expressionCalleeName(call->callee);
            if(name=="host"||name=="abi")require(unsafe,call->span,"unrestricted host/ABI call");
            else if(name=="asm")require(unsafe,call->span,"inline assembly");
            else if(name=="intrinsic")require(unsafe,call->span,"architecture/compiler intrinsic");
            else if(externFunctions_.count(name))require(unsafe,call->span,"extern FFI call to '"+name+"'");
            else if(name=="slice"&&call->args.size()==2)require(unsafe,call->span,"slice construction from a raw pointer");
            return;
        }
    }

    void statement(const StmtPtr&s,bool unsafe){
        if(!s)return;
        if(auto let=std::dynamic_pointer_cast<LetStmt>(s)){expression(let->initializer,unsafe);return;}
        if(auto expr=std::dynamic_pointer_cast<ExprStmt>(s)){expression(expr->expr,unsafe);return;}
        if(auto ret=std::dynamic_pointer_cast<ReturnStmt>(s)){expression(ret->value,unsafe);return;}
        if(auto thrown=std::dynamic_pointer_cast<ThrowStmt>(s)){expression(thrown->value,unsafe);return;}
        if(auto block=std::dynamic_pointer_cast<BlockStmt>(s)){const bool nestedUnsafe=unsafe||block->isUnsafe;for(const auto&child:block->statements)statement(child,nestedUnsafe);return;}
        if(auto branch=std::dynamic_pointer_cast<IfStmt>(s)){expression(branch->condition,unsafe);statement(branch->thenBranch,unsafe);statement(branch->elseBranch,unsafe);return;}
        if(auto loop=std::dynamic_pointer_cast<WhileStmt>(s)){expression(loop->condition,unsafe);statement(loop->body,unsafe);return;}
        if(auto fn=std::dynamic_pointer_cast<FunctionStmt>(s)){if(fn->body)for(const auto&child:fn->body->statements)statement(child,false);return;}
    }

    Diagnostics&diagnostics_;
    std::unordered_set<std::string>externFunctions_;
};

} // namespace

void SafetyAnnotator::annotate(const Program&program)const{
    for(const auto&statement:program.statements)if(auto fn=std::dynamic_pointer_cast<FunctionStmt>(statement);fn&&!fn->isExtern){Annotator annotator;annotator.function(fn);}
}

bool UnsafeChecker::check(const Program&program,Diagnostics&diagnostics)const{
    UnsafeAudit audit(program,diagnostics);
    return audit.run(program);
}

} // namespace noe
