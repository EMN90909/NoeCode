#include "noe.hpp"
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace noe {
namespace {

std::string ctCallName(const ExprPtr& expr) {
    if (auto name = std::dynamic_pointer_cast<NameExpr>(expr)) return name->name;
    if (auto member = std::dynamic_pointer_cast<MemberExpr>(expr)) {
        if (auto base = std::dynamic_pointer_cast<NameExpr>(member->object)) return base->name + "." + member->member;
    }
    return {};
}

struct CtFlow {
    bool ok = true;
    bool returned = false;
    Literal value{};
};

class CtEngine {
public:
    explicit CtEngine(Diagnostics& diagnostics) : diagnostics_(diagnostics) {}

    bool run(Program& program) {
        for (const auto& stmt : program.statements) {
            if (auto function = std::dynamic_pointer_cast<FunctionStmt>(stmt)) functions_[function->name] = function;
        }
        for (auto& stmt : program.statements) transformStmt(stmt, true);
        return !diagnostics_.hasErrors();
    }

private:
    Diagnostics& diagnostics_;
    std::unordered_map<std::string, std::shared_ptr<FunctionStmt>> functions_;
    std::unordered_map<std::string, Literal> globals_;
    std::vector<std::unordered_map<std::string, Literal>> scopes_;
    std::size_t steps_ = 0;
    std::size_t callDepth_ = 0;
    static constexpr std::size_t stepLimit_ = 100000;
    static constexpr std::size_t callDepthLimit_ = 128;

    bool step(Span span) {
        if (++steps_ <= stepLimit_) return true;
        diagnostics_.error("NQR-C5101", span, "compile-time execution exceeded its 100000-step budget",
                           "reduce the compile-time workload or move this computation to runtime");
        return false;
    }

    static bool truthy(const Literal& value, bool& out) {
        if (auto v = std::get_if<bool>(&value)) { out = *v; return true; }
        return false;
    }

    static bool numeric(const Literal& value, long double& out, bool& integral) {
        if (auto v = std::get_if<std::int64_t>(&value)) { out = static_cast<long double>(*v); integral = true; return true; }
        if (auto v = std::get_if<double>(&value)) { out = static_cast<long double>(*v); integral = false; return true; }
        return false;
    }

    static bool checkedAdd(std::int64_t a, std::int64_t b, std::int64_t& out) {
        if ((b > 0 && a > std::numeric_limits<std::int64_t>::max() - b) ||
            (b < 0 && a < std::numeric_limits<std::int64_t>::min() - b)) return false;
        out = a + b; return true;
    }

    static bool checkedSub(std::int64_t a, std::int64_t b, std::int64_t& out) {
        if ((b < 0 && a > std::numeric_limits<std::int64_t>::max() + b) ||
            (b > 0 && a < std::numeric_limits<std::int64_t>::min() + b)) return false;
        out = a - b; return true;
    }

    static bool checkedMul(std::int64_t a, std::int64_t b, std::int64_t& out) {
        if (a == 0 || b == 0) { out = 0; return true; }
        if (a == -1 && b == std::numeric_limits<std::int64_t>::min()) return false;
        if (b == -1 && a == std::numeric_limits<std::int64_t>::min()) return false;
        if (a > 0) {
            if (b > 0) { if (a > std::numeric_limits<std::int64_t>::max() / b) return false; }
            else { if (b < std::numeric_limits<std::int64_t>::min() / a) return false; }
        } else {
            if (b > 0) { if (a < std::numeric_limits<std::int64_t>::min() / b) return false; }
            else if (a < std::numeric_limits<std::int64_t>::max() / b) return false;
        }
        out = a * b; return true;
    }

    std::optional<Literal> lookup(const std::string& name) const {
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) return found->second;
        }
        auto global = globals_.find(name);
        if (global != globals_.end()) return global->second;
        return std::nullopt;
    }

    bool assign(const std::string& name, Literal value) {
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) { found->second = std::move(value); return true; }
        }
        return false;
    }

    void ctError(Span span, std::string message, std::string help = {}) {
        diagnostics_.error("NQR-C5100", span, std::move(message), std::move(help));
    }

    std::optional<Literal> evalBinary(TokenKind op, const Literal& left, const Literal& right, Span span) {
        if (op == TokenKind::EqualEqual || op == TokenKind::BangEqual) {
            bool equal = false;
            long double a = 0, b = 0; bool ai = false, bi = false;
            if (numeric(left, a, ai) && numeric(right, b, bi)) equal = a == b;
            else if (left.index() == right.index()) equal = left == right;
            return Literal{op == TokenKind::EqualEqual ? equal : !equal};
        }
        if (op == TokenKind::AndAnd || op == TokenKind::OrOr) {
            bool a = false, b = false;
            if (!truthy(left, a) || !truthy(right, b)) { ctError(span, "compile-time logical operators require bool operands"); return std::nullopt; }
            return Literal{op == TokenKind::AndAnd ? (a && b) : (a || b)};
        }
        if (op == TokenKind::Plus) {
            if (auto a = std::get_if<std::string>(&left)) {
                if (auto b = std::get_if<std::string>(&right)) return Literal{*a + *b};
            }
        }
        long double a = 0, b = 0; bool ai = false, bi = false;
        if (!numeric(left, a, ai) || !numeric(right, b, bi)) {
            ctError(span, "unsupported operands in compile-time expression"); return std::nullopt;
        }
        if (op == TokenKind::Less) return Literal{a < b};
        if (op == TokenKind::LessEqual) return Literal{a <= b};
        if (op == TokenKind::Greater) return Literal{a > b};
        if (op == TokenKind::GreaterEqual) return Literal{a >= b};
        if (ai && bi) {
            const auto x = std::get<std::int64_t>(left), y = std::get<std::int64_t>(right);
            std::int64_t out = 0;
            if (op == TokenKind::Plus && checkedAdd(x, y, out)) return Literal{out};
            if (op == TokenKind::Minus && checkedSub(x, y, out)) return Literal{out};
            if (op == TokenKind::Star && checkedMul(x, y, out)) return Literal{out};
            if (op == TokenKind::Slash) {
                if (y == 0) { ctError(span, "division by zero during compile-time execution"); return std::nullopt; }
                if (x == std::numeric_limits<std::int64_t>::min() && y == -1) { ctError(span, "integer overflow during compile-time division"); return std::nullopt; }
                return Literal{x / y};
            }
            if (op == TokenKind::Percent) {
                if (y == 0) { ctError(span, "remainder by zero during compile-time execution"); return std::nullopt; }
                return Literal{x % y};
            }
            if (op == TokenKind::Plus || op == TokenKind::Minus || op == TokenKind::Star) {
                ctError(span, "integer overflow during compile-time execution"); return std::nullopt;
            }
        }
        const double x = static_cast<double>(a), y = static_cast<double>(b);
        if (op == TokenKind::Plus) return Literal{x + y};
        if (op == TokenKind::Minus) return Literal{x - y};
        if (op == TokenKind::Star) return Literal{x * y};
        if (op == TokenKind::Slash) { if (y == 0.0) { ctError(span, "division by zero during compile-time execution"); return std::nullopt; } return Literal{x / y}; }
        if (op == TokenKind::Percent) { if (y == 0.0) { ctError(span, "remainder by zero during compile-time execution"); return std::nullopt; } return Literal{std::fmod(x, y)}; }
        ctError(span, "operator is not supported by compile-time execution");
        return std::nullopt;
    }

    std::optional<Literal> evalExpr(const ExprPtr& expr) {
        if (!expr || !step(expr ? expr->span : Span{})) return std::nullopt;
        if (auto literal = std::dynamic_pointer_cast<LiteralExpr>(expr)) return literal->value;
        if (auto name = std::dynamic_pointer_cast<NameExpr>(expr)) {
            auto value = lookup(name->name);
            if (!value) ctError(name->span, "compile-time expression depends on runtime value '" + name->name + "'",
                                "use a const value or pass the value as a parameter to a compile-time function");
            return value;
        }
        if (auto unary = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
            if (unary->op == TokenKind::Ampersand || unary->op == TokenKind::Star || unary->op == TokenKind::Try) {
                ctError(unary->span, "pointers, dereference and try are not allowed in deterministic compile-time execution");
                return std::nullopt;
            }
            auto value = evalExpr(unary->operand); if (!value) return std::nullopt;
            if (unary->op == TokenKind::Bang) { bool b = false; if (!truthy(*value, b)) { ctError(unary->span, "compile-time '!' requires bool"); return std::nullopt; } return Literal{!b}; }
            if (auto integer = std::get_if<std::int64_t>(&*value)) {
                if (unary->op == TokenKind::Plus) return *value;
                if (unary->op == TokenKind::Minus) { if (*integer == std::numeric_limits<std::int64_t>::min()) { ctError(unary->span, "integer overflow during compile-time negation"); return std::nullopt; } return Literal{-*integer}; }
            }
            if (auto number = std::get_if<double>(&*value)) {
                if (unary->op == TokenKind::Plus) return *value;
                if (unary->op == TokenKind::Minus) return Literal{-*number};
            }
            ctError(unary->span, "unsupported unary compile-time operation"); return std::nullopt;
        }
        if (auto binary = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
            if (binary->op == TokenKind::Equal) {
                auto name = std::dynamic_pointer_cast<NameExpr>(binary->left);
                if (!name) { ctError(binary->span, "compile-time assignment requires a local name"); return std::nullopt; }
                auto value = evalExpr(binary->right); if (!value) return std::nullopt;
                if (!assign(name->name, *value)) { ctError(name->span, "cannot assign compile-time value to unknown local '" + name->name + "'"); return std::nullopt; }
                return value;
            }
            if (binary->op == TokenKind::AndAnd || binary->op == TokenKind::OrOr) {
                auto left = evalExpr(binary->left); if (!left) return std::nullopt;
                bool truth = false; if (!truthy(*left, truth)) { ctError(binary->left->span, "compile-time logical operator requires bool"); return std::nullopt; }
                if (binary->op == TokenKind::AndAnd && !truth) return Literal{false};
                if (binary->op == TokenKind::OrOr && truth) return Literal{true};
                auto right = evalExpr(binary->right); if (!right) return std::nullopt;
                return evalBinary(binary->op, *left, *right, binary->span);
            }
            auto left = evalExpr(binary->left); if (!left) return std::nullopt;
            auto right = evalExpr(binary->right); if (!right) return std::nullopt;
            return evalBinary(binary->op, *left, *right, binary->span);
        }
        if (auto cast = std::dynamic_pointer_cast<CastExpr>(expr)) {
            auto value = evalExpr(cast->value); if (!value) return std::nullopt;
            const std::string& type = cast->typeName;
            if (type == "bool") { bool b = false; if (truthy(*value, b)) return Literal{b}; }
            if (type == "float" || type == "float64") {
                long double v = 0; bool integral = false; if (numeric(*value, v, integral)) return Literal{static_cast<double>(v)};
            }
            if (type == "int" || type == "int64" || type == "i64" || type == "isize") {
                long double v = 0; bool integral = false; if (numeric(*value, v, integral) && v >= static_cast<long double>(std::numeric_limits<std::int64_t>::min()) && v <= static_cast<long double>(std::numeric_limits<std::int64_t>::max())) return Literal{static_cast<std::int64_t>(v)};
            }
            ctError(cast->span, "cast to '" + type + "' is not supported in compile-time execution"); return std::nullopt;
        }
        if (auto call = std::dynamic_pointer_cast<CallExpr>(expr)) return evalCall(call);
        ctError(expr->span, "expression kind is not available during compile-time execution",
                "compile-time code is deterministic and cannot use pointers, host services, arrays, indexing or runtime-only state");
        return std::nullopt;
    }

    std::optional<Literal> evalCall(const std::shared_ptr<CallExpr>& call) {
        const std::string name = ctCallName(call->callee);
        if (name.empty() || name == "host" || name == "abi") { ctError(call->span, "host/ABI calls are forbidden during compile-time execution"); return std::nullopt; }
        auto found = functions_.find(name);
        if (found == functions_.end() || found->second->isExtern || !found->second->body) {
            ctError(call->span, "compile-time call target '" + name + "' is not a pure Noqeri function"); return std::nullopt;
        }
        auto function = found->second;
        if (function->params.size() != call->args.size()) { ctError(call->span, "compile-time function argument count mismatch for '" + name + "'"); return std::nullopt; }
        if (++callDepth_ > callDepthLimit_) { --callDepth_; ctError(call->span, "compile-time call depth exceeded 128 frames"); return std::nullopt; }
        std::vector<Literal> args;
        for (const auto& arg : call->args) { auto value = evalExpr(arg); if (!value) { --callDepth_; return std::nullopt; } args.push_back(*value); }
        auto saved = std::move(scopes_);
        scopes_.clear(); scopes_.push_back({});
        for (std::size_t i = 0; i < args.size(); ++i) scopes_.back()[function->params[i].name] = args[i];
        CtFlow flow = execBlock(function->body, false);
        scopes_ = std::move(saved);
        --callDepth_;
        if (!flow.ok) return std::nullopt;
        if (!flow.returned) return Literal{};
        return flow.value;
    }

    CtFlow execBlock(const std::shared_ptr<BlockStmt>& block, bool createScope = true) {
        if (createScope) scopes_.push_back({});
        CtFlow flow;
        for (const auto& stmt : block->statements) {
            flow = execStmt(stmt);
            if (!flow.ok || flow.returned) break;
        }
        if (createScope) scopes_.pop_back();
        return flow;
    }

    CtFlow execStmt(const StmtPtr& stmt) {
        CtFlow flow;
        if (!stmt || !step(stmt ? stmt->span : Span{})) { flow.ok = false; return flow; }
        if (auto let = std::dynamic_pointer_cast<LetStmt>(stmt)) {
            auto value = evalExpr(let->initializer); if (!value) { flow.ok = false; return flow; }
            if (scopes_.empty()) scopes_.push_back({}); scopes_.back()[let->name] = *value; return flow;
        }
        if (auto expr = std::dynamic_pointer_cast<ExprStmt>(stmt)) { if (!evalExpr(expr->expr)) flow.ok = false; return flow; }
        if (auto block = std::dynamic_pointer_cast<BlockStmt>(stmt)) return execBlock(block);
        if (auto branch = std::dynamic_pointer_cast<IfStmt>(stmt)) {
            auto condition = evalExpr(branch->condition); bool truth = false;
            if (!condition || !truthy(*condition, truth)) { ctError(branch->condition->span, "compile-time if condition must be bool"); flow.ok = false; return flow; }
            if (truth) return execStmt(branch->thenBranch);
            if (branch->elseBranch) return execStmt(branch->elseBranch);
            return flow;
        }
        if (auto loop = std::dynamic_pointer_cast<WhileStmt>(stmt)) {
            while (true) {
                auto condition = evalExpr(loop->condition); bool truth = false;
                if (!condition || !truthy(*condition, truth)) { ctError(loop->condition->span, "compile-time while condition must be bool"); flow.ok = false; return flow; }
                if (!truth) return flow;
                flow = execStmt(loop->body); if (!flow.ok || flow.returned) return flow;
            }
        }
        if (auto ret = std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
            auto value = ret->value ? evalExpr(ret->value) : std::optional<Literal>{Literal{}};
            if (!value) { flow.ok = false; return flow; }
            flow.returned = true; flow.value = *value; return flow;
        }
        if (std::dynamic_pointer_cast<ThrowStmt>(stmt)) { ctError(stmt->span, "throw is not permitted during compile-time execution"); flow.ok = false; return flow; }
        if (std::dynamic_pointer_cast<FunctionStmt>(stmt) || std::dynamic_pointer_cast<RecordStmt>(stmt) || std::dynamic_pointer_cast<ImportStmt>(stmt) || std::dynamic_pointer_cast<ModuleStmt>(stmt)) return flow;
        ctError(stmt->span, "statement kind is not supported during compile-time execution"); flow.ok = false; return flow;
    }

    ExprPtr literalExpr(const Literal& value, Span span) {
        auto out = std::make_shared<LiteralExpr>(); out->value = value; out->span = span; return out;
    }

    void transformExpr(ExprPtr& expr) {
        if (!expr) return;
        if (auto unary = std::dynamic_pointer_cast<UnaryExpr>(expr)) transformExpr(unary->operand);
        else if (auto binary = std::dynamic_pointer_cast<BinaryExpr>(expr)) { transformExpr(binary->left); transformExpr(binary->right); }
        else if (auto call = std::dynamic_pointer_cast<CallExpr>(expr)) {
            for (auto& arg : call->args) transformExpr(arg);
            const std::string name = ctCallName(call->callee);
            if (name == "comptime") {
                if (call->args.size() != 1) { ctError(call->span, "comptime expects exactly one expression"); return; }
                steps_ = 0; callDepth_ = 0;
                auto value = evalExpr(call->args[0]);
                if (value) expr = literalExpr(*value, call->span);
            } else if (name == "comptime_assert") {
                if (call->args.size() != 1) { ctError(call->span, "comptime_assert expects exactly one bool expression"); return; }
                steps_ = 0; callDepth_ = 0;
                auto value = evalExpr(call->args[0]); bool result = false;
                if (!value || !truthy(*value, result)) { if (value) ctError(call->span, "comptime_assert requires a bool expression"); return; }
                if (!result) { ctError(call->span, "compile-time assertion failed"); return; }
                expr = literalExpr(Literal{true}, call->span);
            }
        }
        else if (auto cast = std::dynamic_pointer_cast<CastExpr>(expr)) transformExpr(cast->value);
        else if (auto index = std::dynamic_pointer_cast<IndexExpr>(expr)) { transformExpr(index->object); transformExpr(index->index); }
        else if (auto member = std::dynamic_pointer_cast<MemberExpr>(expr)) transformExpr(member->object);
        else if (auto array = std::dynamic_pointer_cast<ArrayExpr>(expr)) for (auto& item : array->elements) transformExpr(item);
    }

    void transformStmt(StmtPtr& stmt, bool topLevel = false) {
        if (!stmt) return;
        if (auto let = std::dynamic_pointer_cast<LetStmt>(stmt)) {
            transformExpr(let->initializer);
            if (topLevel && let->isConst) {
                steps_ = 0; callDepth_ = 0;
                auto value = evalExpr(let->initializer);
                if (value) { globals_[let->name] = *value; let->initializer = literalExpr(*value, let->initializer->span); }
            }
            return;
        }
        if (auto expr = std::dynamic_pointer_cast<ExprStmt>(stmt)) { transformExpr(expr->expr); return; }
        if (auto block = std::dynamic_pointer_cast<BlockStmt>(stmt)) { for (auto& item : block->statements) transformStmt(item, false); return; }
        if (auto branch = std::dynamic_pointer_cast<IfStmt>(stmt)) { transformExpr(branch->condition); transformStmt(branch->thenBranch, false); if (branch->elseBranch) transformStmt(branch->elseBranch, false); return; }
        if (auto loop = std::dynamic_pointer_cast<WhileStmt>(stmt)) { transformExpr(loop->condition); transformStmt(loop->body, false); return; }
        if (auto ret = std::dynamic_pointer_cast<ReturnStmt>(stmt)) { transformExpr(ret->value); return; }
        if (auto thr = std::dynamic_pointer_cast<ThrowStmt>(stmt)) { transformExpr(thr->value); return; }
        if (auto fn = std::dynamic_pointer_cast<FunctionStmt>(stmt)) { if (fn->body) for (auto& item : fn->body->statements) transformStmt(item, false); return; }
    }
};

} // namespace

bool ComptimeEvaluator::evaluate(Program& program, Diagnostics& diagnostics) const {
    CtEngine engine(diagnostics);
    return engine.run(program);
}

} // namespace noe
