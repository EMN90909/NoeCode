#include "task_scheduler.hpp"
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

int main(){
    using namespace noe;
    runtimeTaskReset();
    auto fast=runtimeTaskSpawn([](const std::atomic_bool&cancelled)->std::int64_t{
        if(cancelled.load())return -3;
        return 42;
    });
    if(fast==0){std::cerr<<"spawn failed\n";return 1;}
    const auto fastResult=runtimeTaskJoin(fast,1000);
    if(fastResult!=42||runtimeTaskState(fast)!=RuntimeTaskState::Ready){std::cerr<<"join/state failed\n";return 2;}
    if(!runtimeTaskDestroy(fast)){std::cerr<<"destroy failed\n";return 3;}

    auto slow=runtimeTaskSpawn([](const std::atomic_bool&cancelled)->std::int64_t{
        for(int i=0;i<100;++i){
            if(cancelled.load())return -3;
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        return 7;
    });
    if(runtimeTaskJoin(slow,1)!=-2){std::cerr<<"timeout contract failed\n";return 4;}
    if(!runtimeTaskCancel(slow)||!runtimeTaskCancelled(slow)){std::cerr<<"cancel signal failed\n";return 5;}
    const auto cancelled=runtimeTaskJoin(slow,1000);
    if(cancelled!=-3){std::cerr<<"cancelled join failed: "<<cancelled<<"\n";return 6;}
    runtimeTaskDestroy(slow);
    runtimeTaskReset();
    std::cout<<"task scheduler checks passed\n";
    return 0;
}
