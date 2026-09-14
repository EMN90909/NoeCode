#include "noe.hpp"
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace noe {
namespace {

std::string calleeName(const ExprPtr& expr) {
    if (auto n = std::dynamic_pointer_cast<NameExpr>(expr)) return n->name;
    if (auto m = std::dynamic_pointer_cast<MemberExpr>(expr)) {
        if (auto n = std::dynamic_pointer_cast<NameExpr>(m->object)) return n->name + "." + m->member;
    }
    return {};
}

struct EvalResult {
    bool ok = false;
    Literal value{};
};

struct ExecResult {
    bool ok = true;
    bool returned = false;
    Literal value{};
};

class ComptimeEngine {
public:
    ComptimeEngine(Program& program, Diagnostics& diagnostics) : program_(program), diagnostics_(diagnostics) {
        for (const auto& statement : program_.statements) {
            if (auto function = std::dynamic_pointer_cast<FunctionStmt>(statement); function && !function->isExtern) functions_[function->name] = function;
        }
        scopes_.push_back({});
    }

    bool run() {
        for (auto& statement : program_.statements) transformStatement(statement);
        return !diagnostics_.hasErrors();
    }

private:
    using Scope = std::unordered_map<std::string, Literal>;

    void pushScope() { scopes_.push_back({}); }
    void popScope() { if (scopes_.size() > 1) scopes_.pop_back(); }

    std::optional<Literal> lookup(const std::string& name) const {
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) return found->second;
        }
        return std::nullopt;
    }

    void bind(const std::string& name, const Literal& value) { scopes_.back()[name] = value; }

    bool assign(const std::string& name, const Literal& value) {
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) { found->second = value; return true; }
        }
        return false;
    }

    void error(Span span, const std::string& message, const std::string& help = {}) {
        diagnostics_.error("NQR-C5001", span, message, help);
    }

    static bool asBool(const Literal& value, bool& out) {
        if (auto p = std::get_if<bool>(&value)) { out = *p; return true; }
        return false;
    }

    static bool addChecked(std::int64_t a, std::int64_t b, std::int64_t& out) {
        if ((b > 0 && a > std::numeric_limits<std::int64_t>::max() - b) ||
            (b < 0 && a < std::numeric_limits<std::int64_t>::min() - b)) return false;
        out = a + b; return true;
    }
    static bool subChecked(std::int64_t a, std::int64_t b, std::int64_t& out) {
        if ((b < 0 && a > std::numeric_limits<std::int64_t>::max() + b) ||
            (b > 0 && a < std::numeric_limits<std::int64_t>::min() + b)) return false;
        out = a - b; return true;
    }
    static bool mulChecked(std::int64_t a, std::int64_t b, std::int64_t& out) {
        if (a == 0 || b == 0) { out = 0; return true; }
        if ((a == -1 && b == std::numeric_limits<std::int64_t>::min()) ||
            (b == -1 && a == std::numeric_limits<std::int64_t>::min())) return false;
        const auto candidate = a * b;
        if (candidate / b != a) return false;
        out = candidate; return true;
    }

    EvalResult evalUnary(TokenKind op, const Literal& value, Span span) {
        if (auto integer = std::get_if<std::int64_t>(&value)) {
            if (op == TokenKind::Plus) return {true, *integer};
            if (op == TokenKind::Minus) {
                if (*integer == std::numeric_limits<std::int64_t>::min()) { error(span, "compile-time integer negation overflows"); return {}; }
                return {true, -*integer};
            }
        }
        if (auto number = std::get_if<double>(&value)) {
            if (op == TokenKind::Plus) return {true, *number};
            if (op == TokenKind::Minus) return {true, -*number};
        }
        if (auto boolean = std::get_if<bool>(&value); boolean && op == TokenKind::Bang) return {true, !*boolean};
        error(span, "operator is not supported during compile-time execution");
        return {};
    }

    EvalResult evalBinary(TokenKind op, const Literal& left, const Literal& right, Span span) {
        if (auto a = std::get_if<std::int64_t>(&left)) {
            if (auto b = std::get_if<std::int64_t>(&right)) {
                std::int64_t value = 0;
                switch (op) {
                    case TokenKind::Plus: if (!addChecked(*a,*b,value)) { error(span,"compile-time integer addition overflows"); return {}; } return {true,value};
                    case TokenKind::Minus: if (!subChecked(*a,*b,value)) { error(span,"compile-time integer subtraction overflows"); return {}; } return {true,value};
                    case TokenKind::Star: if (!mulChecked(*a,*b,value)) { error(span,"compile-time integer multiplication overflows"); return {}; } return {true,value};
                    case TokenKind::Slash: if (*b == 0) { error(span,"division by zero during compile-time execution"); return {}; } if (*a == std::numeric_limits<std::int64_t>::min() && *b == -1) { error(span,"compile-time integer division overflows"); return {}; } return {true,*a / *b};
                    case TokenKind::Percent: if (*b == 0) { error(span,"remainder by zero during compile-time execution"); return {}; } return {true,*a % *b};
                    case TokenKind::Less: return {true,*a < *b}; case TokenKind::LessEqual: return {true,*a <= *b};
                    case TokenKind::Greater: return {true,*a > *b}; case TokenKind::GreaterEqual: return {true,*a >= *b};
                    case TokenKind::EqualEqual: return {true,*a == *b}; case TokenKind::BangEqual: return {true,*a != *b};
                    default: break;
                }
            }
        }
        if (auto a = std::get_if<double>(&left)) {
            if (auto b = std::get_if<double>(&right)) {
                switch (op) {
                    case TokenKind::Plus:return {true,*a+*b}; case TokenKind::Minus:return {true,*a-*b}; case TokenKind::Star:return {true,*a**b};
                    case TokenKind::Slash: if (*b == 0.0) { error(span,"division by zero during compile-time execution"); return {}; } return {true,*a / *b};
                    case TokenKind::Less:return {true,*a<*b}; case TokenKind::LessEqual:return {true,*a<=*b}; case TokenKind::Greater:return {true,*a>*b}; case TokenKind::GreaterEqual:return {true,*a>=*b};
                    case TokenKind::EqualEqual:return {true,*a==*b}; case TokenKind::BangEqual:return {true,*a!=*b}; default:break;
                }
            }
        }
        if (auto a = std::get_if<bool>(&left)) {
            if (auto b = std::get_if<bool>(&right)) {
                switch (op) { case TokenKind::AndAnd:return {true,*a&&*b}; case TokenKind::OrOr:return {true,*a||*b}; case TokenKind::EqualEqual:return {true,*a==*b}; case TokenKind::BangEqual:return {true,*a!=*b}; default:break; }
            }
        }
        if (auto a = std::get_if<std::string>(&left)) {
            if (auto b = std::get_if<std::string>(&right)) {
                if (op == TokenKind::Plus) return {true,*a + *b};
                if (op == TokenKind::EqualEqual) return {true,*a == *b};
                if (op == TokenKind::BangEqual) return {true,*a != *b};
            }
        }
        error(span, "operand types are not supported during compile-time execution");
        return {};
    }

    EvalResult evalCall(const std::shared_ptr<CallExpr>& call, int depth) {
        if (++steps_ > stepLimit_) { error(call->span,"compile-time execution exceeded its step budget","simplify the computation or split it into smaller compile-time calls"); return {}; }
        const auto name = calleeName(call->callee);
        if (name == "comptime") {
            if (call->args.size() != 1) { error(call->span,"comptime expects exactly one expression"); return {}; }
            return evalExpr(call->args[0], depth + 1);
        }
        if (name == "len" && call->args.size() == 1) {
            if (auto array = std::dynamic_pointer_cast<ArrayExpr>(call->args[0])) return {true, static_cast<std::int64_t>(array->elements.size())};
            auto value = evalExpr(call->args[0], depth + 1);
            if (value.ok) if (auto text = std::get_if<std::string>(&value.value)) return {true, static_cast<std::int64_t>(text->size())};
            error(call->span,"compile-time len expects an array literal or constant string"); return {};
        }
        auto functionIt = functions_.find(name);
        if (functionIt == functions_.end()) { error(call->span,"call to '"+name+"' is not available during compile-time execution","compile-time code may call pure Noqeri functions but not host, I/O, allocator or runtime services"); return {}; }
        if (depth >= recursionLimit_) { error(call->span,"compile-time recursion limit exceeded"); return {}; }
        const auto& function = functionIt->second;
        if (call->args.size() != function->params.size()) { error(call->span,"compile-time call has the wrong argument count"); return {}; }
        std::vector<Literal> args;
        for (const auto& argument : call->args) { auto value = evalExpr(argument, depth + 1); if (!value.ok) return {}; args.push_back(value.value); }
        pushScope();
        for (std::size_t i=0;i<args.size();++i) bind(function->params[i].name,args[i]);
        auto result = execBlock(function->body, depth + 1);
        popScope();
        if (!result.ok || !result.returned) { if (result.ok) error(call->span,"compile-time function did not return a value"); return {}; }
        return {true,result.value};
    }

    EvalResult evalExpr(const ExprPtr& expression, int depth = 0) {
        if (!expression) return {};
        if (++steps_ > stepLimit_) { error(expression->span,"compile-time execution exceeded its step budget"); return {}; }
        if (auto literal = std::dynamic_pointer_cast<LiteralExpr>(expression)) return {true,literal->value};
        if (auto name = std::dynamic_pointer_cast<NameExpr>(expression)) {
            auto value = lookup(name->name); if (value) return {true,*value};
            error(name->span,"'"+name->name+"' is not known at compile time","use const data or pass a constant argument to a pure compile-time function"); return {};
        }
        if (auto unary = std::dynamic_pointer_cast<UnaryExpr>(expression)) { auto value=evalExpr(unary->operand,depth); return value.ok?evalUnary(unary->op,value.value,unary->span):EvalResult{}; }
        if (auto binary = std::dynamic_pointer_cast<BinaryExpr>(expression)) {
            if (binary->op == TokenKind::Equal) {
                auto name = std::dynamic_pointer_cast<NameExpr>(binary->left); if (!name) { error(binary->span,"compile-time assignment requires a named local"); return {}; }
                auto value = evalExpr(binary->right,depth); if (!value.ok) return {};
                if (!assign(name->name,value.value)) { error(name->span,"cannot assign unknown compile-time local '"+name->name+"'"); return {}; }
                return value;
            }
            auto left=evalExpr(binary->left,depth); if(!left.ok)return{};
            if (binary->op==TokenKind::AndAnd) { bool b=false; if(!asBool(left.value,b)){error(binary->left->span,"compile-time && requires bool");return{};} if(!b)return{true,false}; }
            if (binary->op==TokenKind::OrOr) { bool b=false; if(!asBool(left.value,b)){error(binary->left->span,"compile-time || requires bool");return{};} if(b)return{true,true}; }
            auto right=evalExpr(binary->right,depth); return right.ok?evalBinary(binary->op,left.value,right.value,binary->span):EvalResult{};
        }
        if (auto cast = std::dynamic_pointer_cast<CastExpr>(expression)) {
            auto value=evalExpr(cast->value,depth); if(!value.ok)return{};
            if (auto integer=std::get_if<std::int64_t>(&value.value)) {
                if (cast->typeName=="int"||cast->typeName=="i64"||cast->typeName=="isize"||cast->typeName=="usize"||cast->typeName=="u64"||cast->typeName=="i32"||cast->typeName=="u32"||cast->typeName=="i16"||cast->typeName=="u16"||cast->typeName=="i8"||cast->typeName=="u8") return value;
                if (cast->typeName=="float"||cast->typeName=="float64") return {true,static_cast<double>(*integer)};
            }
            if (auto number=std::get_if<double>(&value.value); number&&(cast->typeName=="int"||cast->typeName=="i64")) return {true,static_cast<std::int64_t>(*number)};
            error(cast->span,"cast is not supported during compile-time execution"); return{};
        }
        if (auto call = std::dynamic_pointer_cast<CallExpr>(expression)) return evalCall(call,depth);
        error(expression->span,"expression is not executable at compile time");
        return {};
    }

    ExecResult execStatement(const StmtPtr& statement, int depth) {
        if (!statement) return {};
        if (++steps_ > stepLimit_) { error(statement->span,"compile-time execution exceeded its step budget"); return {false,false,{}}; }
        if (auto let=std::dynamic_pointer_cast<LetStmt>(statement)) { auto value=evalExpr(let->initializer,depth); if(!value.ok)return{false,false,{}}; bind(let->name,value.value); return{}; }
        if (auto expr=std::dynamic_pointer_cast<ExprStmt>(statement)) { auto value=evalExpr(expr->expr,depth); return {value.ok,false,{}}; }
        if (auto returned=std::dynamic_pointer_cast<ReturnStmt>(statement)) { auto value=evalExpr(returned->value,depth); return {value.ok,value.ok,value.value}; }
        if (auto block=std::dynamic_pointer_cast<BlockStmt>(statement)) return execBlock(block,depth+1);
        if (auto branch=std::dynamic_pointer_cast<IfStmt>(statement)) {
            auto condition=evalExpr(branch->condition,depth); if(!condition.ok)return{false,false,{}}; bool yes=false;if(!asBool(condition.value,yes)){error(branch->condition->span,"compile-time if condition must be bool");return{false,false,{}};}
            if (yes) return execStatement(branch->thenBranch,depth+1); if(branch->elseBranch)return execStatement(branch->elseBranch,depth+1); return{};
        }
        if (auto loop=std::dynamic_pointer_cast<WhileStmt>(statement)) {
            while (true) { if(++steps_>stepLimit_){error(loop->span,"compile-time loop exceeded its step budget");return{false,false,{}};} auto condition=evalExpr(loop->condition,depth);if(!condition.ok)return{false,false,{}};bool yes=false;if(!asBool(condition.value,yes)){error(loop->condition->span,"compile-time while condition must be bool");return{false,false,{}};}if(!yes)return{};auto body=execStatement(loop->body,depth+1);if(!body.ok||body.returned)return body; }
        }
        error(statement->span,"statement is not supported during compile-time execution"); return{false,false,{}};
    }

    ExecResult execBlock(const std::shared_ptr<BlockStmt>& block, int depth) {
        if (!block) return {};
        pushScope();
        for (const auto& statement : block->statements) { auto result=execStatement(statement,depth); if(!result.ok||result.returned){popScope();return result;} }
        popScope(); return{};
    }

    ExprPtr transformExpr(ExprPtr expression) {
        if (!expression) return expression;
        if (auto unary=std::dynamic_pointer_cast<UnaryExpr>(expression)) unary->operand=transformExpr(unary->operand);
        else if (auto binary=std::dynamic_pointer_cast<BinaryExpr>(expression)) { binary->left=transformExpr(binary->left); binary->right=transformExpr(binary->right); }
        else if (auto call=std::dynamic_pointer_cast<CallExpr>(expression)) {
            for(auto&arg:call->args)arg=transformExpr(arg);
            if(calleeName(call->callee)=="comptime") {
                steps_=0;auto result=evalCall(call,0);if(!result.ok)return expression;
                auto literal=std::make_shared<LiteralExpr>();literal->span=call->span;literal->value=result.value;return literal;
            }
        } else if(auto cast=std::dynamic_pointer_cast<CastExpr>(expression))cast->value=transformExpr(cast->value);
        else if(auto index=std::dynamic_pointer_cast<IndexExpr>(expression)){index->object=transformExpr(index->object);index->index=transformExpr(index->index);}
        else if(auto member=std::dynamic_pointer_cast<MemberExpr>(expression))member->object=transformExpr(member->object);
        else if(auto array=std::dynamic_pointer_cast<ArrayExpr>(expression))for(auto&item:array->elements)item=transformExpr(item);
        return expression;
    }

    void transformStatement(StmtPtr& statement) {
        if (!statement) return;
        if(auto let=std::dynamic_pointer_cast<LetStmt>(statement)) {
            let->initializer=transformExpr(let->initializer);
            if(let->isConst)if(auto literal=std::dynamic_pointer_cast<LiteralExpr>(let->initializer))bind(let->name,literal->value);
            return;
        }
        if(auto expr=std::dynamic_pointer_cast<ExprStmt>(statement)){expr->expr=transformExpr(expr->expr);return;}
        if(auto returned=std::dynamic_pointer_cast<ReturnStmt>(statement)){returned->value=transformExpr(returned->value);return;}
        if(auto thrown=std::dynamic_pointer_cast<ThrowStmt>(statement)){thrown->value=transformExpr(thrown->value);return;}
        if(auto block=std::dynamic_pointer_cast<BlockStmt>(statement)){pushScope();for(auto&child:block->statements)transformStatement(child);popScope();return;}
        if(auto branch=std::dynamic_pointer_cast<IfStmt>(statement)){branch->condition=transformExpr(branch->condition);pushScope();transformStatement(branch->thenBranch);popScope();if(branch->elseBranch){pushScope();transformStatement(branch->elseBranch);popScope();}return;}
        if(auto loop=std::dynamic_pointer_cast<WhileStmt>(statement)){loop->condition=transformExpr(loop->condition);pushScope();transformStatement(loop->body);popScope();return;}
        if(auto function=std::dynamic_pointer_cast<FunctionStmt>(statement)) {
            if(function->isExtern||!function->body)return;
            pushScope();
            for(auto&child:function->body->statements)transformStatement(child);
            popScope();
        }
    }

    Program& program_;
    Diagnostics& diagnostics_;
    std::unordered_map<std::string,std::shared_ptr<FunctionStmt>> functions_;
    std::vector<Scope> scopes_;
    std::size_t steps_=0;
    static constexpr std::size_t stepLimit_=100000;
    static constexpr int recursionLimit_=64;
};

} // namespace

bool ComptimeEvaluator::evaluate(Program& program, Diagnostics& diagnostics) const {
    ComptimeEngine engine(program,diagnostics);
    return engine.run();
}

} // namespace noe
