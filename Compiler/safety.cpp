#include "noe.hpp"
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace noe {
namespace {

std::optional<std::size_t> fixedArrayCount(const std::string&type){
    if(type.size()<4||type.front()!='['||type.rfind("[]",0)==0)return std::nullopt;
    auto semi=type.rfind(';'),close=type.rfind(']');if(semi==std::string::npos||close==std::string::npos||semi>=close)return std::nullopt;
    try{return static_cast<std::size_t>(std::stoull(type.substr(semi+1,close-semi-1),nullptr,0));}catch(...){return std::nullopt;}
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

} // namespace

void SafetyAnnotator::annotate(const Program&program)const{
    for(const auto&statement:program.statements)if(auto fn=std::dynamic_pointer_cast<FunctionStmt>(statement);fn&&!fn->isExtern){Annotator annotator;annotator.function(fn);}
}

} // namespace noe
