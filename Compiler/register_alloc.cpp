#include "noe.hpp"
#include <algorithm>
#include <limits>
#include <unordered_map>

namespace noe {
AllocationPlan LinearScanRegisterAllocator::allocate(const NirFunction&fn,const std::vector<std::string>&registers)const{
    AllocationPlan plan;std::unordered_map<Reg,std::size_t>index;
    for(std::size_t pc=0;pc<fn.code.size();++pc){const auto&in=fn.code[pc];if(in.dest){LiveInterval v;v.value=*in.dest;v.begin=pc;v.end=pc;v.type=*in.dest<fn.registerTypes.size()?fn.registerTypes[*in.dest]:Type{};index[v.value]=plan.intervals.size();plan.intervals.push_back(v);}for(Reg arg:in.args){auto it=index.find(arg);if(it!=index.end())plan.intervals[it->second].end=std::max(plan.intervals[it->second].end,pc);}}
    std::sort(plan.intervals.begin(),plan.intervals.end(),[](const auto&a,const auto&b){return a.begin<b.begin||(a.begin==b.begin&&a.value<b.value);});
    struct Active{std::size_t interval;std::string reg;};std::vector<Active>active;std::vector<std::string>free=registers;std::unordered_map<Reg,RegisterLocation>locations;
    auto spill=[&](const LiveInterval&iv){RegisterLocation loc;loc.value=iv.value;loc.spilled=true;loc.spillSlot=plan.spillSlots++;locations[iv.value]=loc;};
    for(std::size_t current=0;current<plan.intervals.size();++current){const auto&iv=plan.intervals[current];for(auto it=active.begin();it!=active.end();){if(plan.intervals[it->interval].end<iv.begin){free.push_back(it->reg);it=active.erase(it);}else ++it;}if(iv.type.size()>8||free.empty()){if(!active.empty()&&iv.type.size()<=8){auto farthest=std::max_element(active.begin(),active.end(),[&](const auto&a,const auto&b){return plan.intervals[a.interval].end<plan.intervals[b.interval].end;});if(plan.intervals[farthest->interval].end>iv.end){auto evicted=plan.intervals[farthest->interval];auto reg=farthest->reg;spill(evicted);*farthest=Active{current,reg};locations[iv.value]=RegisterLocation{iv.value,reg,false,0};continue;}}spill(iv);continue;}auto reg=free.back();free.pop_back();active.push_back({current,reg});locations[iv.value]=RegisterLocation{iv.value,reg,false,0};}
    for(const auto&iv:plan.intervals){auto it=locations.find(iv.value);if(it!=locations.end())plan.locations.push_back(it->second);}return plan;
}
} // namespace noe
