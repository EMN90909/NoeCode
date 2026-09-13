#include "noe.hpp"
#include <chrono>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <vector>

namespace noe {
namespace {
struct FunctionStats { std::uint64_t calls=0,totalNs=0; };
struct ThreadStats { std::uint64_t scopes=0,totalNs=0,allocations=0,allocatedBytes=0; };
bool enabled=false;
std::mutex profileMutex;
std::map<std::string,FunctionStats> functions;
std::map<std::string,std::uint64_t> folded;
std::unordered_map<std::string,ThreadStats> threads;
thread_local std::vector<std::string> stack;

long long nowNs(){return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
std::string threadKey(){std::ostringstream out;out<<std::this_thread::get_id();return out.str();}
std::string foldedKey(){std::string out;for(std::size_t i=0;i<stack.size();++i){if(i)out.push_back(';');out+=stack[i];}return out;}
std::string escapeJson(const std::string&s){std::string out;for(char c:s){switch(c){case'\\':out+="\\\\";break;case'\"':out+="\\\"";break;case'\n':out+="\\n";break;case'\r':out+="\\r";break;case'\t':out+="\\t";break;default:out.push_back(c);break;}}return out;}
}

void runtimeProfileEnable(bool value){std::lock_guard<std::mutex>lock(profileMutex);enabled=value;}
bool runtimeProfileEnabled(){std::lock_guard<std::mutex>lock(profileMutex);return enabled;}
void runtimeProfileReset(){std::lock_guard<std::mutex>lock(profileMutex);functions.clear();folded.clear();threads.clear();stack.clear();}
void runtimeProfileAllocation(std::size_t bytes){if(!runtimeProfileEnabled())return;std::lock_guard<std::mutex>lock(profileMutex);auto&t=threads[threadKey()];++t.allocations;t.allocatedBytes+=bytes;}

RuntimeProfileScope::RuntimeProfileScope(std::string name):name_(std::move(name)){
    if(!runtimeProfileEnabled())return;
    active_=true;started_=nowNs();stack.push_back(name_);
}
RuntimeProfileScope::~RuntimeProfileScope(){
    if(!active_)return;
    const auto elapsed=static_cast<std::uint64_t>(nowNs()-started_);
    const auto key=foldedKey(),thread=threadKey();
    {
        std::lock_guard<std::mutex>lock(profileMutex);
        auto&f=functions[name_];++f.calls;f.totalNs+=elapsed;
        auto&t=threads[thread];++t.scopes;t.totalNs+=elapsed;
        folded[key]+=elapsed;
    }
    if(!stack.empty())stack.pop_back();
}

bool runtimeProfileWrite(const std::string&jsonPath,const std::string&foldedPath){
    std::lock_guard<std::mutex>lock(profileMutex);
    std::ofstream json(jsonPath,std::ios::trunc);if(!json)return false;
    json<<"{\n  \"format\": \"noqeri-runtime-profile-v1\",\n  \"functions\": [\n";
    bool first=true;for(const auto&[name,s]:functions){if(!first)json<<",\n";first=false;json<<"    {\"name\":\""<<escapeJson(name)<<"\",\"calls\":"<<s.calls<<",\"total_ns\":"<<s.totalNs<<",\"mean_ns\":"<<(s.calls?s.totalNs/s.calls:0)<<"}";}
    json<<"\n  ],\n  \"threads\": [\n";first=true;for(const auto&[id,s]:threads){if(!first)json<<",\n";first=false;json<<"    {\"id\":\""<<escapeJson(id)<<"\",\"scopes\":"<<s.scopes<<",\"total_ns\":"<<s.totalNs<<",\"allocations\":"<<s.allocations<<",\"allocated_bytes\":"<<s.allocatedBytes<<"}";}
    json<<"\n  ]\n}\n";json.close();if(!json)return false;
    if(!foldedPath.empty()){
        std::ofstream flame(foldedPath,std::ios::trunc);if(!flame)return false;
        for(const auto&[name,ns]:folded)flame<<name<<' '<<ns<<'\n';
        flame.close();if(!flame)return false;
    }
    return true;
}

} // namespace noe
