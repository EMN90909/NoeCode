#include "noe.hpp"
#include <iostream>
namespace noe {
int TestRunner::runDirectory(const std::filesystem::path& dir,RuntimeCheckConfig checks)const{
    if(!std::filesystem::exists(dir)){std::cerr<<"NQR-TEST7000: test directory not found: "<<dir<<"\n";return 1;}
    int failed=0,total=0;
    for(auto&e:std::filesystem::recursive_directory_iterator(dir)){
        if(!e.is_regular_file()||e.path().extension()!=".nqr")continue;
        ++total;
        try{
            auto c=compileFile(e.path());
            if(c.diagnostics.hasErrors()){
                ++failed;std::cerr<<"FAIL "<<e.path()<<"\n";c.diagnostics.print(e.path().string());
            }else{
                runtimeConfigureChecks(checks);
                Interpreter i;
                int rc=i.run(c.nir);
                runtimeConfigureChecks({});
                if(rc){++failed;std::cerr<<"FAIL "<<e.path()<<"\n";}
                else std::cout<<"PASS "<<e.path()<<"\n";
            }
        }catch(const std::exception&ex){runtimeConfigureChecks({});++failed;std::cerr<<"FAIL "<<e.path()<<": "<<ex.what()<<"\n";}
    }
    std::cout<<"noqeri tests: "<<(total-failed)<<" passed, "<<failed<<" failed, "<<total<<" total";
    if(checks.memory||checks.race||checks.overflow)std::cout<<" [checks:"<<(checks.memory?" memory":"")<<(checks.race?" race":"")<<(checks.overflow?" overflow":"")<<" ]";
    std::cout<<"\n";
    return failed?1:0;
}
}
