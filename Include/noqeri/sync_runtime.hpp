#pragma once
#include <cstddef>
#include <cstdint>

namespace noe {

std::size_t runtimeChannelCreate(std::size_t capacity);
std::int64_t runtimeChannelSendI64(std::size_t handle,std::int64_t value,std::int64_t timeoutMillis);
std::int64_t runtimeChannelReceiveI64(std::size_t handle,std::int64_t timeoutMillis);
std::size_t runtimeChannelCount(std::size_t handle);
bool runtimeChannelClose(std::size_t handle);
bool runtimeChannelDestroy(std::size_t handle);

std::size_t runtimeMutexCreate();
bool runtimeMutexLock(std::size_t handle,std::int64_t timeoutMillis);
bool runtimeMutexTryLock(std::size_t handle);
bool runtimeMutexUnlock(std::size_t handle);
bool runtimeMutexDestroy(std::size_t handle);

std::size_t runtimeRwLockCreate();
bool runtimeRwLockRead(std::size_t handle,std::int64_t timeoutMillis);
bool runtimeRwLockWrite(std::size_t handle,std::int64_t timeoutMillis);
bool runtimeRwLockTryRead(std::size_t handle);
bool runtimeRwLockTryWrite(std::size_t handle);
bool runtimeRwLockUnlock(std::size_t handle);
bool runtimeRwLockDestroy(std::size_t handle);

void runtimeSyncReset();

} // namespace noe
