#include "task_scheduler.hpp"
#include "runtime_checks.hpp"
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace noe {
namespace {
struct TaskControl {
    std::mutex mutex;
    std::condition_variable cv;
    std::thread worker;
    std::atomic_bool cancelled{false};
    RuntimeTaskState state=RuntimeTaskState::Pending;
    std::int64_t result=0;
};
std::mutex tasksMutex;
std::unordered_map<std::size_t,std::shared_ptr<TaskControl>> tasks;
std::size_t nextTask=1;

std::shared_ptr<TaskControl> getTask(std::size_t handle){
    std::lock_guard<std::mutex> lock(tasksMutex);
    auto it=tasks.find(handle);
    return it==tasks.end()?nullptr:it->second;
}
void joinWorker(const std::shared_ptr<TaskControl>& task){
    if(task&&task->worker.joinable()&&task->worker.get_id()!=std::this_thread::get_id())task->worker.join();
}
}

std::size_t runtimeTaskSpawn(RuntimeTaskBody body){
    if(!body)return 0;
    auto task=std::make_shared<TaskControl>();
    std::size_t handle=0;
    {
        std::lock_guard<std::mutex> lock(tasksMutex);
        handle=nextTask++;
        if(handle==0)handle=nextTask++;
        tasks.emplace(handle,task);
    }
    task->worker=std::thread([task,body=std::move(body)]() mutable {
        {
            std::lock_guard<std::mutex> lock(task->mutex);
            if(task->cancelled.load(std::memory_order_acquire)){
                task->state=RuntimeTaskState::Cancelled;
                task->cv.notify_all();
                return;
            }
            task->state=RuntimeTaskState::Running;
        }
        runtimeSynchronizationPoint();
        try{
            const auto result=body(task->cancelled);
            std::lock_guard<std::mutex> lock(task->mutex);
            task->result=result;
            task->state=task->cancelled.load(std::memory_order_acquire)?RuntimeTaskState::Cancelled:RuntimeTaskState::Ready;
        }catch(...){
            std::lock_guard<std::mutex> lock(task->mutex);
            task->result=-1;
            task->state=task->cancelled.load(std::memory_order_acquire)?RuntimeTaskState::Cancelled:RuntimeTaskState::Failed;
        }
        runtimeSynchronizationPoint();
        task->cv.notify_all();
    });
    return handle;
}

RuntimeTaskState runtimeTaskState(std::size_t handle){
    auto task=getTask(handle);if(!task)return RuntimeTaskState::Failed;
    std::lock_guard<std::mutex> lock(task->mutex);return task->state;
}

std::int64_t runtimeTaskJoin(std::size_t handle,std::int64_t timeoutMillis){
    auto task=getTask(handle);if(!task||timeoutMillis<0)return -1;
    {
        std::unique_lock<std::mutex> lock(task->mutex);
        const auto terminal=[&]{return task->state==RuntimeTaskState::Ready||task->state==RuntimeTaskState::Failed||task->state==RuntimeTaskState::Cancelled;};
        if(timeoutMillis==0){if(!terminal())return -2;}
        else if(!task->cv.wait_for(lock,std::chrono::milliseconds(timeoutMillis),terminal))return -2;
        if(task->state==RuntimeTaskState::Cancelled)return -3;
        if(task->state==RuntimeTaskState::Failed)return -1;
    }
    joinWorker(task);
    runtimeSynchronizationPoint();
    std::lock_guard<std::mutex> lock(task->mutex);return task->result;
}

bool runtimeTaskCancel(std::size_t handle){
    auto task=getTask(handle);if(!task)return false;
    task->cancelled.store(true,std::memory_order_release);
    std::lock_guard<std::mutex> lock(task->mutex);
    if(task->state==RuntimeTaskState::Pending)task->state=RuntimeTaskState::Cancelled;
    task->cv.notify_all();return true;
}

bool runtimeTaskCancelled(std::size_t handle){auto task=getTask(handle);return !task||task->cancelled.load(std::memory_order_acquire);}

bool runtimeTaskDestroy(std::size_t handle){
    std::shared_ptr<TaskControl> task;
    {
        std::lock_guard<std::mutex> lock(tasksMutex);
        auto it=tasks.find(handle);if(it==tasks.end())return false;
        task=it->second;tasks.erase(it);
    }
    task->cancelled.store(true,std::memory_order_release);
    joinWorker(task);return true;
}

void runtimeTaskJoinAll(){
    std::vector<std::shared_ptr<TaskControl>> copy;
    {std::lock_guard<std::mutex> lock(tasksMutex);for(auto& item:tasks)copy.push_back(item.second);}
    for(auto& task:copy)joinWorker(task);
}

void runtimeTaskReset(){
    std::vector<std::shared_ptr<TaskControl>> copy;
    {std::lock_guard<std::mutex> lock(tasksMutex);for(auto& item:tasks){item.second->cancelled.store(true,std::memory_order_release);copy.push_back(item.second);}tasks.clear();}
    for(auto& task:copy)joinWorker(task);
}

} // namespace noe
