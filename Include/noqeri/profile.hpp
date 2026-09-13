#pragma once
#include <cstddef>
#include <string>

namespace noe {

void runtimeProfileEnable(bool enabled);
bool runtimeProfileEnabled();
void runtimeProfileReset();
void runtimeProfileAllocation(std::size_t bytes);
bool runtimeProfileWrite(const std::string& jsonPath,const std::string& foldedPath={});

class RuntimeProfileScope {
public:
    explicit RuntimeProfileScope(std::string name);
    ~RuntimeProfileScope();
    RuntimeProfileScope(const RuntimeProfileScope&)=delete;
    RuntimeProfileScope& operator=(const RuntimeProfileScope&)=delete;
private:
    std::string name_;
    long long started_=0;
    bool active_=false;
};

} // namespace noe
