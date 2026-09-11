#include "noe.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <unordered_map>

namespace noe {
namespace {
bool truthyValue(const NirValue& v){ if(auto p=std::get_if<bool>(&v))return *p; if(auto p=std::get_if<std::int64_t>(&v))return *p!=0; if(auto p=std::get_if<double>(&v))return *p!=0.0; if(auto p=std::get_if<std::string>(&v))return !p->empty(); return false; }
double asDouble(const NirValue& v){ if(auto p=std::get_if<double>(&v))return *p; if(auto p=std::get_if<std::int64_t>(&v))return static_cast<double>(*p); throw std::runtime_error("numeric value required"); }
NirValue binary(const std::string& op,const NirValue&a,const NirValue&b){
    if(auto x=std::get_if<std::int64_t>(&a)) if(auto y=std::get_if<std::int64_t>(&b)){
        if(op=="+")return *x+*y; if(op=="-")return *x-*y; if(op=="*")return *x**y; if(op=="/")return *x / *y; if(op=="%")return *x % *y;
        if(op=="==")return *x==*y; if(op=="!=")return *x!=*y; if(op=="<")return *x<*y; if(op=="<=")return *x<=*y; if(op==">")return *x>*y; if(op==">=")return *x>=*y;
    }
    if((std::holds_alternative<double>(a)||std::holds_alternative<std::int64_t>(a))&&(std::holds_alternative<double>(b)||std::holds_alternative<std::int64_t>(b))){
        double x=asDouble(a),y=asDouble(b); if(op=="+")return x+y;if(op=="-")return x-y;if(op=="*")return x*y;if(op=="/")return x/y;if(op=="==")return x==y;if(op=="!=")return x!=y;if(op=="<")return x<y;if(op=="<=")return x<=y;if(op==">")return x>y;if(op==">=")return x>=y;
    }
    if(auto x=std::get_if<std::string>(&a)) if(auto y=std::get_if<std::string>(&b)){ if(op=="+")return *x+*y;if(op=="==")return *x==*y;if(op=="!=")return *x!=*y; }
    if(op=="&&")return truthyValue(a)&&truthyValue(b); if(op=="||")return truthyValue(a)||truthyValue(b);
    throw std::runtime_error("unsupported binary operation "+op);
}
}

std::string Interpreter::valueToString(const NirValue& v) const {
    if(std::holds_alternative<std::monostate>(v))return "null";
    if(auto p=std::get_if<std::int64_t>(&v))return std::to_string(*p);
    if(auto p=std::get_if<double>(&v))return std::to_string(*p);
    if(auto p=std::get_if<bool>(&v))return *p?"true":"false";
    return std::get<std::string>(v);
}

int Interpreter::run(const NirProgram& p){
    try{ runFunction(p,p.entry,{}); return 0; }
    catch(const std::exception& e){ std::cerr<<"NOE-R4000: runtime error: "<<e.what()<<"\n"; return 1; }
}

NirValue Interpreter::runFunction(const NirProgram& p,const NirFunction& fn,const std::vector<NirValue>& argv){
    std::unordered_map<std::string,NirValue> vars;
    std::vector<NirValue> regs(fn.nextReg);
    for(std::size_t i=0;i<fn.params.size()&&i<argv.size();++i)vars[fn.params[i]]=argv[i];
    std::size_t pc=0;
    while(pc<fn.code.size()){
        const auto&i=fn.code[pc];
        auto get=[&](Reg r)->const NirValue&{ if(r>=regs.size())throw std::runtime_error("invalid NIR register"); return regs[r]; };
        switch(i.op){
            case NirOp::Const: regs[*i.dest]=i.literal; break;
            case NirOp::Load:{ auto it=vars.find(i.text); if(it==vars.end())throw std::runtime_error("undefined variable "+i.text); regs[*i.dest]=it->second; break; }
            case NirOp::Store: vars[i.text]=get(i.args[0]); break;
            case NirOp::Unary:{ auto v=get(i.args[0]); if(i.text=="!")regs[*i.dest]=!truthyValue(v); else if(i.text=="-"){ if(auto n=std::get_if<std::int64_t>(&v))regs[*i.dest]=-*n; else regs[*i.dest]=-asDouble(v); } else regs[*i.dest]=v; break; }
            case NirOp::Binary: regs[*i.dest]=binary(i.text,get(i.args[0]),get(i.args[1])); break;
            case NirOp::Call:{
                std::vector<NirValue>a; for(auto r:i.args)a.push_back(get(r));
                if(i.text=="print"){
                    for(std::size_t x=0;x<a.size();++x){ if(x)std::cout<<' '; std::cout<<valueToString(a[x]); }
                    std::cout<<'\n'; regs[*i.dest]=std::monostate{};
                }else{
                    auto f=std::find_if(p.functions.begin(),p.functions.end(),[&](const auto&x){return x.name==i.text;});
                    if(f==p.functions.end())throw std::runtime_error("unknown function "+i.text);
                    regs[*i.dest]=runFunction(p,*f,a);
                }
                break;
            }
            case NirOp::Jump: pc=i.target; continue;
            case NirOp::JumpIfFalse: if(!truthyValue(get(i.args[0]))){pc=i.target;continue;} break;
            case NirOp::Return: return i.args.empty()?NirValue(std::monostate{}):get(i.args[0]);
            case NirOp::Nop: break;
        }
        ++pc;
    }
    return std::monostate{};
}

} // namespace noe
