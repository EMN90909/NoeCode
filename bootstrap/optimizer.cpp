#include "noe.hpp"
#include <optional>
#include <unordered_map>

namespace noe {
namespace {
bool truthy(const NirValue& v){ if(auto p=std::get_if<bool>(&v))return *p; if(auto p=std::get_if<std::int64_t>(&v))return *p!=0; if(auto p=std::get_if<double>(&v))return *p!=0.0; if(auto p=std::get_if<std::string>(&v))return !p->empty(); return false; }
std::optional<NirValue> fold(const std::string& op,const NirValue&a,const NirValue&b){
    if(auto x=std::get_if<std::int64_t>(&a)) if(auto y=std::get_if<std::int64_t>(&b)){
        if(op=="+")return NirValue(*x+*y); if(op=="-")return NirValue(*x-*y); if(op=="*")return NirValue(*x**y);
        if(op=="/"&&*y!=0)return NirValue(*x / *y); if(op=="%"&&*y!=0)return NirValue(*x % *y);
        if(op=="==")return NirValue(*x==*y); if(op=="!=")return NirValue(*x!=*y); if(op=="<")return NirValue(*x<*y); if(op=="<=")return NirValue(*x<=*y); if(op==">")return NirValue(*x>*y); if(op==">=")return NirValue(*x>=*y);
    }
    if(auto x=std::get_if<double>(&a)) if(auto y=std::get_if<double>(&b)){
        if(op=="+")return NirValue(*x+*y); if(op=="-")return NirValue(*x-*y); if(op=="*")return NirValue(*x**y); if(op=="/"&&*y!=0)return NirValue(*x / *y);
        if(op=="==")return NirValue(*x==*y); if(op=="!=")return NirValue(*x!=*y); if(op=="<")return NirValue(*x<*y); if(op=="<=")return NirValue(*x<=*y); if(op==">")return NirValue(*x>*y); if(op==">=")return NirValue(*x>=*y);
    }
    if(auto x=std::get_if<std::string>(&a)) if(auto y=std::get_if<std::string>(&b)){ if(op=="+")return NirValue(*x+*y); if(op=="==")return NirValue(*x==*y); if(op=="!=")return NirValue(*x!=*y); }
    if(op=="&&")return NirValue(truthy(a)&&truthy(b));
    if(op=="||")return NirValue(truthy(a)||truthy(b));
    return std::nullopt;
}
}

void Optimizer::optimize(NirProgram& p) const { optimizeFunction(p.entry); for(auto& f:p.functions) optimizeFunction(f); }
void Optimizer::optimizeFunction(NirFunction& fn) const {
    std::unordered_map<Reg,NirValue> constants;
    for(auto& i:fn.code){
        if(i.op==NirOp::Const && i.dest){ constants[*i.dest]=i.literal; continue; }
        if(i.op==NirOp::Binary && i.dest && i.args.size()==2){
            auto a=constants.find(i.args[0]), b=constants.find(i.args[1]);
            if(a!=constants.end()&&b!=constants.end()) if(auto v=fold(i.text,a->second,b->second)){
                i.op=NirOp::Const; i.literal=*v; i.text.clear(); i.args.clear(); constants[*i.dest]=*v; continue;
            }
        }
        if(i.dest) constants.erase(*i.dest);
    }
}

} // namespace noe
