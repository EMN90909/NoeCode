#pragma once
#include "types.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace noe {

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

struct NirInstruction { NirOp op=NirOp::Nop; std::optional<Reg> dest; Type type{}; std::string text; NirValue literal; std::vector<Reg> args; std::size_t target=0; std::size_t width=8; bool isVolatile=false; };
struct NirFunction { std::string name; std::vector<std::string> genericParams; std::vector<std::string> params; std::vector<Type> paramTypes; Type resultType{}; std::vector<NirInstruction> code; Reg nextReg=0; bool isExtern=false; bool isExport=false; };
struct NirProgram { NirFunction entry; std::vector<NirFunction> functions; };
std::string printNir(const NirProgram& program);

} // namespace noe
