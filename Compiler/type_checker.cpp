#include "noe.hpp"
#include <algorithm>
#include <memory>
#include <functional>

namespace noe {

TypeChecker::TypeChecker(Diagnostics& diagnostics):diagnostics_(diagnostics){pushScope();}
void TypeChecker::pushScope(){scopes_.push_back({});constScopes_.push_back({});}
void TypeChecker::popScope(){scopes_.pop_back();constScopes_.pop_back();}
void TypeChecker::define(const std::string&name,Type type,bool isConst,Span span){auto&scope=scopes_.back();auto&cs=constScopes_.back();if(scope.count(name))diagnostics_.error("NOE-T3003",span,"symbol '"+name+"' is already defined in this scope");else{scope[name]=std::move(type);cs[name]=isConst;}}
std::optional<Type> TypeChecker::resolve(const std::string&name)const{for(auto it=scopes_.rbegin();it!=scopes_.rend();++it){auto f=it->find(name);if(f!=it->end())return f->second;}return std::nullopt;}
bool TypeChecker::isConstSymbol(const std::string&name)const{for(auto it=constScopes_.rbegin();it!=constScopes_.rend();++it){auto f=it->find(name);if(f!=it->end())return f->second;}return false;}
Type TypeChecker::resolveType(const std::string&name,Span span){Type t=typeFromName(name);std::function<bool(const Type&)> validate=[&](const Type&x)->bool{if(x.kind==TypeKind::Record)return records_.count(x.recordName)!=0;if(x.kind==TypeKind::Pointer&&x.pointee)return validate(*x.pointee);return x.kind!=TypeKind::Unknown;};if(!validate(t))diagnostics_.error("NOE-T3000",span,"unknown type '"+name+"'");return t;}
std::optional<RecordField> TypeChecker::resolveField(const Type&base,const std::string&member)const{Type t=base;if(t.kind==TypeKind::Pointer&&t.pointee)t=*t.pointee;if(t.kind!=TypeKind::Record)return std::nullopt;auto r=records_.find(t.recordName);if(r==records_.end())return std::nullopt;for(const auto&f:r->second.fields)if(f.name==member)return f;return std::nullopt;}

bool TypeChecker::check(const Program&program){
    functions_.clear();records_.clear();
    functions_["print"]={{{TypeKind::Unknown}},{TypeKind::Void}};
    functions_["clockMillis"]={{},{TypeKind::I64}};
    functions_["platform"]={{},{TypeKind::String}};
    functions_["textLength"]={{{TypeKind::String}},{TypeKind::Usize}};
    for(const auto&stmt:program.statements)if(auto r=std::dynamic_pointer_cast<RecordStmt>(stmt))records_[r->name]=RecordType{};
    for(const auto&stmt:program.statements)if(auto r=std::dynamic_pointer_cast<RecordStmt>(stmt)){
        std::size_t offset=0,maxAlign=1;auto&rt=records_[r->name];
        for(auto&field:r->fields){Type ft=resolveType(field.typeName,field.span);std::size_t size=ft.size(),align=ft.alignment();if(ft.kind==TypeKind::Record){auto nested=records_.find(ft.recordName);if(nested!=records_.end()){size=nested->second.size;align=nested->second.alignment;}}if(size==0){diagnostics_.error("NOE-T3020",field.span,"record field '"+field.name+"' has incomplete or zero-sized type '"+field.typeName+"'","use a pointer for recursive/forward record references");size=1;}offset=(offset+align-1)/align*align;field.offset=offset;field.size=size;offset+=size;maxAlign=std::max(maxAlign,align);rt.fields.push_back(field);}rt.alignment=maxAlign;rt.size=(offset+maxAlign-1)/maxAlign*maxAlign;r->size=rt.size;r->alignment=rt.alignment;
    }
    for(const auto&stmt:program.statements)if(auto fn=std::dynamic_pointer_cast<FunctionStmt>(stmt)){
        std::vector<Type>params;for(const auto&p:fn->params)params.push_back(p.annotation?resolveType(*p.annotation,p.span):Type{TypeKind::Unknown});Type ret=fn->returnType?resolveType(*fn->returnType,fn->span):Type{TypeKind::Void};if(functions_.count(fn->name))diagnostics_.error("NOE-T3021",fn->span,"function '"+fn->name+"' is already declared");functions_[fn->name]=FunctionType{params,ret,fn->isExtern,fn->isExport};
    }
    for(const auto&stmt:program.statements)checkStmt(stmt);return!diagnostics_.hasErrors();
}

void TypeChecker::checkStmt(const StmtPtr&stmt){
    if(std::dynamic_pointer_cast<RecordStmt>(stmt))return;
    if(auto s=std::dynamic_pointer_cast<LetStmt>(stmt)){Type init=checkExpr(s->initializer);Type declared=s->annotation?resolveType(*s->annotation,s->span):init;if(!canAssign(declared,init))diagnostics_.error("NOE-T3001",s->span,"cannot assign "+init.name()+" to "+declared.name());define(s->name,declared,s->isConst,s->span);return;}
    if(auto s=std::dynamic_pointer_cast<ExprStmt>(stmt)){checkExpr(s->expr);return;}
    if(auto s=std::dynamic_pointer_cast<BlockStmt>(stmt)){pushScope();for(const auto&x:s->statements)checkStmt(x);popScope();return;}
    if(auto s=std::dynamic_pointer_cast<IfStmt>(stmt)){Type c=checkExpr(s->condition);if(c.kind!=TypeKind::Bool&&c.kind!=TypeKind::Unknown)diagnostics_.error("NOE-T3004",s->condition->span,"if condition must be bool, found "+c.name());checkStmt(s->thenBranch);if(s->elseBranch)checkStmt(s->elseBranch);return;}
    if(auto s=std::dynamic_pointer_cast<WhileStmt>(stmt)){Type c=checkExpr(s->condition);if(c.kind!=TypeKind::Bool&&c.kind!=TypeKind::Unknown)diagnostics_.error("NOE-T3004",s->condition->span,"while condition must be bool, found "+c.name());checkStmt(s->body);return;}
    if(auto s=std::dynamic_pointer_cast<ReturnStmt>(stmt)){Type v=s->value?checkExpr(s->value):Type{TypeKind::Void};if(!canAssign(currentReturn_,v))diagnostics_.error("NOE-T3005",s->span,"return type mismatch: expected "+currentReturn_.name()+", found "+v.name());return;}
    if(auto s=std::dynamic_pointer_cast<FunctionStmt>(stmt)){auto sig=functions_[s->name];if(s->isExtern)return;Type saved=currentReturn_;currentReturn_=sig.result;pushScope();for(std::size_t i=0;i<s->params.size();++i)define(s->params[i].name,sig.params[i],false,s->params[i].span);for(const auto&x:s->body->statements)checkStmt(x);popScope();currentReturn_=saved;return;}
}

Type TypeChecker::checkExpr(const ExprPtr&expr){
    if(auto e=std::dynamic_pointer_cast<LiteralExpr>(expr)){if(std::holds_alternative<std::monostate>(e->value))return{TypeKind::Null};if(std::holds_alternative<std::int64_t>(e->value))return{TypeKind::Int};if(std::holds_alternative<double>(e->value))return{TypeKind::Float};if(std::holds_alternative<bool>(e->value))return{TypeKind::Bool};return{TypeKind::String};}
    if(auto e=std::dynamic_pointer_cast<NameExpr>(expr)){auto t=resolve(e->name);if(t)return*t;if(functions_.count(e->name)||e->name=="host"||e->name=="abi")return{TypeKind::Unknown};diagnostics_.error("NOE-T3002",e->span,"unknown symbol '"+e->name+"'");return{TypeKind::Unknown};}
    if(auto e=std::dynamic_pointer_cast<UnaryExpr>(expr)){
        Type t=checkExpr(e->operand);
        if(e->op==TokenKind::Ampersand){bool lvalue=std::dynamic_pointer_cast<NameExpr>(e->operand)||std::dynamic_pointer_cast<MemberExpr>(e->operand)||std::dynamic_pointer_cast<IndexExpr>(e->operand);if(auto u=std::dynamic_pointer_cast<UnaryExpr>(e->operand))lvalue=u->op==TokenKind::Star;if(!lvalue)diagnostics_.error("NOE-T3022",e->span,"address-of requires an addressable value");Type p{TypeKind::Pointer};p.pointee=std::make_shared<Type>(t);return p;}
        if(e->op==TokenKind::Star){if(t.kind!=TypeKind::Pointer||!t.pointee){diagnostics_.error("NOE-T3023",e->span,"dereference requires a pointer");return{TypeKind::Unknown};}e->memoryWidth=std::max<std::size_t>(1,t.pointee->size());e->volatileAccess=t.isVolatile;return*t.pointee;}
        if(e->op==TokenKind::Bang){if(t.kind!=TypeKind::Bool&&t.kind!=TypeKind::Unknown)diagnostics_.error("NOE-T3006",e->span,"operator ! requires bool");return{TypeKind::Bool};}
        if(!t.isNumeric()&&t.kind!=TypeKind::Unknown)diagnostics_.error("NOE-T3006",e->span,"numeric unary operator requires a numeric type");return t;
    }
    if(auto e=std::dynamic_pointer_cast<MemberExpr>(expr)){Type b=checkExpr(e->object);auto f=resolveField(b,e->member);if(!f){diagnostics_.error("NOE-T3024",e->span,"type '"+b.name()+"' has no field '"+e->member+"'");return{TypeKind::Unknown};}e->offset=f->offset;e->fieldSize=f->size;if(b.kind==TypeKind::Pointer){e->volatileAccess=b.isVolatile;e->baseIsPointer=true;}return resolveType(f->typeName,e->span);}
    if(auto e=std::dynamic_pointer_cast<IndexExpr>(expr)){Type p=checkExpr(e->object);Type idx=checkExpr(e->index);if(!idx.isInteger()&&idx.kind!=TypeKind::Unknown)diagnostics_.error("NOE-T3025",e->index->span,"pointer index must be an integer");if(p.kind!=TypeKind::Pointer||!p.pointee){diagnostics_.error("NOE-T3026",e->span,"indexing requires a pointer");return{TypeKind::Unknown};}e->elementSize=std::max<std::size_t>(1,p.pointee->size());e->volatileAccess=p.isVolatile;return*p.pointee;}
    if(auto e=std::dynamic_pointer_cast<CastExpr>(expr)){Type from=checkExpr(e->value),to=resolveType(e->typeName,e->span);bool ok=(from.isNumeric()&&to.isNumeric())||(from.kind==TypeKind::Pointer&&to.kind==TypeKind::Pointer)||(from.kind==TypeKind::Pointer&&(to.kind==TypeKind::Usize||to.kind==TypeKind::Isize))||(to.kind==TypeKind::Pointer&&(from.kind==TypeKind::Usize||from.kind==TypeKind::Isize||from.kind==TypeKind::Int))||(from.kind==TypeKind::Null&&to.kind==TypeKind::Pointer);if(!ok&&from.kind!=TypeKind::Unknown&&to.kind!=TypeKind::Unknown)diagnostics_.error("NOE-T3027",e->span,"cannot cast "+from.name()+" to "+to.name());return to;}
    if(auto e=std::dynamic_pointer_cast<BinaryExpr>(expr)){
        if(e->op==TokenKind::Equal){Type lhs=checkExpr(e->left),rhs=checkExpr(e->right);if(auto n=std::dynamic_pointer_cast<NameExpr>(e->left))if(isConstSymbol(n->name))diagnostics_.error("NOE-T3014",e->span,"cannot assign to const '"+n->name+"'");if(!canAssign(lhs,rhs))diagnostics_.error("NOE-T3001",e->span,"cannot assign "+rhs.name()+" to "+lhs.name());return lhs;}
        Type l=checkExpr(e->left),r=checkExpr(e->right);switch(e->op){case TokenKind::Plus:if(l.kind==TypeKind::String&&r.kind==TypeKind::String)return{TypeKind::String};if(l.kind==TypeKind::Pointer&&r.isInteger())return l;if(r.kind==TypeKind::Pointer&&l.isInteger())return r;[[fallthrough]];case TokenKind::Minus:case TokenKind::Star:case TokenKind::Slash:case TokenKind::Percent:if((!l.isNumeric()||!r.isNumeric())&&l.kind!=TypeKind::Unknown&&r.kind!=TypeKind::Unknown)diagnostics_.error("NOE-T3007",e->span,"arithmetic operator requires numeric operands");return(l.kind==TypeKind::Float||r.kind==TypeKind::Float)?Type{TypeKind::Float}:l;case TokenKind::Less:case TokenKind::LessEqual:case TokenKind::Greater:case TokenKind::GreaterEqual:case TokenKind::EqualEqual:case TokenKind::BangEqual:case TokenKind::AndAnd:case TokenKind::OrOr:return{TypeKind::Bool};default:return{TypeKind::Unknown};}
    }
    if(auto e=std::dynamic_pointer_cast<CallExpr>(expr)){auto n=std::dynamic_pointer_cast<NameExpr>(e->callee);if(!n){diagnostics_.error("NOE-T3010",e->span,"only named functions are callable");return{TypeKind::Unknown};}if(n->name=="host"||n->name=="abi"){if(e->args.empty()){diagnostics_.error("NOE-T3015",e->span,n->name+" requires a service name");return{TypeKind::Unknown};}Type service=checkExpr(e->args[0]);if(service.kind!=TypeKind::String&&service.kind!=TypeKind::Unknown)diagnostics_.error("NOE-T3016",e->args[0]->span,"service name must be a string");for(std::size_t i=1;i<e->args.size();++i)checkExpr(e->args[i]);return{TypeKind::Unknown};}auto it=functions_.find(n->name);if(it==functions_.end()){diagnostics_.error("NOE-T3011",e->span,"unknown function '"+n->name+"'");return{TypeKind::Unknown};}const auto&sig=it->second;if(n->name!="print"&&e->args.size()!=sig.params.size())diagnostics_.error("NOE-T3012",e->span,"wrong argument count for '"+n->name+"'");for(std::size_t i=0;i<e->args.size();++i){Type a=checkExpr(e->args[i]);if(i<sig.params.size()&&!canAssign(sig.params[i],a))diagnostics_.error("NOE-T3013",e->args[i]->span,"argument type mismatch: expected "+sig.params[i].name()+", found "+a.name());}return sig.result;}
    return{TypeKind::Unknown};
}
} // namespace noe
