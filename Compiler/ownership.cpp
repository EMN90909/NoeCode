#include "noe.hpp"
#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace noe {
namespace {

std::string callName(const ExprPtr& expr) {
    if (auto name = std::dynamic_pointer_cast<NameExpr>(expr)) return name->name;
    if (auto member = std::dynamic_pointer_cast<MemberExpr>(expr)) {
        if (auto base = std::dynamic_pointer_cast<NameExpr>(member->object)) return base->name + "." + member->member;
    }
    return {};
}

bool isPrimitiveCopyType(const std::string& raw) {
    std::string name = raw;
    name.erase(std::remove_if(name.begin(), name.end(), [](char c) { return c == ' ' || c == '\t'; }), name.end());
    if (name.empty()) return true;
    if (name.rfind("*", 0) == 0 || name.rfind("[]", 0) == 0) return true;
    static const std::unordered_set<std::string> copy = {
        "bool", "i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64",
        "isize", "usize", "int", "int64", "float", "float64", "string", "void"
    };
    return copy.count(name) != 0;
}

bool annotationOwned(const std::optional<std::string>& annotation,
                     const std::unordered_set<std::string>& records) {
    if (!annotation) return false;
    if (annotation->size() > 2 && annotation->front() == '[') return true;
    if (records.count(*annotation)) return true;
    return !isPrimitiveCopyType(*annotation);
}

struct VariableState {
    bool owned = false;
    bool moved = false;
    std::size_t sharedBorrows = 0;
    std::string borrowsFrom;
    Span declaredAt{};
};

struct FunctionOwnership {
    std::vector<bool> parameterOwned;
};

class OwnershipAnalysis {
public:
    OwnershipAnalysis(const Program& program, Diagnostics& diagnostics) : diagnostics_(diagnostics) {
        for (const auto& stmt : program.statements) {
            if (auto record = std::dynamic_pointer_cast<RecordStmt>(stmt)) records_.insert(record->name);
        }
        for (const auto& stmt : program.statements) {
            if (auto function = std::dynamic_pointer_cast<FunctionStmt>(stmt)) {
                FunctionOwnership info;
                for (const auto& param : function->params) info.parameterOwned.push_back(annotationOwned(param.annotation, records_));
                functions_[function->name] = std::move(info);
            }
        }
    }

    bool run(const Program& program) {
        pushScope();
        for (const auto& stmt : program.statements) analyzeStmt(stmt);
        popScope();
        return !diagnostics_.hasErrors();
    }

private:
    Diagnostics& diagnostics_;
    std::unordered_set<std::string> records_;
    std::unordered_map<std::string, FunctionOwnership> functions_;
    std::unordered_map<std::string, VariableState> variables_;
    std::vector<std::vector<std::string>> scopes_;

    void pushScope() { scopes_.push_back({}); }

    void popScope() {
        if (scopes_.empty()) return;
        for (auto it = scopes_.back().rbegin(); it != scopes_.back().rend(); ++it) {
            auto found = variables_.find(*it);
            if (found == variables_.end()) continue;
            if (!found->second.borrowsFrom.empty()) {
                auto source = variables_.find(found->second.borrowsFrom);
                if (source != variables_.end() && source->second.sharedBorrows > 0) --source->second.sharedBorrows;
            }
            variables_.erase(found);
        }
        scopes_.pop_back();
    }

    void define(const std::string& name, VariableState state) {
        variables_[name] = std::move(state);
        if (!scopes_.empty()) scopes_.back().push_back(name);
    }

    VariableState* stateFor(const std::string& name) {
        auto found = variables_.find(name);
        return found == variables_.end() ? nullptr : &found->second;
    }

    void requireAvailable(const std::string& name, Span span) {
        auto* state = stateFor(name);
        if (state && state->owned && state->moved) {
            diagnostics_.error("NQR-O5001", span,
                "use of moved value '" + name + "'",
                "pass a borrow with '&' when ownership should remain with the caller, or create a fresh value before reuse");
        }
    }

    void consume(const std::string& name, Span span, const std::string& reason) {
        auto* state = stateFor(name);
        if (!state || !state->owned) return;
        requireAvailable(name, span);
        if (state->moved) return;
        if (state->sharedBorrows != 0) {
            diagnostics_.error("NQR-O5002", span,
                "cannot move '" + name + "' while it is borrowed",
                "the borrow must leave scope before this " + reason);
            return;
        }
        state->moved = true;
    }

    void writeTo(const std::string& name, Span span) {
        auto* state = stateFor(name);
        if (!state) return;
        if (state->sharedBorrows != 0) {
            diagnostics_.error("NQR-O5003", span,
                "cannot mutate '" + name + "' while a shared borrow is live",
                "end the borrow scope before assigning a new value");
            return;
        }
        state->moved = false;
    }

    std::string borrowedName(const ExprPtr& expr) const {
        auto unary = std::dynamic_pointer_cast<UnaryExpr>(expr);
        if (!unary || unary->op != TokenKind::Ampersand) return {};
        if (auto name = std::dynamic_pointer_cast<NameExpr>(unary->operand)) return name->name;
        return {};
    }

    bool inferredOwned(const ExprPtr& initializer) {
        if (!initializer) return false;
        if (std::dynamic_pointer_cast<ArrayExpr>(initializer)) return true;
        if (auto name = std::dynamic_pointer_cast<NameExpr>(initializer)) {
            auto* source = stateFor(name->name);
            return source && source->owned;
        }
        return false;
    }

    void bindBorrow(const std::string& borrower, const std::string& source, Span span) {
        auto* state = stateFor(source);
        if (!state) return;
        requireAvailable(source, span);
        ++state->sharedBorrows;
        auto* borrowerState = stateFor(borrower);
        if (borrowerState) borrowerState->borrowsFrom = source;
    }

    bool isSpawnLike(const std::string& name) const {
        return name == "spawn" || name == "task_spawn" || name == "thread_spawn" ||
               name == "task.spawn" || name == "thread.spawn" || name == "taskGroup.spawn";
    }

    void analyzeExpr(const ExprPtr& expr, bool consumeValue = false, const std::string& reason = "value transfer") {
        if (!expr) return;
        if (auto name = std::dynamic_pointer_cast<NameExpr>(expr)) {
            requireAvailable(name->name, name->span);
            if (consumeValue) consume(name->name, name->span, reason);
            return;
        }
        if (auto unary = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
            if (unary->op == TokenKind::Ampersand) {
                if (auto name = std::dynamic_pointer_cast<NameExpr>(unary->operand)) requireAvailable(name->name, name->span);
                else analyzeExpr(unary->operand, false);
            } else {
                analyzeExpr(unary->operand, false);
            }
            return;
        }
        if (auto binary = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
            if (binary->op == TokenKind::Equal) {
                analyzeExpr(binary->right, true, "assignment");
                if (auto name = std::dynamic_pointer_cast<NameExpr>(binary->left)) writeTo(name->name, name->span);
                else analyzeExpr(binary->left, false);
            } else {
                analyzeExpr(binary->left, false);
                analyzeExpr(binary->right, false);
            }
            return;
        }
        if (auto call = std::dynamic_pointer_cast<CallExpr>(expr)) {
            const std::string name = callName(call->callee);
            auto signature = functions_.find(name);
            for (std::size_t i = 0; i < call->args.size(); ++i) {
                const bool ownedParameter = signature != functions_.end() && i < signature->second.parameterOwned.size() && signature->second.parameterOwned[i];
                if (isSpawnLike(name)) {
                    const std::string borrowed = borrowedName(call->args[i]);
                    if (!borrowed.empty()) {
                        diagnostics_.error("NQR-O5004", call->args[i]->span,
                            "borrow of local '" + borrowed + "' cannot cross a spawned task boundary",
                            "move an owned value into the task or copy a scalar value instead");
                    } else if (auto argName = std::dynamic_pointer_cast<NameExpr>(call->args[i])) {
                        auto* state = stateFor(argName->name);
                        analyzeExpr(call->args[i], state && state->owned, "spawn transfer");
                        continue;
                    }
                }
                analyzeExpr(call->args[i], ownedParameter, "function argument transfer");
            }
            return;
        }
        if (auto cast = std::dynamic_pointer_cast<CastExpr>(expr)) { analyzeExpr(cast->value, false); return; }
        if (auto index = std::dynamic_pointer_cast<IndexExpr>(expr)) { analyzeExpr(index->object, false); analyzeExpr(index->index, false); return; }
        if (auto member = std::dynamic_pointer_cast<MemberExpr>(expr)) { analyzeExpr(member->object, false); return; }
        if (auto array = std::dynamic_pointer_cast<ArrayExpr>(expr)) { for (const auto& item : array->elements) analyzeExpr(item, true, "aggregate construction"); return; }
    }

    void analyzeStmt(const StmtPtr& stmt) {
        if (!stmt) return;
        if (auto let = std::dynamic_pointer_cast<LetStmt>(stmt)) {
            const bool owned = annotationOwned(let->annotation, records_) || (!let->annotation && inferredOwned(let->initializer));
            const std::string sourceBorrow = borrowedName(let->initializer);
            if (sourceBorrow.empty()) analyzeExpr(let->initializer, owned, "binding initialization");
            else analyzeExpr(let->initializer, false);
            VariableState state; state.owned = owned; state.declaredAt = let->span;
            define(let->name, state);
            if (!sourceBorrow.empty()) bindBorrow(let->name, sourceBorrow, let->span);
            return;
        }
        if (auto expr = std::dynamic_pointer_cast<ExprStmt>(stmt)) { analyzeExpr(expr->expr, false); return; }
        if (auto block = std::dynamic_pointer_cast<BlockStmt>(stmt)) {
            pushScope();
            for (const auto& item : block->statements) analyzeStmt(item);
            popScope();
            return;
        }
        if (auto branch = std::dynamic_pointer_cast<IfStmt>(stmt)) {
            analyzeExpr(branch->condition, false);
            const auto before = variables_;
            analyzeStmt(branch->thenBranch);
            const auto afterThen = variables_;
            variables_ = before;
            if (branch->elseBranch) analyzeStmt(branch->elseBranch);
            const auto afterElse = variables_;
            variables_ = before;
            for (auto& [name, state] : variables_) {
                auto t = afterThen.find(name), e = afterElse.find(name);
                const bool thenMoved = t != afterThen.end() && t->second.moved;
                const bool elseMoved = e != afterElse.end() && e->second.moved;
                state.moved = state.moved || thenMoved || elseMoved;
                if (t != afterThen.end()) state.sharedBorrows = std::max(state.sharedBorrows, t->second.sharedBorrows);
                if (e != afterElse.end()) state.sharedBorrows = std::max(state.sharedBorrows, e->second.sharedBorrows);
            }
            return;
        }
        if (auto loop = std::dynamic_pointer_cast<WhileStmt>(stmt)) {
            analyzeExpr(loop->condition, false);
            const auto before = variables_;
            analyzeStmt(loop->body);
            const auto after = variables_;
            variables_ = before;
            for (auto& [name, state] : variables_) {
                auto it = after.find(name);
                if (it != after.end()) {
                    state.moved = state.moved || it->second.moved;
                    state.sharedBorrows = std::max(state.sharedBorrows, it->second.sharedBorrows);
                }
            }
            return;
        }
        if (auto ret = std::dynamic_pointer_cast<ReturnStmt>(stmt)) { analyzeExpr(ret->value, true, "return transfer"); return; }
        if (auto thr = std::dynamic_pointer_cast<ThrowStmt>(stmt)) { analyzeExpr(thr->value, false); return; }
        if (auto function = std::dynamic_pointer_cast<FunctionStmt>(stmt)) {
            if (function->isExtern || !function->body) return;
            const auto savedVariables = variables_;
            const auto savedScopes = scopes_;
            variables_.clear(); scopes_.clear(); pushScope();
            for (const auto& param : function->params) {
                VariableState state; state.owned = annotationOwned(param.annotation, records_); state.declaredAt = param.span;
                define(param.name, state);
            }
            for (const auto& item : function->body->statements) analyzeStmt(item);
            popScope();
            variables_ = savedVariables;
            scopes_ = savedScopes;
            return;
        }
    }
};

} // namespace

bool OwnershipChecker::check(const Program& program, Diagnostics& diagnostics) const {
    OwnershipAnalysis analysis(program, diagnostics);
    return analysis.run(program);
}

} // namespace noe
