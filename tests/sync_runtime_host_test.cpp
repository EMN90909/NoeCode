#include "sync_runtime.hpp"
#include <iostream>

int main(){
    using namespace noe;
    runtimeSyncReset();

    const auto channel=runtimeChannelCreate(1);
    if(channel==0)return 1;
    if(runtimeChannelSendI64(channel,7,0)!=0)return 2;
    if(runtimeChannelCount(channel)!=1)return 3;
    if(runtimeChannelSendI64(channel,8,1)!=-2)return 4;
    if(runtimeChannelReceiveI64(channel,1)!=7)return 5;
    if(!runtimeChannelClose(channel))return 6;
    if(runtimeChannelSendI64(channel,9,0)!=-3)return 7;
    if(!runtimeChannelDestroy(channel))return 8;

    const auto mutex=runtimeMutexCreate();
    if(mutex==0||!runtimeMutexLock(mutex,1))return 9;
    if(runtimeMutexDestroy(mutex))return 10;
    if(!runtimeMutexUnlock(mutex)||!runtimeMutexDestroy(mutex))return 11;

    const auto rw=runtimeRwLockCreate();
    if(rw==0||!runtimeRwLockRead(rw,1))return 12;
    if(runtimeRwLockDestroy(rw))return 13;
    if(!runtimeRwLockUnlock(rw))return 14;
    if(!runtimeRwLockWrite(rw,1))return 15;
    if(!runtimeRwLockUnlock(rw)||!runtimeRwLockDestroy(rw))return 16;

    runtimeSyncReset();
    std::cout<<"sync runtime checks passed\n";
    return 0;
}
