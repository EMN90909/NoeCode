#include "noe.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <unordered_map>

namespace noe {
namespace {

enum class ValueKind { Unknown, Int, Bool, String, Float, Null };

ValueKind literalKind(const NirValue& v) {
    if (std::holds_alternative<std::int64_t>(v)) return ValueKind::Int;
    if (std::holds_alternative<bool>(v)) return ValueKind::Bool;
    if (std::holds_alternative<std::string>(v)) return ValueKind::String;
    if (std::holds_alternative<double>(v)) return ValueKind::Float;
    if (std::holds_alternative<std::monostate>(v)) return ValueKind::Null;
    return ValueKind::Unknown;
}

bool isComparison(const std::string& op) {
    return op == "==" || op == "!=" || op == "<" || op == "<=" || op == ">" || op == ">=" || op == "&&" || op == "||";
}

std::string symbolFor(const std::string& name) {
    std::string out = "noe_";
    for (char c : name) out += (std::isalnum(static_cast<unsigned char>(c)) || c == '_') ? c : '_';
    return out;
}

std::string pcLabel(const NirFunction& fn, std::size_t pc) {
    return ".L_" + symbolFor(fn.name) + "_" + std::to_string(pc);
}

std::size_t align16(std::size_t n) { return (n + 15u) & ~std::size_t(15u); }

struct FunctionLayout {
    std::vector<std::string> variables;
    std::unordered_map<std::string, std::size_t> variableIndex;
    std::vector<ValueKind> regKinds;
    std::unordered_map<std::string, ValueKind> varKinds;
};

FunctionLayout layoutFor(const NirFunction& fn, const std::unordered_map<std::string, ValueKind>& returns) {
    FunctionLayout l;
    l.regKinds.assign(fn.nextReg, ValueKind::Unknown);
    auto addVar = [&](const std::string& n) {
        if (!l.variableIndex.count(n)) {
            l.variableIndex[n] = l.variables.size();
            l.variables.push_back(n);
        }
    };
    for (const auto& p : fn.params) addVar(p);
    for (const auto& i : fn.code) if (i.op == NirOp::Load || i.op == NirOp::Store) addVar(i.text);

    for (const auto& i : fn.code) {
        auto rk = [&](Reg r) { return r < l.regKinds.size() ? l.regKinds[r] : ValueKind::Unknown; };
        if (i.op == NirOp::Const && i.dest) l.regKinds[*i.dest] = literalKind(i.literal);
        else if (i.op == NirOp::Store && !i.args.empty()) l.varKinds[i.text] = rk(i.args[0]);
        else if (i.op == NirOp::Load && i.dest) {
            auto it = l.varKinds.find(i.text);
            l.regKinds[*i.dest] = it == l.varKinds.end() ? ValueKind::Unknown : it->second;
        } else if (i.op == NirOp::Unary && i.dest && !i.args.empty()) {
            l.regKinds[*i.dest] = i.text == "!" ? ValueKind::Bool : rk(i.args[0]);
        } else if (i.op == NirOp::Binary && i.dest && i.args.size() == 2) {
            if (isComparison(i.text)) l.regKinds[*i.dest] = ValueKind::Bool;
            else {
                auto a = rk(i.args[0]), b = rk(i.args[1]);
                if (a == ValueKind::String || b == ValueKind::String) l.regKinds[*i.dest] = ValueKind::String;
                else if (a == ValueKind::Float || b == ValueKind::Float) l.regKinds[*i.dest] = ValueKind::Float;
                else l.regKinds[*i.dest] = ValueKind::Int;
            }
        } else if (i.op == NirOp::Call && i.dest) {
            if (i.text == "print") l.regKinds[*i.dest] = ValueKind::Null;
            else {
                auto it = returns.find(i.text);
                l.regKinds[*i.dest] = it == returns.end() ? ValueKind::Unknown : it->second;
            }
        }
    }
    return l;
}

ValueKind inferReturn(const NirFunction& fn, const std::unordered_map<std::string, ValueKind>& returns) {
    auto l = layoutFor(fn, returns);
    ValueKind result = ValueKind::Null;
    for (const auto& i : fn.code) {
        if (i.op == NirOp::Return && !i.args.empty() && i.args[0] < l.regKinds.size()) {
            auto k = l.regKinds[i.args[0]];
            if (result == ValueKind::Null || result == ValueKind::Unknown) result = k;
            else if (k != ValueKind::Unknown && result != k) result = ValueKind::Unknown;
        }
    }
    return result;
}

std::string regSlot(Reg r) { return "QWORD PTR [rbp-" + std::to_string((static_cast<std::size_t>(r) + 1) * 8) + "]"; }
std::string varSlot(const NirFunction& fn, const FunctionLayout& l, const std::string& name) {
    auto it = l.variableIndex.find(name);
    if (it == l.variableIndex.end()) return "QWORD PTR [rbp-8]";
    auto index = static_cast<std::size_t>(fn.nextReg) + it->second + 1;
    return "QWORD PTR [rbp-" + std::to_string(index * 8) + "]";
}

void emitRuntime(std::ostream& out) {
    out << R"ASM(
.text
noe_rt_print_cstr:
    push rbp
    mov rbp, rsp
    mov rsi, rdi
    xor rdx, rdx
.L_noe_strlen:
    cmp BYTE PTR [rsi + rdx], 0
    je .L_noe_write_string
    inc rdx
    jmp .L_noe_strlen
.L_noe_write_string:
    mov rax, 1
    mov rdi, 1
    syscall
    lea rsi, [rip + .L_noe_newline]
    mov rdx, 1
    mov rax, 1
    mov rdi, 1
    syscall
    pop rbp
    ret

noe_rt_print_bool:
    push rbp
    mov rbp, rsp
    test rdi, rdi
    jz .L_noe_bool_false
    lea rdi, [rip + .L_noe_true]
    call noe_rt_print_cstr
    pop rbp
    ret
.L_noe_bool_false:
    lea rdi, [rip + .L_noe_false]
    call noe_rt_print_cstr
    pop rbp
    ret

noe_rt_print_int:
    push rbp
    mov rbp, rsp
    sub rsp, 64
    mov rax, rdi
    lea rsi, [rbp - 1]
    mov BYTE PTR [rsi], 10
    mov rcx, 1
    xor r8d, r8d
    test rax, rax
    jge .L_noe_int_abs_ready
    mov r8b, 1
    neg rax
.L_noe_int_abs_ready:
    test rax, rax
    jne .L_noe_int_digits
    dec rsi
    mov BYTE PTR [rsi], 48
    inc rcx
    jmp .L_noe_int_sign
.L_noe_int_digits:
    xor rdx, rdx
    mov r9, 10
    div r9
    add dl, 48
    dec rsi
    mov BYTE PTR [rsi], dl
    inc rcx
    test rax, rax
    jne .L_noe_int_digits
.L_noe_int_sign:
    test r8b, r8b
    jz .L_noe_int_write
    dec rsi
    mov BYTE PTR [rsi], 45
    inc rcx
.L_noe_int_write:
    mov rdx, rcx
    mov rax, 1
    mov rdi, 1
    syscall
    leave
    ret
)ASM";
}

bool emitFunction(std::ostream& out, const NirFunction& fn, const std::unordered_map<std::string, ValueKind>& returns,
                  const std::map<std::string, std::string>& strings, Diagnostics& diagnostics) {
    auto l = layoutFor(fn, returns);
    const char* argRegs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
    if (fn.params.size() > 6) {
        diagnostics.error("NOE-N5004", {}, "native x86-64 backend currently supports at most 6 function parameters");
        return false;
    }
    std::size_t frame = align16((static_cast<std::size_t>(fn.nextReg) + l.variables.size()) * 8);
    out << "\n" << symbolFor(fn.name) << ":\n"
        << "    push rbp\n    mov rbp, rsp\n";
    if (frame) out << "    sub rsp, " << frame << "\n";
    for (std::size_t i = 0; i < fn.params.size(); ++i)
        out << "    mov " << varSlot(fn, l, fn.params[i]) << ", " << argRegs[i] << "\n";

    auto loadReg = [&](Reg r, const char* cpu) { out << "    mov " << cpu << ", " << regSlot(r) << "\n"; };
    auto storeReg = [&](Reg r, const char* cpu) { out << "    mov " << regSlot(r) << ", " << cpu << "\n"; };

    for (std::size_t pc = 0; pc < fn.code.size(); ++pc) {
        const auto& i = fn.code[pc];
        out << pcLabel(fn, pc) << ":\n";
        switch (i.op) {
            case NirOp::Const: {
                if (!i.dest) break;
                if (auto v = std::get_if<std::int64_t>(&i.literal)) out << "    mov rax, " << *v << "\n";
                else if (auto v = std::get_if<bool>(&i.literal)) out << "    mov rax, " << (*v ? 1 : 0) << "\n";
                else if (auto v = std::get_if<std::string>(&i.literal)) {
                    auto s = strings.find(*v);
                    if (s == strings.end()) { diagnostics.error("NOE-N5005", {}, "internal string-pool error"); return false; }
                    out << "    lea rax, [rip + " << s->second << "]\n";
                } else if (std::holds_alternative<std::monostate>(i.literal)) out << "    xor eax, eax\n";
                else { diagnostics.error("NOE-N5001", {}, "float native code generation is not implemented yet", "use integer code or 'noe run' for the reference interpreter"); return false; }
                storeReg(*i.dest, "rax");
                break;
            }
            case NirOp::Load:
                if (i.dest) { out << "    mov rax, " << varSlot(fn, l, i.text) << "\n"; storeReg(*i.dest, "rax"); }
                break;
            case NirOp::Store:
                if (!i.args.empty()) { loadReg(i.args[0], "rax"); out << "    mov " << varSlot(fn, l, i.text) << ", rax\n"; }
                break;
            case NirOp::Unary:
                if (!i.dest || i.args.empty()) break;
                loadReg(i.args[0], "rax");
                if (i.text == "-") out << "    neg rax\n";
                else if (i.text == "!") out << "    test rax, rax\n    sete al\n    movzx rax, al\n";
                storeReg(*i.dest, "rax");
                break;
            case NirOp::Binary: {
                if (!i.dest || i.args.size() != 2) break;
                auto a = i.args[0] < l.regKinds.size() ? l.regKinds[i.args[0]] : ValueKind::Unknown;
                auto b = i.args[1] < l.regKinds.size() ? l.regKinds[i.args[1]] : ValueKind::Unknown;
                if (a == ValueKind::Float || b == ValueKind::Float) { diagnostics.error("NOE-N5001", {}, "float native arithmetic is not implemented yet"); return false; }
                if (a == ValueKind::String || b == ValueKind::String) { diagnostics.error("NOE-N5002", {}, "runtime string binary operations are not implemented by the native backend yet", "constant strings and print(string) are supported"); return false; }
                loadReg(i.args[0], "rax"); loadReg(i.args[1], "rbx");
                if (i.text == "+") out << "    add rax, rbx\n";
                else if (i.text == "-") out << "    sub rax, rbx\n";
                else if (i.text == "*") out << "    imul rax, rbx\n";
                else if (i.text == "/" || i.text == "%") { out << "    cqo\n    idiv rbx\n"; if (i.text == "%") out << "    mov rax, rdx\n"; }
                else if (i.text == "&&" || i.text == "||") {
                    out << "    test rax, rax\n    setne al\n    movzx rax, al\n    test rbx, rbx\n    setne bl\n    movzx rbx, bl\n";
                    out << (i.text == "&&" ? "    and rax, rbx\n" : "    or rax, rbx\n");
                } else {
                    out << "    cmp rax, rbx\n";
                    if (i.text == "==") out << "    sete al\n";
                    else if (i.text == "!=") out << "    setne al\n";
                    else if (i.text == "<") out << "    setl al\n";
                    else if (i.text == "<=") out << "    setle al\n";
                    else if (i.text == ">") out << "    setg al\n";
                    else if (i.text == ">=") out << "    setge al\n";
                    else { diagnostics.error("NOE-N5003", {}, "unsupported native binary operator '" + i.text + "'"); return false; }
                    out << "    movzx rax, al\n";
                }
                storeReg(*i.dest, "rax");
                break;
            }
            case NirOp::Call: {
                if (i.text == "print") {
                    if (i.args.size() != 1) { diagnostics.error("NOE-N5006", {}, "native print currently accepts exactly one argument"); return false; }
                    Reg r = i.args[0]; loadReg(r, "rdi");
                    ValueKind k = r < l.regKinds.size() ? l.regKinds[r] : ValueKind::Unknown;
                    if (k == ValueKind::String) out << "    call noe_rt_print_cstr\n";
                    else if (k == ValueKind::Bool) out << "    call noe_rt_print_bool\n";
                    else if (k == ValueKind::Float) { diagnostics.error("NOE-N5001", {}, "float printing is not implemented by the native backend yet"); return false; }
                    else out << "    call noe_rt_print_int\n";
                    if (i.dest) { out << "    xor eax, eax\n"; storeReg(*i.dest, "rax"); }
                } else {
                    if (i.args.size() > 6) { diagnostics.error("NOE-N5004", {}, "native x86-64 calls currently support at most 6 arguments"); return false; }
                    for (std::size_t a = 0; a < i.args.size(); ++a) loadReg(i.args[a], argRegs[a]);
                    out << "    call " << symbolFor(i.text) << "\n";
                    if (i.dest) storeReg(*i.dest, "rax");
                }
                break;
            }
            case NirOp::Jump:
                out << "    jmp " << pcLabel(fn, i.target) << "\n";
                break;
            case NirOp::JumpIfFalse:
                if (!i.args.empty()) { loadReg(i.args[0], "rax"); out << "    test rax, rax\n    jz " << pcLabel(fn, i.target) << "\n"; }
                break;
            case NirOp::Return:
                if (!i.args.empty()) loadReg(i.args[0], "rax"); else out << "    xor eax, eax\n";
                out << "    leave\n    ret\n";
                break;
            case NirOp::Nop:
                out << "    nop\n";
                break;
        }
    }
    out << pcLabel(fn, fn.code.size()) << ":\n    xor eax, eax\n    leave\n    ret\n";
    return true;
}

} // namespace

bool NativeBackend::emitAssembly(const NirProgram& program, const std::filesystem::path& output, Diagnostics& diagnostics) const {
#if !defined(__linux__) || !defined(__x86_64__)
    (void)program; (void)output;
    diagnostics.error("NOE-N5000", {}, "bootstrap native backend currently targets Linux x86-64 only", "use 'noe run' on this host; more target emitters can be added behind NIR");
    return false;
#else
    std::unordered_map<std::string, ValueKind> returns;
    for (const auto& f : program.functions) returns[f.name] = ValueKind::Unknown;
    for (int pass = 0; pass < 4; ++pass) for (const auto& f : program.functions) returns[f.name] = inferReturn(f, returns);

    std::map<std::string, std::string> pool;
    std::size_t stringId = 0;
    auto collect = [&](const NirFunction& fn) {
        for (const auto& i : fn.code) if (i.op == NirOp::Const) if (auto s = std::get_if<std::string>(&i.literal)) if (!pool.count(*s)) pool[*s] = ".L_noe_string_" + std::to_string(stringId++);
    };
    collect(program.entry); for (const auto& f : program.functions) collect(f);

    std::filesystem::create_directories(output.parent_path().empty() ? std::filesystem::path(".") : output.parent_path());
    std::ofstream out(output, std::ios::trunc);
    if (!out) { diagnostics.error("NOE-N5007", {}, "cannot create native assembly output: " + output.string()); return false; }
    out << ".intel_syntax noprefix\n.section .rodata\n.L_noe_newline:\n    .byte 10\n.L_noe_true:\n    .asciz \"true\"\n.L_noe_false:\n    .asciz \"false\"\n";
    for (const auto& [text, label] : pool) {
        out << label << ":\n    .byte ";
        if (text.empty()) out << "0";
        else { for (std::size_t i = 0; i < text.size(); ++i) { if (i) out << ','; out << static_cast<unsigned int>(static_cast<unsigned char>(text[i])); } out << ",0"; }
        out << "\n";
    }
    emitRuntime(out);
    if (!emitFunction(out, program.entry, returns, pool, diagnostics)) return false;
    for (const auto& f : program.functions) if (!emitFunction(out, f, returns, pool, diagnostics)) return false;
    out << "\n.global _start\n_start:\n    call " << symbolFor(program.entry.name) << "\n    xor edi, edi\n    mov rax, 60\n    syscall\n"
        << ".section .note.GNU-stack,\"\",@progbits\n";
    return !diagnostics.hasErrors();
#endif
}

} // namespace noe
