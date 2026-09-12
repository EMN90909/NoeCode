#include "noe.hpp"
#include <algorithm>
#include <cctype>
#include <functional>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace noe {
namespace {
std::string trim(std::string value){auto first=value.find_first_not_of(" \t\r\n");if(first==std::string::npos)return{};auto last=value.find_last_not_of(" \t\r\n");return value.substr(first,last-first+1);}
bool ident(char c){return std::isalnum(static_cast<unsigned char>(c))||c=='_';}
std::vector<std::string> splitConstraints(const std::string& raw){std::vector<std::string> out;std::string current;for(char c:raw){if(c=='+'||c=='&'){auto v=trim(current);if(!v.empty())out.push_back(v);current.clear();}else current+=c;}auto v=trim(current);if(!v.empty())out.push_back(v);return out;}
std::string calleeName(const ExprPtr& expr){if(auto n=std::dynamic_pointer_cast<NameExpr>(expr))return n->name;return{};}
std::string safeType(const std::string& type){std::string out;for(char c:type)out+=ident(c)?c:'_';while(out.find("__")!=std::string::npos)out.replace(out.find("__"),2,"_");if(out.empty())out="unknown";return out;}
std::string substitute(std::string value,const std::unordered_map<std::string,std::string>& bindings){std::string out;for(std::size_t i=0;i<value.size();){if(std::isalpha(static_cast<unsigned char>(value[i]))||value[i]=='_'){std::size_t j=i+1;while(j<value.size()&&ident(value[j]))++j;auto word=value.substr(i,j-i);auto it=bindings.find(word);out+=it==bindings.end()?word:it->second;i=j;}else out+=value[i++];}return out;}

ExprPtr cloneExpr(const ExprPtr& e){
    if(!e)return{};
    if(auto x=std::dynamic_pointer_cast<LiteralExpr>(e)){auto n=std::make_shared<LiteralExpr>(*x);return n;}
    if(auto x=std::dynamic_pointer_cast<NameExpr>(e)){auto n=std::make_shared<NameExpr>(*x);return n;}
    if(auto x=std::dynamic_pointer_cast<UnaryExpr>(e)){auto n=std::make_shared<UnaryExpr>(*x);n->operand=cloneExpr(x->operand);return n;}
    if(auto x=std::dynamic_pointer_cast<BinaryExpr>(e)){auto n=std::make_shared<BinaryExpr>(*x);n->left=cloneExpr(x->left);n->right=cloneExpr(x->right);return n;}
    if(auto x=std::dynamic_pointer_cast<CallExpr>(e)){auto n=std::make_shared<CallExpr>(*x);n->callee=cloneExpr(x->callee);n->args.clear();for(const auto&a:x->args)n->args.push_back(cloneExpr(a));return n;}
    if(auto x=std::dynamic_pointer_cast<CastExpr>(e)){auto n=std::make_shared<CastExpr>(*x);n->value=cloneExpr(x->value);return n;}
    if(auto x=std::dynamic_pointer_cast<IndexExpr>(e)){auto n=std::make_shared<IndexExpr>(*x);n->object=cloneExpr(x->object);n->index=cloneExpr(x->index);return n;}
    if(auto x=std::dynamic_pointer_cast<MemberExpr>(e)){auto n=std::make_shared<MemberExpr>(*x);n->object=cloneExpr(x->object);return n;}
    if(auto x=std::dynamic_pointer_cast<ArrayExpr>(e)){auto n=std::make_shared<ArrayExpr>(*x);n->elements.clear();for(const auto&a:x->elements)n->elements.push_back(cloneExpr(a));return n;}
    return{};
}
StmtPtr cloneStmt(const StmtPtr& s){
    if(!s)return{};
    if(auto x=std::dynamic_pointer_cast<ExprStmt>(s)){auto n=std::make_shared<ExprStmt>(*x);n->expr=cloneExpr(x->expr);return n;}
    if(auto x=std::dynamic_pointer_cast<LetStmt>(s)){auto n=std::make_shared<LetStmt>(*x);n->initializer=cloneExpr(x->initializer);return n;}
    if(auto x=std::dynamic_pointer_cast<BlockStmt>(s)){auto n=std::make_shared<BlockStmt>(*x);n->statements.clear();for(const auto&i:x->statements)n->statements.push_back(cloneStmt(i));return n;}
    if(auto x=std::dynamic_pointer_cast<IfStmt>(s)){auto n=std::make_shared<IfStmt>(*x);n->condition=cloneExpr(x->condition);n->thenBranch=cloneStmt(x->thenBranch);n->elseBranch=cloneStmt(x->elseBranch);return n;}
    if(auto x=std::dynamic_pointer_cast<WhileStmt>(s)){auto n=std::make_shared<WhileStmt>(*x);n->condition=cloneExpr(x->condition);n->body=cloneStmt(x->body);return n;}
    if(auto x=std::dynamic_pointer_cast<ReturnStmt>(s)){auto n=std::make_shared<ReturnStmt>(*x);n->value=cloneExpr(x->value);return n;}
    if(auto x=std::dynamic_pointer_cast<ThrowStmt>(s)){auto n=std::make_shared<ThrowStmt>(*x);n->value=cloneExpr(x->value);return n;}
    if(auto x=std::dynamic_pointer_cast<RecordStmt>(s))return std::make_shared<RecordStmt>(*x);
    if(auto x=std::dynamic_pointer_cast<ImportStmt>(s))return std::make_shared<ImportStmt>(*x);
    if(auto x=std::dynamic_pointer_cast<ModuleStmt>(s))return std::make_shared<ModuleStmt>(*x);
    return s;
}
void substituteStmt(const StmtPtr&s,const std::unordered_map<std::string,std::string>&b){if(!s)return;if(auto x=std::dynamic_pointer_cast<LetStmt>(s)){if(x->annotation)x->annotation=substitute(*x->annotation,b);}else if(auto x=std::dynamic_pointer_cast<BlockStmt>(s)){for(auto&i:x->statements)substituteStmt(i,b);}else if(auto x=std::dynamic_pointer_cast<IfStmt>(s)){substituteStmt(x->thenBranch,b);substituteStmt(x->elseBranch,b);}else if(auto x=std::dynamic_pointer_cast<WhileStmt>(s))substituteStmt(x->body,b);}
std::string elementType(const std::string&t){if(t.rfind("[]",0)==0)return t.substr(2);if(t.rfind("*volatile ",0)==0)return t.substr(10);if(t.rfind("*",0)==0)return t.substr(1);if(t.size()>3&&t.front()=='['){auto semi=t.rfind(';');if(semi!=std::string::npos)return t.substr(1,semi-1);}return"unknown";}
bool capability(const std::string&type,const std::string&c){const Type t=typeFromName(type);if(c=="Eq")return t.kind!=TypeKind::Void&&t.kind!=TypeKind::Unknown;if(c=="Ord")return t.isNumeric()||t.kind==TypeKind::String||t.kind==TypeKind::Bool;if(c=="Numeric")return t.isNumeric();if(c=="Integer")return t.isInteger();if(c=="Copy")return !t.isAggregate()&&t.kind!=TypeKind::Void&&t.kind!=TypeKind::Unknown;return false;}

class Monomorphizer {
public:
    Monomorphizer(Program&program,Diagnostics&diagnostics):program_(program),diagnostics_(diagnostics){for(const auto&s:program.statements)if(auto f=std::dynamic_pointer_cast<FunctionStmt>(s))functions_[f->name]=f;}
    bool run(){
        std::vector<StmtPtr> nongeneric;
        for(const auto&s:program_.statements){auto f=std::dynamic_pointer_cast<FunctionStmt>(s);if(!f||f->genericParams.empty())nongeneric.push_back(s);}
        for(auto&s:nongeneric){if(auto f=std::dynamic_pointer_cast<FunctionStmt>(s)){Env env;for(const auto&p:f->params)if(p.annotation)env[p.name]=*p.annotation;processStmt(f->body,env);}else{Env env;processStmt(s,env);}}
        nongeneric.insert(nongeneric.end(),instances_.begin(),instances_.end());program_.statements=std::move(nongeneric);return!diagnostics_.hasErrors();
    }
private:
    using Env=std::unordered_map<std::string,std::string>;
    std::string infer(const ExprPtr&e,Env&env){
        if(!e)return"void";
        if(auto x=std::dynamic_pointer_cast<LiteralExpr>(e)){if(std::holds_alternative<std::int64_t>(x->value))return"int";if(std::holds_alternative<double>(x->value))return"float";if(std::holds_alternative<bool>(x->value))return"bool";if(std::holds_alternative<std::string>(x->value))return"string";return"null";}
        if(auto x=std::dynamic_pointer_cast<NameExpr>(e)){auto i=env.find(x->name);return i==env.end()?"unknown":i->second;}
        if(auto x=std::dynamic_pointer_cast<CastExpr>(e))return x->typeName;
        if(auto x=std::dynamic_pointer_cast<UnaryExpr>(e)){auto t=infer(x->operand,env);if(x->op==TokenKind::Bang)return"bool";if(x->op==TokenKind::Ampersand)return"*"+t;if(x->op==TokenKind::Star)return elementType(t);return t;}
        if(auto x=std::dynamic_pointer_cast<BinaryExpr>(e)){auto l=infer(x->left,env);infer(x->right,env);switch(x->op){case TokenKind::EqualEqual:case TokenKind::BangEqual:case TokenKind::Less:case TokenKind::LessEqual:case TokenKind::Greater:case TokenKind::GreaterEqual:case TokenKind::AndAnd:case TokenKind::OrOr:return"bool";default:return l;}}
        if(auto x=std::dynamic_pointer_cast<ArrayExpr>(e)){auto t=x->elements.empty()?"unknown":infer(x->elements.front(),env);return"["+t+";"+std::to_string(x->elements.size())+"]";}
        if(auto x=std::dynamic_pointer_cast<IndexExpr>(e))return elementType(infer(x->object,env));
        if(auto x=std::dynamic_pointer_cast<MemberExpr>(e)){infer(x->object,env);return"unknown";}
        if(auto x=std::dynamic_pointer_cast<CallExpr>(e))return processCall(x,env);
        return"unknown";
    }
    bool bind(const std::string&pattern,const std::string&actual,const std::vector<std::string>&gps,std::unordered_map<std::string,std::string>&bindings){for(const auto&gp:gps){if(pattern==gp){auto it=bindings.find(gp);if(it==bindings.end()){bindings[gp]=actual;return true;}return it->second==actual;}if(pattern=="*"+gp&&actual.rfind("*",0)==0)return bind(gp,elementType(actual),gps,bindings);if(pattern=="[]"+gp&&actual.rfind("[]",0)==0)return bind(gp,elementType(actual),gps,bindings);}return pattern==actual||typeFromName(pattern).kind==TypeKind::Unknown;}
    std::string processCall(const std::shared_ptr<CallExpr>&call,Env&env){
        auto name=calleeName(call->callee);std::vector<std::string>args;for(auto&a:call->args)args.push_back(infer(a,env));auto fit=functions_.find(name);if(fit==functions_.end())return name=="len"?"usize":"unknown";auto fn=fit->second;if(fn->genericParams.empty())return fn->returnType.value_or("void");std::unordered_map<std::string,std::string>bindings;for(std::size_t i=0;i<fn->params.size()&&i<args.size();++i)if(fn->params[i].annotation&&!bind(*fn->params[i].annotation,args[i],fn->genericParams,bindings))diagnostics_.error("NQR-G4001",call->span,"generic arguments for '"+name+"' disagree");for(const auto&gp:fn->genericParams)if(!bindings.count(gp))diagnostics_.error("NQR-G4002",call->span,"cannot infer generic parameter '"+gp+"' for '"+name+"'");if(diagnostics_.hasErrors())return"unknown";for(const auto&[gp,requirements]:fn->genericConstraints){auto it=bindings.find(gp);if(it==bindings.end())continue;for(const auto&req:requirements)if(!capability(it->second,req))diagnostics_.error("NQR-G4003",call->span,"type '"+it->second+"' does not satisfy constraint '"+req+"' for '"+gp+"'");}if(diagnostics_.hasErrors())return"unknown";auto instance=instantiate(fn,bindings);if(auto callee=std::dynamic_pointer_cast<NameExpr>(call->callee))callee->name=instance;return substitute(fn->returnType.value_or("void"),bindings);
    }
    std::string instantiate(const std::shared_ptr<FunctionStmt>&fn,const std::unordered_map<std::string,std::string>&bindings){std::ostringstream key,name;key<<fn->name;name<<fn->name;for(const auto&gp:fn->genericParams){auto type=bindings.at(gp);key<<'|'<<type;name<<"__"<<safeType(type);}auto k=key.str();if(auto i=cache_.find(k);i!=cache_.end())return i->second;auto instance=name.str();cache_[k]=instance;auto copy=std::make_shared<FunctionStmt>(*fn);copy->name=instance;copy->genericParams.clear();copy->genericConstraints.clear();copy->params=fn->params;for(auto&p:copy->params)if(p.annotation)p.annotation=substitute(*p.annotation,bindings);if(copy->returnType)copy->returnType=substitute(*copy->returnType,bindings);copy->body=std::dynamic_pointer_cast<BlockStmt>(cloneStmt(fn->body));substituteStmt(copy->body,bindings);instances_.push_back(copy);Env env;for(const auto&p:copy->params)if(p.annotation)env[p.name]=*p.annotation;processStmt(copy->body,env);return instance;}
    void processStmt(const StmtPtr&s,Env&env){
        if(!s)return;if(auto x=std::dynamic_pointer_cast<ExprStmt>(s)){infer(x->expr,env);return;}if(auto x=std::dynamic_pointer_cast<LetStmt>(s)){auto t=infer(x->initializer,env);env[x->name]=x->annotation.value_or(t);return;}if(auto x=std::dynamic_pointer_cast<ReturnStmt>(s)){infer(x->value,env);return;}if(auto x=std::dynamic_pointer_cast<ThrowStmt>(s)){infer(x->value,env);return;}if(auto x=std::dynamic_pointer_cast<BlockStmt>(s)){Env nested=env;for(auto&i:x->statements)processStmt(i,nested);return;}if(auto x=std::dynamic_pointer_cast<IfStmt>(s)){infer(x->condition,env);Env a=env,b=env;processStmt(x->thenBranch,a);processStmt(x->elseBranch,b);return;}if(auto x=std::dynamic_pointer_cast<WhileStmt>(s)){infer(x->condition,env);Env nested=env;processStmt(x->body,nested);return;}
    }
    Program&program_;Diagnostics&diagnostics_;std::unordered_map<std::string,std::shared_ptr<FunctionStmt>>functions_;std::unordered_map<std::string,std::string>cache_;std::vector<StmtPtr>instances_;
};
}

GenericSyntaxInfo preprocessGenericSyntax(const std::string&source){GenericSyntaxInfo info;info.source=source;bool inString=false,lineComment=false,blockComment=false,escape=false;for(std::size_t i=0;i<source.size();++i){char c=source[i],n=i+1<source.size()?source[i+1]:'\0';if(lineComment){if(c=='\n')lineComment=false;continue;}if(blockComment){if(c=='*'&&n=='/'){blockComment=false;++i;}continue;}if(inString){if(!escape&&c=='"')inString=false;escape=!escape&&c=='\\';if(c!='\\')escape=false;continue;}if(c=='"'){inString=true;continue;}if(c=='/'&&n=='/'){lineComment=true;++i;continue;}if(c=='/'&&n=='*'){blockComment=true;++i;continue;}const std::string kw="function";if(i+kw.size()>source.size()||source.compare(i,kw.size(),kw)!=0||(i&&ident(source[i-1]))||(i+kw.size()<source.size()&&ident(source[i+kw.size()])))continue;std::size_t p=i+kw.size();while(p<source.size()&&std::isspace(static_cast<unsigned char>(source[p])))++p;std::size_t ns=p;while(p<source.size()&&ident(source[p]))++p;if(p==ns)continue;auto fn=source.substr(ns,p-ns);while(p<source.size()&&std::isspace(static_cast<unsigned char>(source[p])))++p;if(p>=source.size()||source[p]!='<')continue;std::size_t end=source.find('>',p+1);if(end==std::string::npos)continue;std::size_t seg=p+1;while(seg<end){std::size_t comma=source.find(',',seg);if(comma==std::string::npos||comma>end)comma=end;auto part=source.substr(seg,comma-seg);auto colon=part.find(':');if(colon!=std::string::npos){auto gp=trim(part.substr(0,colon));auto constraints=splitConstraints(part.substr(colon+1));if(!gp.empty()&&!constraints.empty())info.constraints[fn][gp]=constraints;for(std::size_t x=seg+colon;x<comma;++x)if(info.source[x]!='\n'&&info.source[x]!='\r')info.source[x]=' ';}seg=comma+1;}i=end;}
return info;}
void applyGenericSyntax(Program&program,const GenericSyntaxInfo&syntax){for(auto&s:program.statements)if(auto fn=std::dynamic_pointer_cast<FunctionStmt>(s)){auto it=syntax.constraints.find(fn->name);if(it!=syntax.constraints.end())fn->genericConstraints=it->second;}}
bool GenericEngine::monomorphize(Program&program,Diagnostics&diagnostics)const{return Monomorphizer(program,diagnostics).run();}
} // namespace noe
