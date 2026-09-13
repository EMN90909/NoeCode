#include "noe.hpp"
#include <algorithm>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <vector>

namespace noe {
namespace {
struct MemoryRegion { std::uintptr_t start=0; std::size_t size=0; std::string label; };
struct Access { std::thread::id thread; bool write=false; std::uint64_t epoch=0; };
RuntimeCheckConfig config;
std::mutex stateMutex;
std::vector<MemoryRegion> regions;
std::unordered_map<std::uintptr_t,Access> accesses;
std::uint64_t epoch=1;

bool contains(const MemoryRegion&r,std::uintptr_t address,std::size_t width){
    if(width==0)return false;
    if(address<r.start)return false;
    const auto offset=address-r.start;
    return offset<=r.size && width<=r.size-offset;
}

void checkedAccess(std::uintptr_t address,std::size_t width,bool write,bool atomicAccess){
    std::lock_guard<std::mutex>lock(stateMutex);
    if(config.memory){
        const bool valid=std::any_of(regions.begin(),regions.end(),[&](const auto&r){return contains(r,address,width);});
        if(!valid)throw std::runtime_error("memory check: invalid "+std::string(write?"write":"read")+" at address "+std::to_string(address)+" width="+std::to_string(width));
    }
    if(!config.race)return;
    if(atomicAccess){++epoch;accesses.erase(address);return;}
    const auto current=std::this_thread::get_id();
    for(std::size_t n=0;n<width;++n){
        const auto key=address+n;
        auto it=accesses.find(key);
        if(it!=accesses.end()&&it->second.thread!=current&&(write||it->second.write)&&it->second.epoch==epoch){
            throw std::runtime_error("race detector: conflicting unsynchronized memory access at address "+std::to_string(key));
        }
        accesses[key]=Access{current,write,epoch};
    }
}

std::runtime_error overflow(const char*operation){return std::runtime_error(std::string("overflow check: signed integer ")+operation+" overflow");}
}

void runtimeConfigureChecks(RuntimeCheckConfig next){std::lock_guard<std::mutex>lock(stateMutex);config=next;}
RuntimeCheckConfig runtimeCheckConfig(){std::lock_guard<std::mutex>lock(stateMutex);return config;}
void runtimeResetChecks(){std::lock_guard<std::mutex>lock(stateMutex);regions.clear();accesses.clear();epoch=1;}
void runtimeRegisterMemory(std::uintptr_t address,std::size_t size,const std::string&label){if(!address||!size)return;std::lock_guard<std::mutex>lock(stateMutex);regions.push_back({address,size,label});}
void runtimeUnregisterMemory(std::uintptr_t address){std::lock_guard<std::mutex>lock(stateMutex);regions.erase(std::remove_if(regions.begin(),regions.end(),[&](const auto&r){return r.start==address;}),regions.end());for(auto it=accesses.begin();it!=accesses.end();){if(it->first>=address&&std::any_of(regions.begin(),regions.end(),[&](const auto&r){return contains(r,it->first,1);}))++it;else if(it->first>=address)it=accesses.erase(it);else ++it;}}
void runtimeCheckMemoryRead(std::uintptr_t address,std::size_t width,bool atomicAccess){checkedAccess(address,width,false,atomicAccess);}
void runtimeCheckMemoryWrite(std::uintptr_t address,std::size_t width,bool atomicAccess){checkedAccess(address,width,true,atomicAccess);}
void runtimeSynchronizationPoint(){std::lock_guard<std::mutex>lock(stateMutex);++epoch;accesses.clear();}

std::int64_t runtimeAddI64(std::int64_t a,std::int64_t b){if(!runtimeCheckConfig().overflow)return a+b;if((b>0&&a>std::numeric_limits<std::int64_t>::max()-b)||(b<0&&a<std::numeric_limits<std::int64_t>::min()-b))throw overflow("addition");return a+b;}
std::int64_t runtimeSubI64(std::int64_t a,std::int64_t b){if(!runtimeCheckConfig().overflow)return a-b;if((b<0&&a>std::numeric_limits<std::int64_t>::max()+b)||(b>0&&a<std::numeric_limits<std::int64_t>::min()+b))throw overflow("subtraction");return a-b;}
std::int64_t runtimeMulI64(std::int64_t a,std::int64_t b){
    if(!runtimeCheckConfig().overflow)return a*b;
    if(a==0||b==0)return 0;
    if(a==-1&&b==std::numeric_limits<std::int64_t>::min())throw overflow("multiplication");
    if(b==-1&&a==std::numeric_limits<std::int64_t>::min())throw overflow("multiplication");
    if(a>0){if(b>0){if(a>std::numeric_limits<std::int64_t>::max()/b)throw overflow("multiplication");}else if(b<std::numeric_limits<std::int64_t>::min()/a)throw overflow("multiplication");}
    else {if(b>0){if(a<std::numeric_limits<std::int64_t>::min()/b)throw overflow("multiplication");}else if(a!=0&&b<std::numeric_limits<std::int64_t>::max()/a)throw overflow("multiplication");}
    return a*b;
}
std::int64_t runtimeDivI64(std::int64_t a,std::int64_t b){if(b==0)throw std::runtime_error("division by zero");if(runtimeCheckConfig().overflow&&a==std::numeric_limits<std::int64_t>::min()&&b==-1)throw overflow("division");return a/b;}
std::int64_t runtimeModI64(std::int64_t a,std::int64_t b){if(b==0)throw std::runtime_error("modulo by zero");if(runtimeCheckConfig().overflow&&a==std::numeric_limits<std::int64_t>::min()&&b==-1)throw overflow("modulo");return a%b;}
std::int64_t runtimeNegI64(std::int64_t value){if(runtimeCheckConfig().overflow&&value==std::numeric_limits<std::int64_t>::min())throw overflow("negation");return -value;}

} // namespace noe
