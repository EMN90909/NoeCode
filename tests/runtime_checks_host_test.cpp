#include "noqeri.hpp"
#include <atomic>
#include <cstdint>
#include <iostream>
#include <limits>
#include <thread>

int main(){
    using namespace noe;
    std::uint64_t cell=0;
    const auto address=reinterpret_cast<std::uintptr_t>(&cell);

    runtimeResetChecks();runtimeConfigureChecks({true,false,false});runtimeRegisterMemory(address,sizeof(cell),"test cell");
    runtimeCheckMemoryWrite(address,sizeof(cell));runtimeCheckMemoryRead(address,sizeof(cell));
    bool memoryRejected=false;try{runtimeCheckMemoryRead(address+sizeof(cell),1);}catch(...){memoryRejected=true;}
    if(!memoryRejected){std::cerr<<"memory checker accepted out-of-range access\n";return 1;}

    runtimeResetChecks();runtimeConfigureChecks({false,false,true});
    bool overflowRejected=false;try{(void)runtimeAddI64(std::numeric_limits<std::int64_t>::max(),1);}catch(...){overflowRejected=true;}
    if(!overflowRejected){std::cerr<<"overflow checker accepted max+1\n";return 1;}
    if(runtimeAddI64(20,22)!=42){std::cerr<<"checked addition changed valid arithmetic\n";return 1;}

    runtimeResetChecks();runtimeConfigureChecks({true,true,false});runtimeRegisterMemory(address,sizeof(cell),"race cell");
    std::atomic<int>ready{0};std::atomic<bool>raceRejected{false};
    auto writer=[&](std::uint64_t value){
        ready.fetch_add(1);while(ready.load()<2)std::this_thread::yield();
        try{runtimeCheckMemoryWrite(address,sizeof(cell));cell=value;}catch(...){raceRejected.store(true);}
    };
    std::thread first(writer,1),second(writer,2);first.join();second.join();
    if(!raceRejected.load()){std::cerr<<"race detector missed conflicting cross-thread writes\n";return 1;}

    runtimeResetChecks();runtimeConfigureChecks({true,true,false});runtimeRegisterMemory(address,sizeof(cell),"synchronized cell");
    runtimeCheckMemoryWrite(address,sizeof(cell));runtimeSynchronizationPoint();runtimeCheckMemoryRead(address,sizeof(cell));

    runtimeConfigureChecks({});runtimeResetChecks();
    std::cout<<"runtime checks: PASS\n";
    return 0;
}
