#include "noe.hpp"

namespace noe {
int Interpreter::runProfiled(const NirProgram&program,const std::string&jsonPath,const std::string&foldedPath){
    runtimeProfileReset();
    runtimeProfileEnable(true);
    int status=1;
    {
        RuntimeProfileScope scope("program");
        status=run(program);
    }
    runtimeProfileEnable(false);
    if(!runtimeProfileWrite(jsonPath,foldedPath))return status==0?1:status;
    return status;
}
} // namespace noe
