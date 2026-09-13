#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace noe {

enum class RuntimeTaskState : int {
    Pending = 0,
    Running = 1,
    Ready = 2,
    Failed = 3,
    Cancelled = 4
};

using RuntimeTaskBody = std::function<std::int64_t(const std::atomic_bool& cancelled)>;

std::size_t runtimeTaskSpawn(RuntimeTaskBody body);
RuntimeTaskState runtimeTaskState(std::size_t handle);
std::int64_t runtimeTaskJoin(std::size_t handle,std::int64_t timeoutMillis);
bool runtimeTaskCancel(std::size_t handle);
bool runtimeTaskCancelled(std::size_t handle);
bool runtimeTaskDestroy(std::size_t handle);
void runtimeTaskJoinAll();
void runtimeTaskReset();

} // namespace noe
