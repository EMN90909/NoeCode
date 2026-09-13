#include "noe.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace noe {
namespace {
thread_local std::vector<std::unique_ptr<std::uint8_t[]>> allocations;
thread_local const std::atomic_bool* currentTaskCancellation=nullptr;
std::mutex atomicMutex;

bool truthyValue(const NirValue&v){if(auto p=std::get_if<bool>(&v))return*p;if(auto p=std::get_if<std::int64_t>(&v))return*p!=0;if(auto p=std::get_if<double>(&v))return*p!=0.0;if(auto p=std::get_if<std::string>(&v))return!p->empty();return false;}
std::int64_t asInt(const NirValue&v){if(auto p=std::get_if<std::int64_t>(&v))return*p;if(auto p=std::get_if<bool>(&v))return*p?1:0;if(std::holds_alternative<std::monostate>(v))return 0;throw std::runtime_error("integer or pointer value required");}
double asDouble(const NirValue&v){if(auto p=std::get_if<double>(&v))return*p;if(auto p=std::get_if<std::int64_t>(&v))return static_cast<double>(*p);throw std::runtime_error("numeric value required");}
NirValue binary(const std::string&op,const NirValue&a,const NirValue&b){if(auto x=std::get_if<std::int64_t>(&a))if(auto y=std::get_if<std::int64_t>(&b)){if(op=="+")return runtimeAddI64(*x,*y);if(op=="-")return runtimeSubI64(*x,*y);if(op=="*")return runtimeMulI64(*x,*y);if(op=="/")return runtimeDivI64(*x,*y);if(op=="%")return runtimeModI64(*x,*y);if(op=="==")return*x==*y;if(op=="!=")return*x!=*y;if(op=="<")return*x<*y;if(op=="<=")return*x<=*y;if(op==">")return*x>*y;if(op==">=")return*x>=*y;}if((std::holds_alternative<double>(a)||std::holds_alternative<std::int64_t>(a))&&(std::holds_alternative<double>(b)||std::holds_alternative<std::int64_t>(b))){double x=asDouble(a),y=asDouble(b);if(op=="+")return x+y;if(op=="-")return x-y;if(op=="*")return x*y;if(op=="/")return x/y;if(op=="==")return x==y;if(op=="!=")return x!=y;if(op=="<")return x<y;if(op=="<=")return x<=y;if(op==">")return x>y;if(op==">=")return x>=y;}if(auto x=std::get_if<std::string>(&a))if(auto y=std::get_if<std::string>(&b)){if(op=="+")return*x+*y;if(op=="==")return*x==*y;if(op=="!=")return*x!=*y;}if(op=="&&")return truthyValue(a)&&truthyValue(b);if(op=="||")return truthyValue(a)||truthyValue(b);throw std::runtime_error("unsupported binary operation "+op);}
std::uint64_t rawLoad(std::uintptr_t address,std::size_t width,bool vol,bool atomicAccess=false){if(address==0)throw std::runtime_error("null pointer dereference");runtimeCheckMemoryRead(address,width,atomicAccess);switch(width){case 1:return vol?*reinterpret_cast<volatile std::uint8_t*>(address):*reinterpret_cast<std::uint8_t*>(address);case 2:return vol?*reinterpret_cast<volatile std::uint16_t*>(address):*reinterpret_cast<std::uint16_t*>(address);case 4:return vol?*reinterpret_cast<volatile std::uint32_t*>(address):*reinterpret_cast<std::uint32_t*>(address);case 8:return vol?*reinterpret_cast<volatile std::uint64_t*>(address):*reinterpret_cast<std::uint64_t*>(address);default:throw std::runtime_error("unsupported memory load width");}}
void rawStore(std::uintptr_t address,std::size_t width,std::uint64_t value,bool vol,bool atomicAccess=false){if(address==0)throw std::runtime_error("null pointer store");runtimeCheckMemoryWrite(address,width,atomicAccess);switch(width){case 1:if(vol)*reinterpret_cast<volatile std::uint8_t*>(address)=static_cast<std::uint8_t>(value);else*reinterpret_cast<std::uint8_t*>(address)=static_cast<std::uint8_t>(value);break;case 2:if(vol)*reinterpret_cast<volatile std::uint16_t*>(address)=static_cast<std::uint16_t>(value);else*reinterpret_cast<std::uint16_t*>(address)=static_cast<std::uint16_t>(value);break;case 4:if(vol)*reinterpret_cast<volatile std::uint32_t*>(address)=static_cast<std::uint32_t>(value);else*reinterpret_cast<std::uint32_t*>(address)=static_cast<std::uint32_t>(value);break;case 8:if(vol)*reinterpret_cast<volatile std::uint64_t*>(address)=static_cast<std::uint64_t>(value);else*reinterpret_cast<std::uint64_t*>(address)=static_cast<std::uint64_t>(value);break;default:throw std::runtime_error("unsupported memory store width");}}
std::uintptr_t allocateBytes(std::size_t bytes){const auto size=std::max<std::size_t>(bytes,1);auto p=std::make_unique<std::uint8_t[]>(size);std::memset(p.get(),0,size);auto address=reinterpret_cast<std::uintptr_t>(p.get());runtimeRegisterMemory(address,size,"runtime allocation");allocations.push_back(std::move(p));return address;}
struct FrameMemory { std::vector<std::uintptr_t> addresses; ~FrameMemory(){for(auto address:addresses)runtimeUnregisterMemory(address);} void add(std::uintptr_t address){if(std::find(addresses.begin(),addresses.end(),address)==addresses.end()){addresses.push_back(address);runtimeRegisterMemory(address,sizeof(std::uint64_t),"interpreter local");}} };
bool stringValue(const NirValue&value,std::string&out){if(auto p=std::get_if<std::string>(&value)){out=*p;return true;}return false;}
}

std::string Interpreter::valueToString(const NirValue&v)const{if(std::holds_alternative<std::monostate>(v))return"null";if(auto p=std::get_if<std::int64_t>(&v))return std::to_string(*p);if(auto p=std::get_if<double>(&v))return std::to_string(*p);if(auto p=std::get_if<bool>(&v))return*p?"true":"false";return std::get<std::string>(v);}
int Interpreter::run(const NirProgram&p){try{allocations.clear();runtimeTaskReset();runtimeSyncReset();runtimeResetChecks();runFunction(p,p.entry,{});runtimeTaskJoinAll();runtimeTaskReset();runtimeSyncReset();allocations.clear();runtimeResetChecks();return 0;}catch(const std::exception&e){runtimeTaskReset();runtimeSyncReset();allocations.clear();runtimeResetChecks();std::cerr<<"NQR-R4000: runtime error: "<<e.what()<<"\n";return 1;}}
NirValue Interpreter::runFunction(const NirProgram&p,const NirFunction&fn,const std::vector<NirValue>&argv){
    if(fn.isExtern)throw std::runtime_error("cannot directly interpret extern function "+fn.name);
    RuntimeProfileScope profileScope(fn.name);
    std::unordered_map<std::string,NirValue>vars;std::unordered_map<std::string,std::uint64_t>cells;std::unordered_set<std::string>addressTaken;std::vector<NirValue>regs(fn.nextReg);FrameMemory frameMemory;
    for(std::size_t i=0;i<fn.params.size()&&i<argv.size();++i){vars[fn.params[i]]=argv[i];if(std::holds_alternative<std::int64_t>(argv[i]))cells[fn.params[i]]=static_cast<std::uint64_t>(std::get<std::int64_t>(argv[i]));}
    std::size_t pc=0;
    while(pc<fn.code.size()){
        const auto&i=fn.code[pc];auto get=[&](Reg r)->const NirValue&{if(r>=regs.size())throw std::runtime_error("invalid NIR register");return regs[r];};
        switch(i.op){
            case NirOp::Const:regs[*i.dest]=i.literal;break;
            case NirOp::Load:{if(addressTaken.count(i.text)){regs[*i.dest]=static_cast<std::int64_t>(cells[i.text]);break;}auto it=vars.find(i.text);if(it==vars.end())throw std::runtime_error("undefined variable "+i.text);regs[*i.dest]=it->second;break;}
            case NirOp::Store:{vars[i.text]=get(i.args[0]);if(auto n=std::get_if<std::int64_t>(&vars[i.text]))cells[i.text]=static_cast<std::uint64_t>(*n);else if(auto b=std::get_if<bool>(&vars[i.text]))cells[i.text]=*b?1:0;break;}
            case NirOp::AddressOf:addressTaken.insert(i.text);cells.try_emplace(i.text,0);frameMemory.add(reinterpret_cast<std::uintptr_t>(&cells[i.text]));regs[*i.dest]=static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(&cells[i.text]));break;
            case NirOp::LoadMemory:{auto address=static_cast<std::uintptr_t>(asInt(get(i.args[0])));regs[*i.dest]=static_cast<std::int64_t>(rawLoad(address,i.width,i.isVolatile));break;}
            case NirOp::StoreMemory:{auto address=static_cast<std::uintptr_t>(asInt(get(i.args[0])));rawStore(address,i.width,static_cast<std::uint64_t>(asInt(get(i.args[1]))),i.isVolatile);break;}
            case NirOp::PtrOffset:{std::int64_t base=asInt(get(i.args[0]));std::int64_t delta=static_cast<std::int64_t>(i.target);if(i.args.size()>1)delta=runtimeMulI64(asInt(get(i.args[1])),static_cast<std::int64_t>(i.width));regs[*i.dest]=runtimeAddI64(base,delta);break;}
            case NirOp::StackAlloc:regs[*i.dest]=static_cast<std::int64_t>(allocateBytes(i.width));break;
            case NirOp::MakeSlice:{auto descriptor=allocateBytes(16);rawStore(descriptor,8,static_cast<std::uint64_t>(asInt(get(i.args[0]))),false);rawStore(descriptor+8,8,static_cast<std::uint64_t>(asInt(get(i.args[1]))),false);regs[*i.dest]=static_cast<std::int64_t>(descriptor);break;}
            case NirOp::SliceData:{auto descriptor=static_cast<std::uintptr_t>(asInt(get(i.args[0])));regs[*i.dest]=static_cast<std::int64_t>(rawLoad(descriptor,8,false));break;}
            case NirOp::SliceLen:{auto descriptor=static_cast<std::uintptr_t>(asInt(get(i.args[0])));regs[*i.dest]=static_cast<std::int64_t>(rawLoad(descriptor+8,8,false));break;}
            case NirOp::AtomicLoad:{std::lock_guard<std::mutex> lock(atomicMutex);auto address=static_cast<std::uintptr_t>(asInt(get(i.args[0])));regs[*i.dest]=static_cast<std::int64_t>(rawLoad(address,i.width,true,true));runtimeSynchronizationPoint();break;}
            case NirOp::AtomicStore:{std::lock_guard<std::mutex> lock(atomicMutex);auto address=static_cast<std::uintptr_t>(asInt(get(i.args[0])));rawStore(address,i.width,static_cast<std::uint64_t>(asInt(get(i.args[1]))),true,true);runtimeSynchronizationPoint();break;}
            case NirOp::AtomicExchange:{std::lock_guard<std::mutex> lock(atomicMutex);auto address=static_cast<std::uintptr_t>(asInt(get(i.args[0])));auto old=rawLoad(address,i.width,true,true);rawStore(address,i.width,static_cast<std::uint64_t>(asInt(get(i.args[1]))),true,true);runtimeSynchronizationPoint();regs[*i.dest]=static_cast<std::int64_t>(old);break;}
            case NirOp::AtomicCompareExchange:{std::lock_guard<std::mutex> lock(atomicMutex);auto address=static_cast<std::uintptr_t>(asInt(get(i.args[0])));auto old=rawLoad(address,i.width,true,true);auto expected=static_cast<std::uint64_t>(asInt(get(i.args[1])));if(old==expected)rawStore(address,i.width,static_cast<std::uint64_t>(asInt(get(i.args[2]))),true,true);runtimeSynchronizationPoint();regs[*i.dest]=static_cast<std::int64_t>(old);break;}
            case NirOp::AtomicFence:std::atomic_thread_fence(std::memory_order_seq_cst);runtimeSynchronizationPoint();if(i.dest)regs[*i.dest]=std::monostate{};break;
            case NirOp::Intrinsic:{if(i.text=="x86.rdtsc"){auto now=std::chrono::high_resolution_clock::now().time_since_epoch().count();regs[*i.dest]=static_cast<std::int64_t>(now);}else if(i.text=="compiler.fence")std::atomic_signal_fence(std::memory_order_seq_cst);else if(i.text=="x86.pause"){}else if(i.text=="x86.halt")throw std::runtime_error("x86.halt is not executable in the reference interpreter");else throw std::runtime_error("unknown intrinsic "+i.text);if(i.dest&&i.text!="x86.rdtsc")regs[*i.dest]=std::monostate{};break;}
            case NirOp::InlineAsm:if(i.dest)regs[*i.dest]=std::monostate{};break;
            case NirOp::Try:{auto value=asInt(get(i.args[0]));if(value<0)return NirValue(value);regs[*i.dest]=value;break;}
            case NirOp::Throw:{auto code=asInt(get(i.args[0]));return NirValue(code<0?code:-(code+1));}
            case NirOp::Cast:{auto v=get(i.args[0]);Type t=typeFromName(i.text);if(t.kind==TypeKind::Float)regs[*i.dest]=asDouble(v);else if(t.isInteger()||t.kind==TypeKind::Pointer)regs[*i.dest]=asInt(v);else regs[*i.dest]=v;break;}
            case NirOp::Unary:{auto v=get(i.args[0]);if(i.text=="!")regs[*i.dest]=!truthyValue(v);else if(i.text=="-"){if(auto n=std::get_if<std::int64_t>(&v))regs[*i.dest]=runtimeNegI64(*n);else regs[*i.dest]=-asDouble(v);}else regs[*i.dest]=v;break;}
            case NirOp::Binary:regs[*i.dest]=binary(i.text,get(i.args[0]),get(i.args[1]));break;
            case NirOp::Call:{
                std::vector<NirValue>a;for(auto r:i.args)a.push_back(get(r));
                if(i.text=="taskHostSpawn"){
                    if(a.size()<2)throw std::runtime_error("taskHostSpawn expects entry and payload strings");
                    std::string entry,payload;if(!stringValue(a[0],entry)||!stringValue(a[1],payload))throw std::runtime_error("taskHostSpawn expects entry and payload strings");
                    auto taskFn=std::find_if(p.functions.begin(),p.functions.end(),[&](const auto&x){return x.name==entry&&!x.isExtern;});
                    if(taskFn==p.functions.end())throw std::runtime_error("task entry not found: "+entry);
                    const NirFunction* fnPtr=&*taskFn;
                    const auto handle=runtimeTaskSpawn([this,&p,fnPtr,payload](const std::atomic_bool&cancelled)->std::int64_t{
                        currentTaskCancellation=&cancelled;allocations.clear();
                        try{if(cancelled.load(std::memory_order_acquire)){currentTaskCancellation=nullptr;return -3;}std::vector<NirValue> taskArgs;if(!fnPtr->params.empty())taskArgs.emplace_back(payload);auto value=runFunction(p,*fnPtr,taskArgs);allocations.clear();currentTaskCancellation=nullptr;return asInt(value);}catch(...){allocations.clear();currentTaskCancellation=nullptr;throw;}
                    });
                    regs[*i.dest]=static_cast<std::int64_t>(handle);break;
                }
                if(i.text=="taskHostState"){regs[*i.dest]=static_cast<std::int64_t>(runtimeTaskState(static_cast<std::size_t>(asInt(a.at(0)))));break;}
                if(i.text=="taskHostJoin"){regs[*i.dest]=runtimeTaskJoin(static_cast<std::size_t>(asInt(a.at(0))),asInt(a.at(1)));break;}
                if(i.text=="taskHostCancel"){regs[*i.dest]=runtimeTaskCancel(static_cast<std::size_t>(asInt(a.at(0))));break;}
                if(i.text=="taskHostDestroy"){regs[*i.dest]=runtimeTaskDestroy(static_cast<std::size_t>(asInt(a.at(0))));break;}
                if(i.text=="taskHostCancelled"){regs[*i.dest]=runtimeTaskCancelled(static_cast<std::size_t>(asInt(a.at(0))));break;}
                if(i.text=="taskHostCurrentCancelled"){regs[*i.dest]=currentTaskCancellation&&currentTaskCancellation->load(std::memory_order_acquire);break;}
                if(i.text=="channelHostCreate"){regs[*i.dest]=static_cast<std::int64_t>(runtimeChannelCreate(static_cast<std::size_t>(asInt(a.at(0)))));break;}
                if(i.text=="channelHostSendI64"){regs[*i.dest]=runtimeChannelSendI64(static_cast<std::size_t>(asInt(a.at(0))),asInt(a.at(1)),asInt(a.at(2)));break;}
                if(i.text=="channelHostReceiveI64"){regs[*i.dest]=runtimeChannelReceiveI64(static_cast<std::size_t>(asInt(a.at(0))),asInt(a.at(1)));break;}
                if(i.text=="channelHostCount"){regs[*i.dest]=static_cast<std::int64_t>(runtimeChannelCount(static_cast<std::size_t>(asInt(a.at(0)))));break;}
                if(i.text=="channelHostClose"){regs[*i.dest]=runtimeChannelClose(static_cast<std::size_t>(asInt(a.at(0))));break;}
                if(i.text=="channelHostDestroy"){regs[*i.dest]=runtimeChannelDestroy(static_cast<std::size_t>(asInt(a.at(0))));break;}
                if(i.text=="mutexHostCreate"){regs[*i.dest]=static_cast<std::int64_t>(runtimeMutexCreate());break;}
                if(i.text=="mutexHostLock"){regs[*i.dest]=runtimeMutexLock(static_cast<std::size_t>(asInt(a.at(0))),asInt(a.at(1)));break;}
                if(i.text=="mutexHostTryLock"){regs[*i.dest]=runtimeMutexTryLock(static_cast<std::size_t>(asInt(a.at(0))));break;}
                if(i.text=="mutexHostUnlock"){regs[*i.dest]=runtimeMutexUnlock(static_cast<std::size_t>(asInt(a.at(0))));break;}
                if(i.text=="mutexHostDestroy"){regs[*i.dest]=runtimeMutexDestroy(static_cast<std::size_t>(asInt(a.at(0))));break;}
                if(i.text=="rwlockHostCreate"){regs[*i.dest]=static_cast<std::int64_t>(runtimeRwLockCreate());break;}
                if(i.text=="rwlockHostReadLock"){regs[*i.dest]=runtimeRwLockRead(static_cast<std::size_t>(asInt(a.at(0))),asInt(a.at(1)));break;}
                if(i.text=="rwlockHostWriteLock"){regs[*i.dest]=runtimeRwLockWrite(static_cast<std::size_t>(asInt(a.at(0))),asInt(a.at(1)));break;}
                if(i.text=="rwlockHostTryRead"){regs[*i.dest]=runtimeRwLockTryRead(static_cast<std::size_t>(asInt(a.at(0))));break;}
                if(i.text=="rwlockHostTryWrite"){regs[*i.dest]=runtimeRwLockTryWrite(static_cast<std::size_t>(asInt(a.at(0))));break;}
                if(i.text=="rwlockHostUnlock"){regs[*i.dest]=runtimeRwLockUnlock(static_cast<std::size_t>(asInt(a.at(0))));break;}
                if(i.text=="rwlockHostDestroy"){regs[*i.dest]=runtimeRwLockDestroy(static_cast<std::size_t>(asInt(a.at(0))));break;}
                auto user=std::find_if(p.functions.begin(),p.functions.end(),[&](const auto&x){return x.name==i.text&&!x.isExtern;});if(user!=p.functions.end()){regs[*i.dest]=runFunction(p,*user,a);break;}std::string error;if(i.text=="host"||i.text=="abi"){if(a.empty()||!std::holds_alternative<std::string>(a[0]))throw std::runtime_error(i.text+" requires a string service name");std::string service=std::get<std::string>(a[0]);std::vector<NirValue>serviceArgs(a.begin()+1,a.end());std::optional<NirValue>result;if(host_)result=callNoqeriAbi(host_,service,serviceArgs,error);if(!result)result=callNoqeriAbi(defaultNoqeriAbi(),service,serviceArgs,error);if(!result)throw std::runtime_error(error.empty()?"unknown ABI service "+service:error);regs[*i.dest]=*result;break;}std::optional<NirValue>result;if(host_)result=callNoqeriAbi(host_,i.text,a,error);if(!result)result=callNoqeriAbi(defaultNoqeriAbi(),i.text,a,error);if(!result)throw std::runtime_error(error.empty()?"unknown function "+i.text:error);regs[*i.dest]=*result;break;}
            case NirOp::Jump:pc=i.target;continue;
            case NirOp::JumpIfFalse:if(!truthyValue(get(i.args[0]))){pc=i.target;continue;}break;
            case NirOp::Return:return i.args.empty()?NirValue(std::monostate{}):get(i.args[0]);
            case NirOp::Nop:break;
        }
        ++pc;
    }
    return std::monostate{};
}
} // namespace noe
