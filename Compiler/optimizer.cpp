#include "noe.hpp"
#include <cstring>
#include <limits>
#include <optional>
#include <unordered_map>
#include <unordered_set>
namespace noe { namespace {
bool truthy(const NirValue&v){if(auto p=std::get_if<bool>(&v))return*p;if(auto p=std::get_if<std::int64_t>(&v))return*p!=0;if(auto p=std::get_if<double>(&v))return*p!=0.0;if(auto p=std::get_if<std::string>(&v))return!p->empty();return false;}
std::int64_t wrapped(std::uint64_t value){std::int64_t out=0;std::memcpy(&out,&value,sizeof(out));return out;}
std::optional<NirValue>fold(const std::string&op,const NirValue&a,const NirValue&b){
    if(auto x=std::get_if<std::int64_t>(&a))if(auto y=std::get_if<std::int64_t>(&b)){
        if(op=="+")return NirValue(wrapped(static_cast<std::uint64_t>(*x)+static_cast<std::uint64_t>(*y)));
        if(op=="-")return NirValue(wrapped(static_cast<std::uint64_t>(*x)-static_cast<std::uint64_t>(*y)));
        if(op=="*")return NirValue(wrapped(static_cast<std::uint64_t>(*x)*static_cast<std::uint64_t>(*y)));
        if(op=="/"&&*y!=0){if(*x==std::numeric_limits<std::int64_t>::min()&&*y==-1)return NirValue(*x);return NirValue(*x / *y);}
        if(op=="%"&&*y!=0){if(*x==std::numeric_limits<std::int64_t>::min()&&*y==-1)return NirValue(std::int64_t(0));return NirValue(*x % *y);}
        if(op=="==")return NirValue(*x==*y);if(op=="!=")return NirValue(*x!=*y);if(op=="<")return NirValue(*x<*y);if(op=="<=")return NirValue(*x<=*y);if(op==">")return NirValue(*x>*y);if(op==">=")return NirValue(*x>=*y);
    }
    if(auto x=std::get_if<double>(&a))if(auto y=std::get_if<double>(&b)){if(op=="+")return NirValue(*x+*y);if(op=="-")return NirValue(*x-*y);if(op=="*")return NirValue(*x**y);if(op=="/"&&*y!=0)return NirValue(*x / *y);if(op=="==")return NirValue(*x==*y);if(op=="!=")return NirValue(*x!=*y);if(op=="<")return NirValue(*x<*y);if(op=="<=")return NirValue(*x<=*y);if(op==">")return NirValue(*x>*y);if(op==">=")return NirValue(*x>=*y);}
    if(auto x=std::get_if<std::string>(&a))if(auto y=std::get_if<std::string>(&b)){if(op=="+")return NirValue(*x+*y);if(op=="==")return NirValue(*x==*y);if(op=="!=")return NirValue(*x!=*y);}
    if(op=="&&")return NirValue(truthy(a)&&truthy(b));if(op=="||")return NirValue(truthy(a)||truthy(b));return std::nullopt;
}
bool sideEffect(const NirInstruction&i){switch(i.op){case NirOp::CheckNonNull:case NirOp::CheckBounds:case NirOp::Store:case NirOp::StoreMemory:case NirOp::Call:case NirOp::AtomicStore:case NirOp::AtomicExchange:case NirOp::AtomicCompareExchange:case NirOp::AtomicFence:case NirOp::Intrinsic:case NirOp::InlineAsm:case NirOp::Try:case NirOp::Throw:case NirOp::Jump:case NirOp::JumpIfFalse:case NirOp::Return:return true;case NirOp::LoadMemory:case NirOp::AtomicLoad:return i.isVolatile||i.op==NirOp::AtomicLoad;default:return false;}}
bool constants(NirFunction&fn){
    bool changed=false;std::unordered_map<Reg,NirValue>known;
    for(auto&i:fn.code){
        if(i.op==NirOp::Const&&i.dest){known[*i.dest]=i.literal;continue;}
        if(i.op==NirOp::Binary&&i.dest&&i.args.size()==2){auto a=known.find(i.args[0]),b=known.find(i.args[1]);if(a!=known.end()&&b!=known.end())if(auto value=fold(i.text,a->second,b->second)){i.op=NirOp::Const;i.literal=*value;i.text.clear();i.args.clear();known[*i.dest]=*value;changed=true;continue;}}
        if(i.op==NirOp::CheckNonNull&&i.args.size()==1){auto a=known.find(i.args[0]);if(a!=known.end()){if(auto value=std::get_if<std::int64_t>(&a->second);value&&*value!=0){i.op=NirOp::Nop;i.args.clear();changed=true;continue;}}}
        if(i.op==NirOp::CheckBounds&&i.args.size()==2){auto a=known.find(i.args[0]),b=known.find(i.args[1]);if(a!=known.end()&&b!=known.end()){auto index=std::get_if<std::int64_t>(&a->second),length=std::get_if<std::int64_t>(&b->second);if(index&&length&&*index>=0&&*length>=0&&*index<*length){i.op=NirOp::Nop;i.args.clear();changed=true;continue;}}}
        if(i.dest)known.erase(*i.dest);
    }
    return changed;
}
bool dce(NirFunction&fn){std::unordered_set<Reg>used;for(const auto&i:fn.code)for(auto arg:i.args)used.insert(arg);bool changed=false;std::vector<NirInstruction>kept;kept.reserve(fn.code.size());for(auto&i:fn.code){if(i.op==NirOp::Nop){changed=true;continue;}if(i.dest&&!used.count(*i.dest)&&!sideEffect(i)){changed=true;continue;}kept.push_back(std::move(i));}fn.code=std::move(kept);return changed;}
void pipeline(NirFunction&fn,OptimizationLevel level){if(level==OptimizationLevel::O0)return;int rounds=level==OptimizationLevel::O3?3:(level==OptimizationLevel::O2?2:1);for(int r=0;r<rounds;++r){bool a=constants(fn),b=dce(fn);if(!a&&!b)break;}if(level==OptimizationLevel::Oz)dce(fn);}
} void PassManager::optimize(NirProgram&p)const{pipeline(p.entry,level_);for(auto&f:p.functions)pipeline(f,level_);}void Optimizer::optimize(NirProgram&p)const{optimize(p,OptimizationLevel::O1);}void Optimizer::optimize(NirProgram&p,OptimizationLevel level)const{PassManager(level).optimize(p);}void Optimizer::optimizeFunction(NirFunction&fn)const{pipeline(fn,OptimizationLevel::O1);} } // namespace noe
