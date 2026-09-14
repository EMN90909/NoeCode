#include "noe.hpp"
#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
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

class OwnershipInspector {
public:
    explicit OwnershipInspector(Diagnostics& diagnostics) : diagnostics_(diagnostics) { scopes_.push_back({}); }

    void parameter(const Parameter& parameter) { declare(parameter.name); }

    void run(const std::shared_ptr<BlockStmt>& body) {
        if (!body) return;
        for (auto& statement : body->statements) inspectStatement(statement);
    }

    void runTopLevel(std::vector<StmtPtr>& statements) {
        for (auto& statement : statements) {
            if (!std::dynamic_pointer_cast<FunctionStmt>(statement)) inspectStatement(statement);
        }
    }

private:
    struct Snapshot {
        std::unordered_set<std::string> moved;
        std::unordered_map<std::string, std::string> aliases;
        std::unordered_map<std::string, std::size_t> borrows;
    };

    void declare(const std::string& name) {
        if (scopes_.empty()) scopes_.push_back({});
        scopes_.back().push_back(name);
        moved_.erase(name);
        clearAlias(name);
    }

    void pushScope() { scopes_.push_back({}); }

    void popScope() {
        if (scopes_.empty()) return;
        for (const auto& name : scopes_.back()) {
            clearAlias(name);
            moved_.erase(name);
            borrowCounts_.erase(name);
        }
        scopes_.pop_back();
    }

    void clearAlias(const std::string& alias) {
        auto it = aliasSource_.find(alias);
        if (it == aliasSource_.end()) return;
        auto count = borrowCounts_.find(it->second);
        if (count != borrowCounts_.end()) {
            if (count->second > 1) --count->second;
            else borrowCounts_.erase(count);
        }
        aliasSource_.erase(it);
    }

    std::string borrowSource(const ExprPtr& expression) const {
        if (!expression) return {};
        if (auto unary = std::dynamic_pointer_cast<UnaryExpr>(expression); unary && unary->op == TokenKind::Ampersand) {
            if (auto name = std::dynamic_pointer_cast<NameExpr>(unary->operand)) return name->name;
            return {};
        }
        if (auto name = std::dynamic_pointer_cast<NameExpr>(expression)) {
            auto it = aliasSource_.find(name->name);
            return it == aliasSource_.end() ? std::string{} : it->second;
        }
        if (auto cast = std::dynamic_pointer_cast<CastExpr>(expression)) return borrowSource(cast->value);
        return {};
    }

    void bindAlias(const std::string& alias, const ExprPtr& value) {
        clearAlias(alias);
        const auto source = borrowSource(value);
        if (source.empty() || source == alias) return;
        aliasSource_[alias] = source;
        ++borrowCounts_[source];
    }

    bool isBorrowed(const std::string& name) const {
        auto it = borrowCounts_.find(name);
        return it != borrowCounts_.end() && it->second != 0;
    }

    void checkReadable(const std::string& name, Span span) {
        if (moved_.count(name)) {
            diagnostics_.error("NQR-O4201", span,
                "use of moved value '" + name + "'",
                "reinitialize the owner before using it again, or borrow it instead of moving it");
        }
    }

    void checkMutation(const std::string& name, Span span) {
        if (isBorrowed(name)) {
            diagnostics_.error("NQR-O4202", span,
                "cannot mutate '" + name + "' while it has a live borrow",
                "end the borrow's lexical scope before mutating the owner");
        }
    }

    void inspectMove(ExprPtr& expression, const std::shared_ptr<CallExpr>& call) {
        if (call->args.size() != 1) {
            diagnostics_.error("NQR-O4203", call->span, "move expects exactly one named value");
            return;
        }
        auto owner = std::dynamic_pointer_cast<NameExpr>(call->args[0]);
        if (!owner) {
            diagnostics_.error("NQR-O4204", call->span,
                "move currently requires a named owner",
                "bind the value to a local first, then move that local");
            inspectExpression(call->args[0]);
            return;
        }
        checkReadable(owner->name, owner->span);
        if (isBorrowed(owner->name)) {
            diagnostics_.error("NQR-O4205", call->span,
                "cannot move '" + owner->name + "' while it has a live borrow",
                "end the borrow's lexical scope before transferring ownership");
        }
        moved_.insert(owner->name);
        expression = call->args[0]; // move is a compile-time ownership operation; runtime value is unchanged.
    }

    void inspectExpression(ExprPtr& expression, bool assignmentTarget = false) {
        if (!expression) return;
        if (auto name = std::dynamic_pointer_cast<NameExpr>(expression)) {
            if (!assignmentTarget) checkReadable(name->name, name->span);
            return;
        }
        if (auto unary = std::dynamic_pointer_cast<UnaryExpr>(expression)) {
            if (unary->op == TokenKind::Ampersand) {
                if (auto name = std::dynamic_pointer_cast<NameExpr>(unary->operand)) checkReadable(name->name, name->span);
            }
            inspectExpression(unary->operand);
            return;
        }
        if (auto call = std::dynamic_pointer_cast<CallExpr>(expression)) {
            if (calleeName(call->callee) == "move") { inspectMove(expression, call); return; }
            for (auto& arg : call->args) inspectExpression(arg);
            return;
        }
        if (auto binary = std::dynamic_pointer_cast<BinaryExpr>(expression)) {
            if (binary->op == TokenKind::Equal) {
                inspectExpression(binary->right);
                if (auto target = std::dynamic_pointer_cast<NameExpr>(binary->left)) {
                    checkMutation(target->name, target->span);
                    moved_.erase(target->name);
                    bindAlias(target->name, binary->right);
                } else {
                    inspectExpression(binary->left, true);
                }
                return;
            }
            inspectExpression(binary->left);
            inspectExpression(binary->right);
            return;
        }
        if (auto cast = std::dynamic_pointer_cast<CastExpr>(expression)) { inspectExpression(cast->value); return; }
        if (auto index = std::dynamic_pointer_cast<IndexExpr>(expression)) {
            inspectExpression(index->object);
            inspectExpression(index->index);
            return;
        }
        if (auto member = std::dynamic_pointer_cast<MemberExpr>(expression)) { inspectExpression(member->object); return; }
        if (auto array = std::dynamic_pointer_cast<ArrayExpr>(expression)) {
            for (auto& item : array->elements) inspectExpression(item);
        }
    }

    Snapshot snapshot() const { return Snapshot{moved_, aliasSource_, borrowCounts_}; }
    void restore(const Snapshot& s) { moved_ = s.moved; aliasSource_ = s.aliases; borrowCounts_ = s.borrows; }

    void mergeControlFlow(const Snapshot& a, const Snapshot& b) {
        moved_ = a.moved;
        moved_.insert(b.moved.begin(), b.moved.end());
        aliasSource_.clear();
        for (const auto& [alias, source] : a.aliases) {
            auto it = b.aliases.find(alias);
            if (it != b.aliases.end() && it->second == source) aliasSource_[alias] = source;
        }
        borrowCounts_ = a.borrows;
        for (const auto& [source, count] : b.borrows) {
            auto& slot = borrowCounts_[source];
            slot = std::max(slot, count);
        }
    }

    void inspectBranch(StmtPtr& statement) {
        if (!statement) return;
        pushScope();
        inspectStatement(statement);
        popScope();
    }

    void inspectStatement(StmtPtr& statement) {
        if (!statement) return;
        if (auto let = std::dynamic_pointer_cast<LetStmt>(statement)) {
            inspectExpression(let->initializer);
            declare(let->name);
            bindAlias(let->name, let->initializer);
            return;
        }
        if (auto expr = std::dynamic_pointer_cast<ExprStmt>(statement)) { inspectExpression(expr->expr); return; }
        if (auto returned = std::dynamic_pointer_cast<ReturnStmt>(statement)) { inspectExpression(returned->value); return; }
        if (auto thrown = std::dynamic_pointer_cast<ThrowStmt>(statement)) { inspectExpression(thrown->value); return; }
        if (auto block = std::dynamic_pointer_cast<BlockStmt>(statement)) {
            pushScope();
            for (auto& child : block->statements) inspectStatement(child);
            popScope();
            return;
        }
        if (auto branch = std::dynamic_pointer_cast<IfStmt>(statement)) {
            inspectExpression(branch->condition);
            const auto base = snapshot();
            inspectBranch(branch->thenBranch);
            const auto thenState = snapshot();
            restore(base);
            if (branch->elseBranch) inspectBranch(branch->elseBranch);
            const auto elseState = snapshot();
            mergeControlFlow(thenState, elseState);
            return;
        }
        if (auto loop = std::dynamic_pointer_cast<WhileStmt>(statement)) {
            inspectExpression(loop->condition);
            const auto before = snapshot();
            inspectBranch(loop->body);
            const auto after = snapshot();
            mergeControlFlow(before, after); // loop may run zero or many times.
            return;
        }
    }

    Diagnostics& diagnostics_;
    std::unordered_set<std::string> moved_;
    std::unordered_map<std::string, std::string> aliasSource_;
    std::unordered_map<std::string, std::size_t> borrowCounts_;
    std::vector<std::vector<std::string>> scopes_;
};

} // namespace

bool OwnershipChecker::check(Program& program, Diagnostics& diagnostics) const {
    OwnershipInspector topLevel(diagnostics);
    topLevel.runTopLevel(program.statements);
    for (auto& statement : program.statements) {
        auto function = std::dynamic_pointer_cast<FunctionStmt>(statement);
        if (!function || function->isExtern || !function->body) continue;
        OwnershipInspector inspector(diagnostics);
        for (const auto& parameter : function->params) inspector.parameter(parameter);
        inspector.run(function->body);
    }
    return !diagnostics.hasErrors();
}

} // namespace noe
