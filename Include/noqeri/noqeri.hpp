#pragma once
#include "abi.h"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace noe {

inline constexpr const char* NOQERI_LANGUAGE_VERSION = "1.4";
inline constexpr const char* NOQERI_COMPILER_VERSION = "1.4.0-general-systems";
inline constexpr const char* NOQERI_SUPPORTED_TARGET = "cross-platform-frontend;freestanding-x86_64;noqeri-abi-v2";
inline constexpr const char* NOE_LANGUAGE_VERSION = NOQERI_LANGUAGE_VERSION;
inline constexpr const char* NOE_COMPILER_VERSION = NOQERI_COMPILER_VERSION;
inline constexpr const char* NOE_SUPPORTED_TARGET = NOQERI_SUPPORTED_TARGET;

struct Span { std::size_t start=0,end=0,line=1,column=1; };

enum class TokenKind {
    Eof, Identifier, Integer, Float, String,
    Let, Const, Function, If, Else, While, Return, True, False, Null,
    Import, Module, Record, Class, Extern, Export, Volatile, As, Throw, Try,
    LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    Comma, Colon, Semicolon, Dot,
    Plus, Minus, Star, Slash, Percent, Ampersand,
    Bang, BangEqual, Equal, EqualEqual,
    Less, LessEqual, Greater, GreaterEqual,
    AndAnd, OrOr
};

struct Token { TokenKind kind=TokenKind::Eof; std::string lexeme; Span span; };
const char* tokenKindName(TokenKind kind);

struct Diagnostic { std::string code,message; Span span; std::string help; };
class Diagnostics {
public:
    void error(std::string code,Span span,std::string message,std::string help={});
    bool hasErrors() const { return !items_.empty(); }
    const std::vector<Diagnostic>& items() const { return items_; }
    void print(const std::string& sourceName) const;
private:
    std::vector<Diagnostic> items_;
};

using Literal = std::variant<std::monostate,std::int64_t,double,bool,std::string>;

struct Expr { virtual ~Expr()=default; Span span; };
using ExprPtr = std::shared_ptr<Expr>;
struct LiteralExpr final : Expr { Literal value; };
struct NameExpr final : Expr { std::string name; };
struct UnaryExpr final : Expr { TokenKind op; ExprPtr operand; std::size_t memoryWidth=8; bool volatileAccess=false; };
struct BinaryExpr final : Expr { ExprPtr left; TokenKind op; ExprPtr right; };
struct CallExpr final : Expr {
    ExprPtr callee;
    std::vector<ExprPtr> args;
    std::size_t builtinWidth=8;
    std::size_t builtinCount=0;
    std::string builtinText;
};
struct CastExpr final : Expr { ExprPtr value; std::string typeName; };
struct IndexExpr final : Expr { ExprPtr object,index; std::size_t elementSize=1; bool volatileAccess=false; bool baseIsSlice=false; };
struct MemberExpr final : Expr { ExprPtr object; std::string member; std::size_t offset=0; std::size_t fieldSize=0; bool volatileAccess=false; bool baseIsPointer=false; };
struct ArrayExpr final : Expr { std::vector<ExprPtr> elements; std::size_t elementSize=0; };

struct Stmt { virtual ~Stmt()=default; Span span; };
using StmtPtr = std::shared_ptr<Stmt>;
struct ExprStmt final : Stmt { ExprPtr expr; };
struct LetStmt final : Stmt { bool isConst=false; std::string name; std::optional<std::string> annotation; ExprPtr initializer; };
struct BlockStmt final : Stmt { std::vector<StmtPtr> statements; };
struct IfStmt final : Stmt { ExprPtr condition; StmtPtr thenBranch; StmtPtr elseBranch; };
struct WhileStmt final : Stmt { ExprPtr condition; StmtPtr body; };
struct ReturnStmt final : Stmt { ExprPtr value; };
struct ThrowStmt final : Stmt { ExprPtr value; };
struct Parameter { std::string name; std::optional<std::string> annotation; Span span; };
struct FunctionStmt final : Stmt {
    std::string name;
    std::vector<std::string> genericParams;
    std::vector<Parameter> params;
    std::optional<std::string> returnType;
    std::shared_ptr<BlockStmt> body;
    bool isExtern=false;
    bool isExport=false;
};
struct RecordField { std::string name,typeName; Span span; std::size_t offset=0,size=0; };
struct RecordStmt final : Stmt { std::string name; std::vector<RecordField> fields; std::size_t size=0,alignment=1; };
struct ImportStmt final : Stmt { std::string path; };
struct ModuleStmt final : Stmt { std::string name; };
struct Program { std::vector<StmtPtr> statements; };

enum class TypeKind {
    Unknown,Void,Null,Bool,
    I8,I16,I32,I64,U8,U16,U32,U64,Isize,Usize,Int,Float,String,
    Pointer,Record,Array,Slice,Generic
};
struct Type {
    TypeKind kind=TypeKind::Unknown;
    std::string recordName;
    std::string genericName;
    std::shared_ptr<Type> pointee;
    std::shared_ptr<Type> element;
    std::size_t count=0;
    bool isVolatile=false;
    std::string name() const;
    bool isInteger() const;
    bool isNumeric() const { return isInteger() || kind==TypeKind::Float; }
    bool isPointer() const { return kind==TypeKind::Pointer; }
    bool isAggregate() const { return kind==TypeKind::Array || kind==TypeKind::Slice || kind==TypeKind::Record; }
    std::size_t size() const;
    std::size_t alignment() const;
    bool operator==(const Type& o) const;
    bool operator!=(const Type& o) const { return !(*this==o); }
};
Type typeFromName(const std::string& name);
bool canAssign(Type target,Type value);

using NirValue = std::variant<std::monostate,std::int64_t,double,bool,std::string>;
using Reg = std::uint32_t;
enum class NirOp {
    Const,Load,Store,Unary,Binary,Cast,Call,
    AddressOf,LoadMemory,StoreMemory,PtrOffset,StackAlloc,
    MakeSlice,SliceData,SliceLen,
    AtomicLoad,AtomicStore,AtomicExchange,AtomicCompareExchange,AtomicFence,
    Intrinsic,InlineAsm,Try,Throw,
    Jump,JumpIfFalse,Return,Nop
};
struct NirInstruction {
    NirOp op=NirOp::Nop;
    std::optional<Reg> dest;
    std::string text;
    NirValue literal;
    std::vector<Reg> args;
    std::size_t target=0;
    std::size_t width=8;
    bool isVolatile=false;
};
struct NirFunction {
    std::string name;
    std::vector<std::string> genericParams;
    std::vector<std::string> params;
    std::vector<NirInstruction> code;
    Reg nextReg=0;
    bool isExtern=false;
    bool isExport=false;
};
struct NirProgram { NirFunction entry; std::vector<NirFunction> functions; };
std::string printNir(const NirProgram& program);

class Lexer {
public:
    Lexer(std::string source,Diagnostics& diagnostics);
    std::vector<Token> lex();
private:
    char peek(std::size_t offset=0) const;
    bool match(char expected);
    char advance();
    void skipWhitespaceAndComments();
    Token scanToken();
    Token identifier();
    Token number();
    Token stringLiteral();
    Token make(TokenKind kind,std::size_t start,std::size_t line,std::size_t col);
    std::string source_;
    Diagnostics& diagnostics_;
    std::size_t current_=0,line_=1,column_=1;
};

class Parser {
public:
    Parser(std::vector<Token> tokens,Diagnostics& diagnostics);
    Program parse();
private:
    StmtPtr declaration();
    StmtPtr functionDeclaration(bool isExtern=false,bool isExport=false);
    StmtPtr recordDeclaration();
    StmtPtr importDeclaration();
    StmtPtr moduleDeclaration();
    StmtPtr letDeclaration(bool isConst);
    StmtPtr statement();
    StmtPtr ifStatement();
    StmtPtr whileStatement();
    StmtPtr returnStatement();
    StmtPtr throwStatement();
    std::shared_ptr<BlockStmt> block();
    StmtPtr expressionStatement();
    ExprPtr expression();
    ExprPtr assignment();
    ExprPtr logicalOr();
    ExprPtr logicalAnd();
    ExprPtr equality();
    ExprPtr comparison();
    ExprPtr term();
    ExprPtr factor();
    ExprPtr unary();
    ExprPtr cast();
    ExprPtr call();
    ExprPtr primary();
    std::string parseTypeName();
    std::vector<std::string> parseGenericParams();
    bool match(std::initializer_list<TokenKind> kinds);
    bool check(TokenKind kind) const;
    const Token& advance();
    const Token& previous() const;
    const Token& peek() const;
    const Token& consume(TokenKind kind,const std::string& message);
    void synchronize();
    std::vector<Token> tokens_;
    Diagnostics& diagnostics_;
    std::size_t current_=0;
};

struct FunctionType {
    std::vector<Type> params;
    Type result;
    std::vector<std::string> genericParams;
    bool isExtern=false;
    bool isExport=false;
};
struct RecordType { std::vector<RecordField> fields; std::size_t size=0,alignment=1; };
class TypeChecker {
public:
    explicit TypeChecker(Diagnostics& diagnostics);
    bool check(const Program& program);
private:
    void checkStmt(const StmtPtr& stmt);
    Type checkExpr(const ExprPtr& expr);
    void pushScope();
    void popScope();
    void define(const std::string& name,Type type,bool isConst,Span span);
    std::optional<Type> resolve(const std::string& name) const;
    bool isConstSymbol(const std::string& name) const;
    Type resolveType(const std::string& name,Span span);
    std::optional<RecordField> resolveField(const Type& base,const std::string& member) const;
    Type substituteGeneric(const Type& type,const std::unordered_map<std::string,Type>& bindings) const;
    bool bindGeneric(const Type& pattern,const Type& actual,std::unordered_map<std::string,Type>& bindings) const;
    Diagnostics& diagnostics_;
    std::vector<std::unordered_map<std::string,Type>> scopes_;
    std::vector<std::unordered_map<std::string,bool>> constScopes_;
    std::unordered_map<std::string,FunctionType> functions_;
    std::unordered_map<std::string,RecordType> records_;
    std::unordered_set<std::string> activeGenericParams_;
    Type currentReturn_{};
    bool insideFunction_=false;
};

class Lowerer {
public:
    NirProgram lower(const Program& program);
private:
    void lowerStmt(NirFunction& fn,const StmtPtr& stmt);
    Reg lowerExpr(NirFunction& fn,const ExprPtr& expr);
    Reg lowerAddress(NirFunction& fn,const ExprPtr& expr);
    Reg emit(NirFunction& fn,NirInstruction instruction);
};
class Optimizer { public: void optimize(NirProgram& program) const; private: void optimizeFunction(NirFunction& fn) const; };

const noqeri_abi* defaultNoqeriAbi();
std::optional<NirValue> callNoqeriAbi(const noqeri_abi* abi,const std::string& name,const std::vector<NirValue>& args,std::string& error);
const noqeri_abi* defaultHostApi();
std::optional<NirValue> callAbiFunction(const noqeri_abi* abi,const std::string& name,const std::vector<NirValue>& args,std::string& error);
class Interpreter {
public:
    explicit Interpreter(const noqeri_abi* abi=nullptr):host_(abi){}
    int run(const NirProgram& program);
private:
    NirValue runFunction(const NirProgram& program,const NirFunction& fn,const std::vector<NirValue>& args);
    std::string valueToString(const NirValue& value) const;
    const noqeri_abi* host_=nullptr;
};
class NativeBackend { public: bool emitAssembly(const NirProgram& program,const std::filesystem::path& output,Diagnostics& diagnostics) const; };
class LinkerDriver { public: bool link(const std::filesystem::path& assembly,const std::filesystem::path& output,Diagnostics& diagnostics) const; };

struct ProjectManifest { std::string name="app",version="0.0.0",entry="src/main.nqr",profile="app",target="freestanding-x86_64"; std::unordered_map<std::string,std::string> dependencies; };
class PackageManager { public: std::optional<ProjectManifest> loadManifest(const std::filesystem::path& path,Diagnostics& diagnostics) const; bool writeLock(const ProjectManifest& manifest,const std::filesystem::path& path,Diagnostics& diagnostics) const; bool createProject(const std::filesystem::path& directory,const std::string& name,Diagnostics& diagnostics) const; };
class Formatter { public: std::string format(const std::string& source,Diagnostics& diagnostics) const; };
class TestRunner { public: int runDirectory(const std::filesystem::path& dir) const; };
class LanguageServer { public: int run(); };
struct CompileResult { Program ast; NirProgram nir; Diagnostics diagnostics; };
CompileResult compileSource(const std::string& source);
CompileResult compileFile(const std::filesystem::path& path);
std::string readTextFile(const std::filesystem::path& path);
int runProductionDoctor(const std::filesystem::path& root);

} // namespace noe
namespace noqeri = noe;
