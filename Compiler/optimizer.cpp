#include "noe.hpp"
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace noe { namespace {

bool truthy(const NirValue& v) {
    if (auto p = std::get_if<bool>(&v)) return *p;
    if (auto p = std::get_if<std::int64_t>(&v)) return *p != 0;
    if (auto p = std::get_if<double>(&v)) return *p != 0.0;
    if (auto p = std::get_if<std::string>(&v)) return !p->empty();
    return false;
}

std::optional<NirValue> fold(const std::string& op, const NirValue& a, const NirValue& b) {
    if (auto x = std::get_if<std::int64_t>(&a)) if (auto y = std::get_if<std::int64_t>(&b)) {
        if (op == "+") return NirValue(*x + *y);
        if (op == "-") return NirValue(*x - *y);
        if (op == "*") return NirValue(*x * *y);
        if (op == "/" && *y != 0) return NirValue(*x / *y);
        if (op == "%" && *y != 0) return NirValue(*x % *y);
        if (op == "==") return NirValue(*x == *y);
        if (op == "!=") return NirValue(*x != *y);
        if (op == "<") return NirValue(*x < *y);
        if (op == "<=") return NirValue(*x <= *y);
        if (op == ">") return NirValue(*x > *y);
        if (op == ">=") return NirValue(*x >= *y);
    }
    if (auto x = std::get_if<double>(&a)) if (auto y = std::get_if<double>(&b)) {
        if (op == "+") return NirValue(*x + *y);
        if (op == "-") return NirValue(*x - *y);
        if (op == "*") return NirValue(*x * *y);
        if (op == "/" && *y != 0) return NirValue(*x / *y);
        if (op == "==") return NirValue(*x == *y);
        if (op == "!=") return NirValue(*x != *y);
        if (op == "<") return NirValue(*x < *y);
        if (op == "<=") return NirValue(*x <= *y);
        if (op == ">") return NirValue(*x > *y);
        if (op == ">=") return NirValue(*x >= *y);
    }
    if (auto x = std::get_if<std::string>(&a)) if (auto y = std::get_if<std::string>(&b)) {
        if (op == "+") return NirValue(*x + *y);
        if (op == "==") return NirValue(*x == *y);
        if (op == "!=") return NirValue(*x != *y);
    }
    if (op == "&&") return NirValue(truthy(a) && truthy(b));
    if (op == "||") return NirValue(truthy(a) || truthy(b));
    return std::nullopt;
}

bool sideEffect(const NirInstruction& i) {
    switch (i.op) {
        case NirOp::Store:
        case NirOp::StoreMemory:
        case NirOp::Call:
        case NirOp::AtomicStore:
        case NirOp::AtomicExchange:
        case NirOp::AtomicCompareExchange:
        case NirOp::AtomicFence:
        case NirOp::Intrinsic:
        case NirOp::InlineAsm:
        case NirOp::Try:
        case NirOp::Throw:
        case NirOp::Jump:
        case NirOp::JumpIfFalse:
        case NirOp::Return:
            return true;
        case NirOp::LoadMemory:
        case NirOp::AtomicLoad:
            return i.isVolatile || i.op == NirOp::AtomicLoad;
        default:
            return false;
    }
}

bool constants(NirFunction& fn) {
    bool changed = false;
    std::unordered_map<Reg, NirValue> known;
    for (auto& i : fn.code) {
        if (i.op == NirOp::Const && i.dest) {
            known[*i.dest] = i.literal;
            continue;
        }
        if (i.op == NirOp::Binary && i.dest && i.args.size() == 2) {
            auto a = known.find(i.args[0]);
            auto b = known.find(i.args[1]);
            if (a != known.end() && b != known.end()) {
                if (auto value = fold(i.text, a->second, b->second)) {
                    i.op = NirOp::Const;
                    i.literal = *value;
                    i.text.clear();
                    i.args.clear();
                    known[*i.dest] = *value;
                    changed = true;
                    continue;
                }
            }
        }
        if (i.dest) known.erase(*i.dest);
    }
    return changed;
}

bool dce(NirFunction& fn) {
    std::unordered_set<Reg> used;
    for (const auto& i : fn.code) {
        for (auto arg : i.args) used.insert(arg);
    }

    const std::size_t oldSize = fn.code.size();
    std::vector<bool> keep(oldSize, true);
    bool changed = false;
    for (std::size_t index = 0; index < oldSize; ++index) {
        const auto& i = fn.code[index];
        if (i.dest && !used.count(*i.dest) && !sideEffect(i)) {
            keep[index] = false;
            changed = true;
        }
    }
    if (!changed) return false;

    // NIR branches store instruction indices. Removing a dead instruction shifts
    // every later index, so branch targets must be translated to the compacted
    // instruction stream. prefixKept[t] is exactly the new index of old target t;
    // it also maps a target that names a removed instruction to the next live one.
    std::vector<std::size_t> prefixKept(oldSize + 1, 0);
    for (std::size_t index = 0; index < oldSize; ++index) {
        prefixKept[index + 1] = prefixKept[index] + (keep[index] ? 1u : 0u);
    }

    std::vector<NirInstruction> kept;
    kept.reserve(prefixKept[oldSize]);
    for (std::size_t index = 0; index < oldSize; ++index) {
        if (!keep[index]) continue;
        NirInstruction instruction = std::move(fn.code[index]);
        if ((instruction.op == NirOp::Jump || instruction.op == NirOp::JumpIfFalse) && instruction.target <= oldSize) {
            instruction.target = prefixKept[instruction.target];
        }
        kept.push_back(std::move(instruction));
    }
    fn.code = std::move(kept);
    return true;
}

void pipeline(NirFunction& fn, OptimizationLevel level) {
    if (level == OptimizationLevel::O0) return;
    int rounds = level == OptimizationLevel::O3 ? 3 : (level == OptimizationLevel::O2 ? 2 : 1);
    for (int r = 0; r < rounds; ++r) {
        bool a = constants(fn);
        bool b = dce(fn);
        if (!a && !b) break;
    }
    if (level == OptimizationLevel::Oz) dce(fn);
}

} // namespace

void PassManager::optimize(NirProgram& p) const {
    pipeline(p.entry, level_);
    for (auto& f : p.functions) pipeline(f, level_);
}

void Optimizer::optimize(NirProgram& p) const { optimize(p, OptimizationLevel::O1); }
void Optimizer::optimize(NirProgram& p, OptimizationLevel level) const { PassManager(level).optimize(p); }
void Optimizer::optimizeFunction(NirFunction& fn) const { pipeline(fn, OptimizationLevel::O1); }

} // namespace noe
