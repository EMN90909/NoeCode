#pragma once
#include "nir.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace noe {
struct LiveInterval { Reg value=0; std::size_t begin=0; std::size_t end=0; Type type{}; };
struct RegisterLocation { Reg value=0; std::string physical; bool spilled=false; std::size_t spillSlot=0; };
struct AllocationPlan { std::vector<LiveInterval> intervals; std::vector<RegisterLocation> locations; std::size_t spillSlots=0; };
class LinearScanRegisterAllocator {
public:
    AllocationPlan allocate(const NirFunction& function,const std::vector<std::string>& registers={"r12","r13","r14","r15"}) const;
};
} // namespace noe
