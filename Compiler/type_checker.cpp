#include "noe.hpp"
#include <memory>

namespace noe {

TypeChecker::TypeChecker(Diagnostics& diagnostics) : diagnostics_(diagnostics) { pushScope(); }
void TypeChecker::pushScope(){ scopes_.push_back({}); constScopes_.push_back({}); }
void TypeChecker::popScope(){ scopes_.pop_back(); constScopes_.pop_back(); }
void TypeChecker::define(const std::string& name, Type type, bool isConst, Span span){
    auto& scope=scopes_.back();
    auto& constScope=constScopes_.back();
    if(scope.count(name)) diagnostics_.error("NOE-T3003",span,"symbol '"+name+"' is already defined in this scope");
    else { scope[name]=type; constScope[name]=isConst; }
}
std::optional<Type> TypeChecker::resolve(const std::string& name) const {
    for(auto it=scopes_.rbegin();it!=scopes_.rend();++it){ auto f=it->find(name); if(f!=it->end()) return f->second; }
    return std::nullopt;
}
bool TypeChecker::isConstSymbol(const std::string& name) const {
    for(auto it=constScopes_.rbegin();it!=constScopes_.rend();++it){ auto f=it->find(name); if(f!=it->end()) return f->second; }
    return false;
}

bool TypeChecker::check(const Program& program){
    functions_["print"] = FunctionType{{Type{TypeKind::Unknown}}, Type{TypeKind::Void}};
    functions_["clockMillis"] = FunctionType{{}, Type{TypeKind::Int}};
    functions_["platform"] = FunctionType{{}, Type{TypeKind::String}};
    functions_["textLength"] = FunctionType{{Type{TypeKind::String}}, Type{TypeKind::Int}};
    for(const auto& stmt:program.statements){
        if(auto fn=std::dynamic_pointer_cast<FunctionStmt>(stmt)){
            std::vector<Type> params;
            for(const auto& p:fn->params){
                Type t=p.annotation?typeFromName(*p.annotation):Type{TypeKind::Unknown};
                if(p.annotation && t.kind==TypeKind::Unknown) diagnostics_.error("NOE-T3000",p.span,"unknown type '"+*p.annotation+"'");
                params.push_back(t);
            }
            Type ret=fn->returnType?typeFromName(*fn->returnType):Type{TypeKind::Void};
            if(fn->returnType && ret.kind==TypeKind::Unknown) diagnostics_.error("NOE-T3000",fn->span,"unknown return type '"+*fn->returnType+"'");
            functions_[fn->name]=FunctionType{params,ret};
        }
    }
    for(const auto& stmt:program.statements) checkStmt(stmt);
    return !diagnostics_.hasErrors();
}

void TypeChecker::checkStmt(const StmtPtr& stmt){
    if(auto s=std::dynamic_pointer_cast<LetStmt>(stmt)){
        Type init=checkExpr(s->initializer);
        Type declared=s->annotation?typeFromName(*s->annotation):init;
        if(s->annotation && declared.kind==TypeKind::Unknown) diagnostics_.error("NOE-T3000",s->span,"unknown type '"+*s->annotation+"'");
        if(!canAssign(declared,init)) diagnostics_.error("NOE-T3001",s->span,"cannot assign "+init.name()+" to "+declared.name(),"change the value or the declared type");
        define(s->name,declared,s->isConst,s->span); return;
    }
    if(auto s=std::dynamic_pointer_cast<ExprStmt>(stmt)){ checkExpr(s->expr); return; }
    if(auto s=std::dynamic_pointer_cast<BlockStmt>(stmt)){ pushScope(); for(const auto& x:s->statements) checkStmt(x); popScope(); return; }
    if(auto s=std::dynamic_pointer_cast<IfStmt>(stmt)){
        Type c=checkExpr(s->condition); if(c.kind!=TypeKind::Bool && c.kind!=TypeKind::Unknown) diagnostics_.error("NOE-T3004",s->condition->span,"if condition must be bool, found "+c.name());
        checkStmt(s->thenBranch); if(s->elseBranch) checkStmt(s->elseBranch); return;
    }
    if(auto s=std::dynamic_pointer_cast<WhileStmt>(stmt)){
        Type c=checkExpr(s->condition); if(c.kind!=TypeKind::Bool && c.kind!=TypeKind::Unknown) diagnostics_.error("NOE-T3004",s->condition->span,"while condition must be bool, found "+c.name());
        checkStmt(s->body); return;
    }
    if(auto s=std::dynamic_pointer_cast<ReturnStmt>(stmt)){
        Type v=s->value?checkExpr(s->value):Type{TypeKind::Void};
        if(!canAssign(currentReturn_,v)) diagnostics_.error("NOE-T3005",s->span,"return type mismatch: expected "+currentReturn_.name()+", found "+v.name());
        return;
    }
    if(auto s=std::dynamic_pointer_cast<FunctionStmt>(stmt)){
        auto sig=functions_[s->name]; Type saved=currentReturn_; currentReturn_=sig.result;
        pushScope(); for(std::size_t i=0;i<s->params.size();++i) define(s->params[i].name,sig.params[i],false,s->params[i].span);
        for(const auto& x:s->body->statements) checkStmt(x);
        popScope(); currentReturn_=saved; return;
    }
}

Type TypeChecker::checkExpr(const ExprPtr& expr){
    if(auto e=std::dynamic_pointer_cast<LiteralExpr>(expr)){
        if(std::holds_alternative<std::monostate>(e->value)) return {TypeKind::Null};
        if(std::holds_alternative<std::int64_t>(e->value)) return {TypeKind::Int};
        if(std::holds_alternative<double>(e->value)) return {TypeKind::Float};
        if(std::holds_alternative<bool>(e->value)) return {TypeKind::Bool};
        return {TypeKind::String};
    }
    if(auto e=std::dynamic_pointer_cast<NameExpr>(expr)){
        auto t=resolve(e->name); if(t) return *t;
        if(functions_.count(e->name) || e->name=="host") return {TypeKind::Unknown};
        diagnostics_.error("NOE-T3002",e->span,"unknown symbol '"+e->name+"'"); return {TypeKind::Unknown};
    }
    if(auto e=std::dynamic_pointer_cast<UnaryExpr>(expr)){
        Type t=checkExpr(e->operand);
        if(e->op==TokenKind::Bang){ if(t.kind!=TypeKind::Bool && t.kind!=TypeKind::Unknown) diagnostics_.error("NOE-T3006",e->span,"operator ! requires bool"); return {TypeKind::Bool}; }
        if(!t.isNumeric() && t.kind!=TypeKind::Unknown) diagnostics_.error("NOE-T3006",e->span,"numeric unary operator requires int or float");
        return t;
    }
    if(auto e=std::dynamic_pointer_cast<BinaryExpr>(expr)){
        if(e->op==TokenKind::Equal){
            auto n=std::dynamic_pointer_cast<NameExpr>(e->left); Type rhs=checkExpr(e->right); if(!n) return {TypeKind::Unknown};
            auto lhs=resolve(n->name); if(!lhs){ diagnostics_.error("NOE-T3002",n->span,"unknown symbol '"+n->name+"'"); return {TypeKind::Unknown}; }
            if(isConstSymbol(n->name)) diagnostics_.error("NOE-T3014",e->span,"cannot assign to const '"+n->name+"'","declare it with let when mutation is intended");
            if(!canAssign(*lhs,rhs)) diagnostics_.error("NOE-T3001",e->span,"cannot assign "+rhs.name()+" to "+lhs->name());
            return *lhs;
        }
        Type l=checkExpr(e->left), r=checkExpr(e->right);
        switch(e->op){
            case TokenKind::Plus:
                if(l.kind==TypeKind::String && r.kind==TypeKind::String) return {TypeKind::String};
                [[fallthrough]];
            case TokenKind::Minus: case TokenKind::Star: case TokenKind::Slash: case TokenKind::Percent:
                if((!l.isNumeric()||!r.isNumeric()) && l.kind!=TypeKind::Unknown && r.kind!=TypeKind::Unknown) diagnostics_.error("NOE-T3007",e->span,"arithmetic operator requires numeric operands");
                return (l.kind==TypeKind::Float||r.kind==TypeKind::Float)?Type{TypeKind::Float}:Type{TypeKind::Int};
            case TokenKind::Less: case TokenKind::LessEqual: case TokenKind::Greater: case TokenKind::GreaterEqual:
                if((!l.isNumeric()||!r.isNumeric()) && l.kind!=TypeKind::Unknown && r.kind!=TypeKind::Unknown) diagnostics_.error("NOE-T3008",e->span,"comparison requires numeric operands");
                return {TypeKind::Bool};
            case TokenKind::EqualEqual: case TokenKind::BangEqual: return {TypeKind::Bool};
            case TokenKind::AndAnd: case TokenKind::OrOr:
                if((l.kind!=TypeKind::Bool||r.kind!=TypeKind::Bool) && l.kind!=TypeKind::Unknown && r.kind!=TypeKind::Unknown) diagnostics_.error("NOE-T3009",e->span,"logical operator requires bool operands");
                return {TypeKind::Bool};
            default: return {TypeKind::Unknown};
        }
    }
    if(auto e=std::dynamic_pointer_cast<CallExpr>(expr)){
        auto n=std::dynamic_pointer_cast<NameExpr>(e->callee);
        if(!n){ diagnostics_.error("NOE-T3010",e->span,"only named functions are callable in bootstrap noqeri"); return {TypeKind::Unknown}; }
        if(n->name=="host"){
            if(e->args.empty()){
                diagnostics_.error("NOE-T3015",e->span,"host requires a service name as its first argument");
                return {TypeKind::Unknown};
            }
            Type service=checkExpr(e->args[0]);
            if(service.kind!=TypeKind::String && service.kind!=TypeKind::Unknown)
                diagnostics_.error("NOE-T3016",e->args[0]->span,"host service name must be a string");
            for(std::size_t i=1;i<e->args.size();++i) checkExpr(e->args[i]);
            return {TypeKind::Unknown};
        }
        auto it=functions_.find(n->name); if(it==functions_.end()){ diagnostics_.error("NOE-T3011",e->span,"unknown function '"+n->name+"'"); return {TypeKind::Unknown}; }
        auto& sig=it->second;
        if(n->name!="print" && e->args.size()!=sig.params.size()) diagnostics_.error("NOE-T3012",e->span,"wrong argument count for '"+n->name+"'");
        for(std::size_t i=0;i<e->args.size();++i){ Type a=checkExpr(e->args[i]); if(i<sig.params.size() && !canAssign(sig.params[i],a)) diagnostics_.error("NOE-T3013",e->args[i]->span,"argument type mismatch: expected "+sig.params[i].name()+", found "+a.name()); }
        return sig.result;
    }
    return {TypeKind::Unknown};
}

} // namespace noe
