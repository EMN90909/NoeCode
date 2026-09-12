#include "noe.hpp"
#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>

namespace noe {

const char* tokenKindName(TokenKind k){
    switch(k){
        case TokenKind::Eof:return"eof";case TokenKind::Identifier:return"identifier";case TokenKind::Integer:return"integer";case TokenKind::Float:return"float";case TokenKind::String:return"string";
        case TokenKind::Let:return"let";case TokenKind::Const:return"const";case TokenKind::Function:return"function";case TokenKind::If:return"if";case TokenKind::Else:return"else";case TokenKind::While:return"while";
        case TokenKind::Return:return"return";case TokenKind::True:return"true";case TokenKind::False:return"false";case TokenKind::Null:return"null";case TokenKind::Import:return"import";case TokenKind::Module:return"module";case TokenKind::Record:return"record";
        case TokenKind::Class:return"class";case TokenKind::Extern:return"extern";case TokenKind::Export:return"export";case TokenKind::Volatile:return"volatile";case TokenKind::As:return"as";case TokenKind::Throw:return"throw";case TokenKind::Try:return"try";
        case TokenKind::LParen:return"(";case TokenKind::RParen:return")";case TokenKind::LBrace:return"{";case TokenKind::RBrace:return"}";case TokenKind::LBracket:return"[";case TokenKind::RBracket:return"]";
        case TokenKind::Comma:return",";case TokenKind::Colon:return":";case TokenKind::Semicolon:return";";case TokenKind::Dot:return".";case TokenKind::Plus:return"+";case TokenKind::Minus:return"-";
        case TokenKind::Star:return"*";case TokenKind::Slash:return"/";case TokenKind::Percent:return"%";case TokenKind::Ampersand:return"&";case TokenKind::Bang:return"!";case TokenKind::BangEqual:return"!=";
        case TokenKind::Equal:return"=";case TokenKind::EqualEqual:return"==";case TokenKind::Less:return"<";case TokenKind::LessEqual:return"<=";case TokenKind::Greater:return">";case TokenKind::GreaterEqual:return">=";
        case TokenKind::AndAnd:return"&&";case TokenKind::OrOr:return"||";
    }
    return"?";
}

void Diagnostics::error(std::string code,Span span,std::string message,std::string help){if(code.rfind("NOE-",0)==0||code.rfind("RIC-",0)==0)code.replace(0,4,"NQR-");items_.push_back({std::move(code),std::move(message),span,std::move(help)});}
void Diagnostics::print(const std::string& sourceName)const{for(const auto&d:items_){std::cerr<<d.code<<": "<<d.message<<"\n --> "<<sourceName<<':'<<d.span.line<<':'<<d.span.column<<"\n";if(!d.help.empty())std::cerr<<" help: "<<d.help<<"\n";}}

bool Type::isInteger()const{switch(kind){case TypeKind::I8:case TypeKind::I16:case TypeKind::I32:case TypeKind::I64:case TypeKind::U8:case TypeKind::U16:case TypeKind::U32:case TypeKind::U64:case TypeKind::Isize:case TypeKind::Usize:case TypeKind::Int:return true;default:return false;}}
std::size_t Type::size()const{
    switch(kind){
        case TypeKind::Bool:case TypeKind::I8:case TypeKind::U8:return 1;
        case TypeKind::I16:case TypeKind::U16:return 2;
        case TypeKind::I32:case TypeKind::U32:return 4;
        case TypeKind::I64:case TypeKind::U64:case TypeKind::Int:case TypeKind::Float:case TypeKind::Isize:case TypeKind::Usize:case TypeKind::Pointer:case TypeKind::String:case TypeKind::Null:case TypeKind::Generic:return 8;
        case TypeKind::Slice:return 16;
        case TypeKind::Array:return element?element->size()*count:0;
        default:return 0;
    }
}
std::size_t Type::alignment()const{if(kind==TypeKind::Slice)return 8;if(kind==TypeKind::Array&&element)return element->alignment();auto s=size();return s?s:1;}
std::string Type::name()const{
    switch(kind){
        case TypeKind::Unknown:return"unknown";case TypeKind::Void:return"void";case TypeKind::Null:return"null";case TypeKind::Bool:return"bool";
        case TypeKind::I8:return"i8";case TypeKind::I16:return"i16";case TypeKind::I32:return"i32";case TypeKind::I64:return"i64";case TypeKind::U8:return"u8";case TypeKind::U16:return"u16";case TypeKind::U32:return"u32";case TypeKind::U64:return"u64";
        case TypeKind::Isize:return"isize";case TypeKind::Usize:return"usize";case TypeKind::Int:return"int";case TypeKind::Float:return"float";case TypeKind::String:return"string";case TypeKind::Record:return recordName;case TypeKind::Generic:return genericName;
        case TypeKind::Pointer:return std::string("*")+(isVolatile?"volatile ":"")+(pointee?pointee->name():"unknown");
        case TypeKind::Slice:return std::string("[]")+(element?element->name():"unknown");
        case TypeKind::Array:return std::string("[")+(element?element->name():"unknown")+";"+std::to_string(count)+"]";
    }
    return"unknown";
}
bool Type::operator==(const Type&o)const{
    if(kind!=o.kind||isVolatile!=o.isVolatile||recordName!=o.recordName||genericName!=o.genericName||count!=o.count)return false;
    if(kind==TypeKind::Pointer){if(!pointee||!o.pointee)return pointee==o.pointee;return*pointee==*o.pointee;}
    if(kind==TypeKind::Array||kind==TypeKind::Slice){if(!element||!o.element)return element==o.element;return*element==*o.element;}
    return true;
}

Type typeFromName(const std::string& raw){
    std::string n=raw;
    if(n.rfind("*",0)==0){n.erase(0,1);bool vol=false;if(n.rfind("volatile ",0)==0){vol=true;n.erase(0,9);}Type p; p.kind=TypeKind::Pointer;p.isVolatile=vol;p.pointee=std::make_shared<Type>(typeFromName(n));return p;}
    if(n.rfind("[]",0)==0){Type s;s.kind=TypeKind::Slice;s.element=std::make_shared<Type>(typeFromName(n.substr(2)));return s;}
    if(n.size()>3&&n.front()=='['&&n.back()==']'){
        auto semi=n.rfind(';');
        if(semi!=std::string::npos){Type a;a.kind=TypeKind::Array;a.element=std::make_shared<Type>(typeFromName(n.substr(1,semi-1)));try{a.count=static_cast<std::size_t>(std::stoull(n.substr(semi+1,n.size()-semi-2),nullptr,0));}catch(...){a.count=0;}return a;}
    }
    if(n=="void")return Type{TypeKind::Void};if(n=="null")return Type{TypeKind::Null};if(n=="bool")return Type{TypeKind::Bool};if(n=="i8")return Type{TypeKind::I8};if(n=="i16")return Type{TypeKind::I16};if(n=="i32")return Type{TypeKind::I32};if(n=="i64")return Type{TypeKind::I64};if(n=="u8")return Type{TypeKind::U8};if(n=="u16")return Type{TypeKind::U16};if(n=="u32")return Type{TypeKind::U32};if(n=="u64")return Type{TypeKind::U64};if(n=="isize")return Type{TypeKind::Isize};if(n=="usize")return Type{TypeKind::Usize};if(n=="int"||n=="int64")return Type{TypeKind::Int};if(n=="float"||n=="float64")return Type{TypeKind::Float};if(n=="string")return Type{TypeKind::String};
    Type r;r.kind=TypeKind::Record;r.recordName=n;return r;
}

namespace {
std::size_t integerBits(TypeKind kind){
    switch(kind){
        case TypeKind::I8:case TypeKind::U8:return 8;
        case TypeKind::I16:case TypeKind::U16:return 16;
        case TypeKind::I32:case TypeKind::U32:return 32;
        case TypeKind::I64:case TypeKind::U64:case TypeKind::Isize:case TypeKind::Usize:case TypeKind::Int:return 64;
        default:return 0;
    }
}
bool integerSigned(TypeKind kind){
    switch(kind){
        case TypeKind::I8:case TypeKind::I16:case TypeKind::I32:case TypeKind::I64:case TypeKind::Isize:case TypeKind::Int:return true;
        default:return false;
    }
}
}

bool canAssign(Type target,Type value){
    if(target.kind==TypeKind::Unknown||value.kind==TypeKind::Unknown||target.kind==TypeKind::Generic||value.kind==TypeKind::Generic)return true;
    if(target==value)return true;
    if(target.isInteger()&&value.isInteger()){
        const auto targetBits=integerBits(target.kind),valueBits=integerBits(value.kind);
        const bool targetSigned=integerSigned(target.kind),valueSigned=integerSigned(value.kind);
        if(targetSigned==valueSigned)return targetBits>=valueBits;
        if(targetSigned&&!valueSigned)return targetBits>valueBits;
        return false;
    }
    if(target.kind==TypeKind::Float&&value.isInteger())return true;
    if(target.kind==TypeKind::Pointer&&value.kind==TypeKind::Null)return true;
    if(target.kind==TypeKind::Pointer&&value.kind==TypeKind::Pointer&&target.pointee&&value.pointee)return target.pointee->kind==TypeKind::Void||value.pointee->kind==TypeKind::Void||*target.pointee==*value.pointee;
    return false;
}

std::string readTextFile(const std::filesystem::path& path){std::ifstream in(path,std::ios::binary);if(!in)throw std::runtime_error("cannot open file: "+path.string());std::ostringstream out;out<<in.rdbuf();return out.str();}

CompileResult compileSource(const std::string& source){
    CompileResult result;
    Lexer lexer(source,result.diagnostics);auto tokens=lexer.lex();if(result.diagnostics.hasErrors())return result;
    Parser parser(std::move(tokens),result.diagnostics);result.ast=parser.parse();if(result.diagnostics.hasErrors())return result;
    TypeChecker checker(result.diagnostics);if(!checker.check(result.ast))return result;
    Lowerer lowerer;result.nir=lowerer.lower(result.ast);Optimizer optimizer;optimizer.optimize(result.nir);return result;
}

CompileResult compileFile(const std::filesystem::path& path){
    CompileResult failed;
    try{
        std::set<std::filesystem::path> visited;
        std::function<std::string(const std::filesystem::path&)> load=[&](const std::filesystem::path& input)->std::string{
            auto full=std::filesystem::weakly_canonical(input);
            if(visited.count(full))return {};
            visited.insert(full);
            auto source=readTextFile(full);
            std::istringstream lines(source);
            std::ostringstream imports,body;
            std::string line;
            std::regex re(R"(^\s*import\s+\"([^\"]+)\"\s*;?\s*$)");
            std::smatch m;
            while(std::getline(lines,line)){
                if(std::regex_match(line,m,re)){
                    std::filesystem::path imported=m[1].str();
                    if(imported.extension().empty())imported += ".nqr";
                    if(imported.is_relative())imported=full.parent_path()/imported;
                    imports<<load(imported)<<'\n';
                    body<<line<<'\n';
                }else body<<line<<'\n';
            }
            return imports.str()+body.str();
        };
        return compileSource(load(path));
    }catch(const std::exception& e){
        failed.diagnostics.error("NQR-M6000",{},e.what(),"check import paths relative to the importing file");
        return failed;
    }
}
} // namespace noe