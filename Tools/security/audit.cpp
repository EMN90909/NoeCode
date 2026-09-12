#include "noe.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace noe {
namespace {
std::string readOptional(const std::filesystem::path&path){std::ifstream in(path,std::ios::binary);if(!in)return{};std::ostringstream out;out<<in.rdbuf();return out.str();}
struct Capability { const char* needle; const char* label; };
}
int runSecurityAudit(const std::filesystem::path&root){
    int integrityFailures=0;std::size_t reviews=0;Diagnostics diagnostics;auto manifestPath=root/"project.nqr";auto manifest=PackageManager{}.loadManifest(manifestPath,diagnostics);
    if(!manifest){++integrityFailures;diagnostics.print(manifestPath.string());}else{
        auto lockPath=root/"noqeri.lock";auto lock=readOptional(lockPath);if(lock.rfind("noqeri-lock 2\n",0)!=0){std::cerr<<"fail package lock missing or not content-addressed v2: "<<lockPath.string()<<"\n";++integrityFailures;}else{
            for(const auto&[alias,spec]:manifest->dependencySpecs){const auto prefix="dependency "+alias+" ";auto at=lock.find(prefix);if(at==std::string::npos){std::cerr<<"fail dependency not locked: "<<alias<<"\n";++integrityFailures;continue;}auto end=lock.find('\n',at);auto line=lock.substr(at,end==std::string::npos?std::string::npos:end-at);if(line.find("checksum sha256:")==std::string::npos){std::cerr<<"fail dependency lacks SHA-256 lock identity: "<<alias<<"\n";++integrityFailures;}}
        }
    }
    const std::vector<Capability>capabilities={{"asm(\"","inline assembly"},{"intrinsic(\"","target intrinsic"},{"*volatile ","volatile memory"},{"host(\"","host service"},{"abi(\"","ABI service"},{" as *","pointer cast"}};
    std::error_code ec;if(std::filesystem::exists(root,ec))for(const auto&entry:std::filesystem::recursive_directory_iterator(root,ec)){if(ec)break;if(!entry.is_regular_file())continue;auto path=entry.path();if(path.extension()!=".nqr")continue;auto generic=path.generic_string();if(generic.find("/.git/")!=std::string::npos||generic.find("/build/")!=std::string::npos)continue;auto source=readOptional(path);for(const auto&cap:capabilities){std::size_t pos=0,count=0;while((pos=source.find(cap.needle,pos))!=std::string::npos){++count;++reviews;pos+=std::char_traits<char>::length(cap.needle);}if(count)std::cout<<"review "<<cap.label<<" count="<<count<<" file="<<path.string()<<"\n";}}
    if(ec){std::cerr<<"fail source audit traversal: "<<ec.message()<<"\n";++integrityFailures;}
    std::cout<<"audit integrity="<<(integrityFailures?"failed":"ok")<<" capability-review="<<reviews<<"\n";
    if(reviews)std::cout<<"note capability findings are review points, not automatic vulnerabilities\n";
    return integrityFailures?1:0;
}
} // namespace noe
