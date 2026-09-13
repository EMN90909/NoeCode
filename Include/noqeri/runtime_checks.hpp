#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace noe {

struct RuntimeCheckConfig {
    bool memory = false;
    bool race = false;
    bool overflow = false;
};

void runtimeConfigureChecks(RuntimeCheckConfig config);
RuntimeCheckConfig runtimeCheckConfig();
void runtimeResetChecks();

void runtimeRegisterMemory(std::uintptr_t address,std::size_t size,const std::string& label);
void runtimeUnregisterMemory(std::uintptr_t address);
void runtimeCheckMemoryRead(std::uintptr_t address,std::size_t width,bool atomicAccess=false);
void runtimeCheckMemoryWrite(std::uintptr_t address,std::size_t width,bool atomicAccess=false);
void runtimeSynchronizationPoint();

std::int64_t runtimeAddI64(std::int64_t left,std::int64_t right);
std::int64_t runtimeSubI64(std::int64_t left,std::int64_t right);
std::int64_t runtimeMulI64(std::int64_t left,std::int64_t right);
std::int64_t runtimeDivI64(std::int64_t left,std::int64_t right);
std::int64_t runtimeModI64(std::int64_t left,std::int64_t right);
std::int64_t runtimeNegI64(std::int64_t value);

} // namespace noe
