#include "sync_runtime.hpp"
#include "runtime_checks.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <unordered_map>

namespace noe {
namespace {
std::atomic<std::size_t> nextHandle{1};
std::mutex registryMutex;

struct Channel {
    explicit Channel(std::size_t cap):capacity(cap){}
    std::mutex mutex;
    std::condition_variable canSend,canReceive;
    std::deque<std::int64_t> queue;
    std::size_t capacity=1;
    bool closed=false;
};
struct MutexState { std::timed_mutex mutex; std::mutex ownerMutex; std::thread::id owner{}; };
struct RwState {
    std::shared_timed_mutex mutex;
    std::mutex ownerMutex;
    std::thread::id writer{};
    std::unordered_map<std::thread::id,std::size_t> readers;
};
std::unordered_map<std::size_t,std::shared_ptr<Channel>> channels;
std::unordered_map<std::size_t,std::shared_ptr<MutexState>> mutexes;
std::unordered_map<std::size_t,std::shared_ptr<RwState>> rwlocks;

template<class Map>
auto lookup(Map&map,std::size_t handle)->typename Map::mapped_type{std::lock_guard<std::mutex>lock(registryMutex);auto it=map.find(handle);return it==map.end()?typename Map::mapped_type{}:it->second;}
std::size_t allocateHandle(){auto value=nextHandle.fetch_add(1,std::memory_order_relaxed);if(value==0)value=nextHandle.fetch_add(1,std::memory_order_relaxed);return value;}

template<class Predicate>
bool waitUntil(std::condition_variable&cv,std::unique_lock<std::mutex>&lock,std::int64_t timeoutMillis,Predicate predicate){
    if(predicate())return true;
    if(timeoutMillis==0)return false;
    return cv.wait_for(lock,std::chrono::milliseconds(timeoutMillis),predicate);
}
}

std::size_t runtimeChannelCreate(std::size_t capacity){if(capacity==0||capacity>1048576)return 0;auto state=std::make_shared<Channel>(capacity);const auto h=allocateHandle();std::lock_guard<std::mutex>lock(registryMutex);channels.emplace(h,std::move(state));return h;}
std::int64_t runtimeChannelSendI64(std::size_t handle,std::int64_t value,std::int64_t timeoutMillis){
    auto state=lookup(channels,handle);if(!state||timeoutMillis<0)return -1;
    std::unique_lock<std::mutex>lock(state->mutex);
    if(!waitUntil(state->canSend,lock,timeoutMillis,[&]{return state->closed||state->queue.size()<state->capacity;}))return -2;
    if(state->closed)return -3;
    state->queue.push_back(value);lock.unlock();state->canReceive.notify_one();runtimeSynchronizationPoint();return 0;
}
std::int64_t runtimeChannelReceiveI64(std::size_t handle,std::int64_t timeoutMillis){
    auto state=lookup(channels,handle);if(!state||timeoutMillis<0)return 0;
    std::unique_lock<std::mutex>lock(state->mutex);
    if(!waitUntil(state->canReceive,lock,timeoutMillis,[&]{return state->closed||!state->queue.empty();}))return 0;
    if(state->queue.empty())return 0;
    const auto value=state->queue.front();state->queue.pop_front();lock.unlock();state->canSend.notify_one();runtimeSynchronizationPoint();return value;
}
std::size_t runtimeChannelCount(std::size_t handle){auto state=lookup(channels,handle);if(!state)return 0;std::lock_guard<std::mutex>lock(state->mutex);return state->queue.size();}
bool runtimeChannelClose(std::size_t handle){auto state=lookup(channels,handle);if(!state)return false;{std::lock_guard<std::mutex>lock(state->mutex);state->closed=true;}state->canSend.notify_all();state->canReceive.notify_all();runtimeSynchronizationPoint();return true;}
bool runtimeChannelDestroy(std::size_t handle){std::shared_ptr<Channel>state;{std::lock_guard<std::mutex>lock(registryMutex);auto it=channels.find(handle);if(it==channels.end())return false;state=it->second;channels.erase(it);}if(state){std::lock_guard<std::mutex>lock(state->mutex);state->closed=true;state->canSend.notify_all();state->canReceive.notify_all();}return true;}

std::size_t runtimeMutexCreate(){auto state=std::make_shared<MutexState>();const auto h=allocateHandle();std::lock_guard<std::mutex>lock(registryMutex);mutexes.emplace(h,std::move(state));return h;}
bool runtimeMutexLock(std::size_t handle,std::int64_t timeoutMillis){auto state=lookup(mutexes,handle);if(!state||timeoutMillis<0)return false;const bool ok=timeoutMillis==0?state->mutex.try_lock():state->mutex.try_lock_for(std::chrono::milliseconds(timeoutMillis));if(!ok)return false;{std::lock_guard<std::mutex>lock(state->ownerMutex);state->owner=std::this_thread::get_id();}runtimeSynchronizationPoint();return true;}
bool runtimeMutexTryLock(std::size_t handle){return runtimeMutexLock(handle,0);}
bool runtimeMutexUnlock(std::size_t handle){auto state=lookup(mutexes,handle);if(!state)return false;{std::lock_guard<std::mutex>lock(state->ownerMutex);if(state->owner!=std::this_thread::get_id())return false;state->owner={};}runtimeSynchronizationPoint();state->mutex.unlock();return true;}
bool runtimeMutexDestroy(std::size_t handle){
    auto state=lookup(mutexes,handle);if(!state)return false;
    {std::lock_guard<std::mutex>lock(state->ownerMutex);if(state->owner!=std::thread::id{})return false;}
    std::lock_guard<std::mutex>lock(registryMutex);auto it=mutexes.find(handle);if(it==mutexes.end()||it->second!=state)return false;mutexes.erase(it);return true;
}

std::size_t runtimeRwLockCreate(){auto state=std::make_shared<RwState>();const auto h=allocateHandle();std::lock_guard<std::mutex>lock(registryMutex);rwlocks.emplace(h,std::move(state));return h;}
bool runtimeRwLockRead(std::size_t handle,std::int64_t timeoutMillis){auto state=lookup(rwlocks,handle);if(!state||timeoutMillis<0)return false;const bool ok=timeoutMillis==0?state->mutex.try_lock_shared():state->mutex.try_lock_shared_for(std::chrono::milliseconds(timeoutMillis));if(!ok)return false;{std::lock_guard<std::mutex>lock(state->ownerMutex);state->readers[std::this_thread::get_id()]++;}runtimeSynchronizationPoint();return true;}
bool runtimeRwLockWrite(std::size_t handle,std::int64_t timeoutMillis){auto state=lookup(rwlocks,handle);if(!state||timeoutMillis<0)return false;const bool ok=timeoutMillis==0?state->mutex.try_lock():state->mutex.try_lock_for(std::chrono::milliseconds(timeoutMillis));if(!ok)return false;{std::lock_guard<std::mutex>lock(state->ownerMutex);state->writer=std::this_thread::get_id();}runtimeSynchronizationPoint();return true;}
bool runtimeRwLockTryRead(std::size_t handle){return runtimeRwLockRead(handle,0);}
bool runtimeRwLockTryWrite(std::size_t handle){return runtimeRwLockWrite(handle,0);}
bool runtimeRwLockUnlock(std::size_t handle){
    auto state=lookup(rwlocks,handle);if(!state)return false;bool writer=false,reader=false;
    {std::lock_guard<std::mutex>lock(state->ownerMutex);const auto id=std::this_thread::get_id();if(state->writer==id){state->writer={};writer=true;}else{auto it=state->readers.find(id);if(it!=state->readers.end()&&it->second){if(--it->second==0)state->readers.erase(it);reader=true;}}}
    if(!writer&&!reader)return false;runtimeSynchronizationPoint();if(writer)state->mutex.unlock();else state->mutex.unlock_shared();return true;
}
bool runtimeRwLockDestroy(std::size_t handle){
    auto state=lookup(rwlocks,handle);if(!state)return false;
    {std::lock_guard<std::mutex>lock(state->ownerMutex);if(state->writer!=std::thread::id{}||!state->readers.empty())return false;}
    std::lock_guard<std::mutex>lock(registryMutex);auto it=rwlocks.find(handle);if(it==rwlocks.end()||it->second!=state)return false;rwlocks.erase(it);return true;
}

void runtimeSyncReset(){
    std::lock_guard<std::mutex>lock(registryMutex);
    channels.clear();mutexes.clear();rwlocks.clear();
}

} // namespace noe
