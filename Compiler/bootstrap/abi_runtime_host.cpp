#include "noe.hpp"
#include "abi.h"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#endif

namespace noe {
namespace {
std::string valueText(const noqeri_abi_value& value){switch(value.tag){case NOQERI_ABI_NULL:return"null";case NOQERI_ABI_BOOL:return value.as.boolean?"true":"false";case NOQERI_ABI_INT:return std::to_string(value.as.integer);case NOQERI_ABI_FLOAT:return std::to_string(value.as.floating);case NOQERI_ABI_STRING:return value.as.string.data?std::string(value.as.string.data,value.as.string.size):std::string{};default:return"<invalid>";}}
int32_t desktopWrite(void*,const char* data,size_t size){if(data&&size)std::cout.write(data,static_cast<std::streamsize>(size));return std::cout?0:1;}
int64_t desktopClock(void*){return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());}
noqeri_abi_string desktopPlatform(void*){
#if defined(_WIN32)
static constexpr char text[]="windows";
#elif defined(__APPLE__)
static constexpr char text[]="macos";
#elif defined(__linux__)
static constexpr char text[]="linux";
#else
static constexpr char text[]="standalone";
#endif
return{text,sizeof(text)-1};}

thread_local std::string serviceStringResult;
thread_local std::string serviceErrorText;
std::atomic<std::uint64_t> nextServiceHandle{1};
struct WatchState { std::filesystem::path path; bool recursive=false; std::uint64_t fingerprint=0; };
std::mutex serviceStateMutex;
std::unordered_map<std::uint64_t,WatchState> watchers;
std::unordered_map<std::uint64_t,std::vector<unsigned char>> mappings;

enum class DesktopServiceKind {
    FsExists,FsIsDirectory,FsCreateDirectory,FsCreateDirectories,FsRemove,FsRemoveRecursive,FsRename,FsCopy,
    FsReadText,FsWriteText,FsAppendText,FsFileSize,FsModifiedMillis,FsPermissions,FsSetPermissions,
    FsIsSymlink,FsReadSymlink,FsCreateSymlink,FsTempFile,FsTempDirectory,FsAtomicReplace,
    FsWatch,FsWatchPoll,FsWatchClose,FsMmapRead,FsMmapLength,FsMmapClose
};

bool abiStringArg(const noqeri_abi_value*args,size_t argc,size_t index,std::string&out){if(index>=argc||args[index].tag!=NOQERI_ABI_STRING)return false;out=args[index].as.string.data?std::string(args[index].as.string.data,args[index].as.string.size):std::string{};return true;}
bool abiIntArg(const noqeri_abi_value*args,size_t argc,size_t index,std::int64_t&out){if(index>=argc||args[index].tag!=NOQERI_ABI_INT)return false;out=args[index].as.integer;return true;}
bool abiBoolArg(const noqeri_abi_value*args,size_t argc,size_t index,bool&out){if(index>=argc||args[index].tag!=NOQERI_ABI_BOOL)return false;out=args[index].as.boolean!=0;return true;}
void abiNull(noqeri_abi_value*result){result->tag=NOQERI_ABI_NULL;}
void abiBool(noqeri_abi_value*result,bool value){result->tag=NOQERI_ABI_BOOL;result->as.boolean=value?1:0;}
void abiInt(noqeri_abi_value*result,std::int64_t value){result->tag=NOQERI_ABI_INT;result->as.integer=value;}
void abiString(noqeri_abi_value*result,std::string value){serviceStringResult=std::move(value);result->tag=NOQERI_ABI_STRING;result->as.string={serviceStringResult.data(),serviceStringResult.size()};}
int32_t abiFailure(noqeri_abi_error*error,const std::string&message){serviceErrorText=message;if(error){error->code=1;error->message=serviceErrorText.c_str();}return 1;}

std::int64_t modifiedMillis(const std::filesystem::path&path,std::error_code&ec){auto value=std::filesystem::last_write_time(path,ec);if(ec)return-1;auto system=std::chrono::time_point_cast<std::chrono::milliseconds>(value-decltype(value)::clock::now()+std::chrono::system_clock::now());return static_cast<std::int64_t>(system.time_since_epoch().count());}
std::uint64_t watchFingerprint(const std::filesystem::path&path,bool recursive){std::error_code ec;std::uint64_t hash=1469598103934665603ull;auto mix=[&](std::uint64_t value){hash^=value;hash*=1099511628211ull;};if(!std::filesystem::exists(path,ec)){mix(0);return hash;}auto sample=[&](const std::filesystem::directory_entry&entry){std::error_code local;mix(static_cast<std::uint64_t>(entry.path().generic_string().size()));if(entry.is_regular_file(local))mix(static_cast<std::uint64_t>(entry.file_size(local)));local.clear();auto t=entry.last_write_time(local);if(!local)mix(static_cast<std::uint64_t>(t.time_since_epoch().count()));};if(std::filesystem::is_directory(path,ec)){if(recursive){for(std::filesystem::recursive_directory_iterator it(path,ec),end;!ec&&it!=end;it.increment(ec))sample(*it);}else{for(std::filesystem::directory_iterator it(path,ec),end;!ec&&it!=end;it.increment(ec))sample(*it);}}else{std::filesystem::directory_entry entry(path,ec);if(!ec)sample(entry);}return hash;}
std::filesystem::path temporaryPath(const std::string&prefix){std::error_code ec;auto root=std::filesystem::temp_directory_path(ec);if(ec)root=std::filesystem::current_path(ec);std::string safe=std::filesystem::path(prefix).filename().string();if(safe.empty())safe="noqeri";const auto id=nextServiceHandle.fetch_add(1);return root/(safe+"-"+std::to_string(desktopClock(nullptr))+"-"+std::to_string(id));}

int32_t desktopService(void*context,const noqeri_abi_value*args,size_t argc,noqeri_abi_value*result,noqeri_abi_error*error){
    if(!context||!result)return abiFailure(error,"invalid desktop service invocation");
    const auto kind=*static_cast<DesktopServiceKind*>(context);
    std::string a,b;std::int64_t n=0;bool flag=false;std::error_code ec;
    switch(kind){
        case DesktopServiceKind::FsExists:
            if(!abiStringArg(args,argc,0,a))return abiFailure(error,"fsExists expects path string");abiBool(result,std::filesystem::exists(a,ec)&&!ec);return 0;
        case DesktopServiceKind::FsIsDirectory:
            if(!abiStringArg(args,argc,0,a))return abiFailure(error,"fsIsDirectory expects path string");abiBool(result,std::filesystem::is_directory(a,ec)&&!ec);return 0;
        case DesktopServiceKind::FsCreateDirectory:
            if(!abiStringArg(args,argc,0,a))return abiFailure(error,"fsCreateDirectory expects path string");{bool created=std::filesystem::create_directory(a,ec);abiInt(result,ec?-1:(created?1:0));}return 0;
        case DesktopServiceKind::FsCreateDirectories:
            if(!abiStringArg(args,argc,0,a))return abiFailure(error,"fsCreateDirectories expects path string");{bool created=std::filesystem::create_directories(a,ec);abiInt(result,ec?-1:(created?1:0));}return 0;
        case DesktopServiceKind::FsRemove:
            if(!abiStringArg(args,argc,0,a))return abiFailure(error,"fsRemove expects path string");{bool removed=std::filesystem::remove(a,ec);abiInt(result,ec?-1:(removed?1:0));}return 0;
        case DesktopServiceKind::FsRemoveRecursive:
            if(!abiStringArg(args,argc,0,a))return abiFailure(error,"fsRemoveRecursive expects path string");{auto count=std::filesystem::remove_all(a,ec);abiInt(result,ec?-1:static_cast<std::int64_t>(count));}return 0;
        case DesktopServiceKind::FsRename:
            if(!abiStringArg(args,argc,0,a)||!abiStringArg(args,argc,1,b))return abiFailure(error,"fsRename expects two paths");std::filesystem::rename(a,b,ec);abiInt(result,ec?-1:0);return 0;
        case DesktopServiceKind::FsCopy:
            if(!abiStringArg(args,argc,0,a)||!abiStringArg(args,argc,1,b))return abiFailure(error,"fsCopy expects two paths");std::filesystem::copy(a,b,std::filesystem::copy_options::recursive|std::filesystem::copy_options::overwrite_existing,ec);abiInt(result,ec?-1:0);return 0;
        case DesktopServiceKind::FsReadText:{
            if(!abiStringArg(args,argc,0,a))return abiFailure(error,"fsReadText expects path string");std::ifstream input(a,std::ios::binary);if(!input)return abiFailure(error,"cannot open file for reading: "+a);std::ostringstream out;out<<input.rdbuf();if(!input.eof()&&input.fail())return abiFailure(error,"failed reading file: "+a);abiString(result,out.str());return 0;}
        case DesktopServiceKind::FsWriteText:
        case DesktopServiceKind::FsAppendText:{
            if(!abiStringArg(args,argc,0,a)||!abiStringArg(args,argc,1,b))return abiFailure(error,"file write expects path and contents strings");std::ofstream output(a,std::ios::binary|(kind==DesktopServiceKind::FsAppendText?std::ios::app:std::ios::trunc));if(!output){abiInt(result,-1);return 0;}output.write(b.data(),static_cast<std::streamsize>(b.size()));output.flush();abiInt(result,output?0:-1);return 0;}
        case DesktopServiceKind::FsFileSize:
            if(!abiStringArg(args,argc,0,a))return abiFailure(error,"fsFileSize expects path string");{auto size=std::filesystem::file_size(a,ec);abiInt(result,ec?-1:static_cast<std::int64_t>(size));}return 0;
        case DesktopServiceKind::FsModifiedMillis:
            if(!abiStringArg(args,argc,0,a))return abiFailure(error,"fsModifiedMillis expects path string");abiInt(result,modifiedMillis(a,ec));return 0;
        case DesktopServiceKind::FsPermissions:
            if(!abiStringArg(args,argc,0,a))return abiFailure(error,"fsPermissions expects path string");{auto status=std::filesystem::status(a,ec);abiInt(result,ec?-1:static_cast<std::int64_t>(static_cast<unsigned>(status.permissions())));}return 0;
        case DesktopServiceKind::FsSetPermissions:
            if(!abiStringArg(args,argc,0,a)||!abiIntArg(args,argc,1,n))return abiFailure(error,"fsSetPermissions expects path and mode");std::filesystem::permissions(a,static_cast<std::filesystem::perms>(static_cast<unsigned>(n)),std::filesystem::perm_options::replace,ec);abiInt(result,ec?-1:0);return 0;
        case DesktopServiceKind::FsIsSymlink:
            if(!abiStringArg(args,argc,0,a))return abiFailure(error,"fsIsSymlink expects path string");abiBool(result,std::filesystem::is_symlink(std::filesystem::symlink_status(a,ec))&&!ec);return 0;
        case DesktopServiceKind::FsReadSymlink:
            if(!abiStringArg(args,argc,0,a))return abiFailure(error,"fsReadSymlink expects path string");{auto target=std::filesystem::read_symlink(a,ec);if(ec)return abiFailure(error,"cannot read symlink: "+a);abiString(result,target.generic_string());}return 0;
        case DesktopServiceKind::FsCreateSymlink:
            if(!abiStringArg(args,argc,0,a)||!abiStringArg(args,argc,1,b))return abiFailure(error,"fsCreateSymlink expects target and link path");std::filesystem::create_symlink(a,b,ec);abiInt(result,ec?-1:0);return 0;
        case DesktopServiceKind::FsTempFile:{
            if(!abiStringArg(args,argc,0,a))return abiFailure(error,"fsTempFile expects prefix");auto path=temporaryPath(a);std::ofstream file(path,std::ios::binary|std::ios::trunc);if(!file)return abiFailure(error,"cannot create temporary file");file.close();abiString(result,path.generic_string());return 0;}
        case DesktopServiceKind::FsTempDirectory:{
            if(!abiStringArg(args,argc,0,a))return abiFailure(error,"fsTempDirectory expects prefix");auto path=temporaryPath(a);if(!std::filesystem::create_directories(path,ec)&&ec)return abiFailure(error,"cannot create temporary directory");abiString(result,path.generic_string());return 0;}
        case DesktopServiceKind::FsAtomicReplace:
            if(!abiStringArg(args,argc,0,a)||!abiStringArg(args,argc,1,b))return abiFailure(error,"fsAtomicReplace expects source and destination");
#if defined(_WIN32)
            abiInt(result,MoveFileExA(a.c_str(),b.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)?0:-1);
#else
            std::filesystem::rename(a,b,ec);abiInt(result,ec?-1:0);
#endif
            return 0;
        case DesktopServiceKind::FsWatch:{
            if(!abiStringArg(args,argc,0,a)||!abiBoolArg(args,argc,1,flag))return abiFailure(error,"fsWatch expects path and recursive bool");if(!std::filesystem::exists(a,ec)||ec){abiInt(result,0);return 0;}const auto handle=nextServiceHandle.fetch_add(1);WatchState state{a,flag,watchFingerprint(a,flag)};{std::lock_guard<std::mutex>lock(serviceStateMutex);watchers.emplace(handle,std::move(state));}abiInt(result,static_cast<std::int64_t>(handle));return 0;}
        case DesktopServiceKind::FsWatchPoll:{
            if(!abiIntArg(args,argc,0,n)){return abiFailure(error,"fsWatchPoll expects watcher handle");}std::int64_t timeout=0;if(!abiIntArg(args,argc,1,timeout)||timeout<0)return abiFailure(error,"fsWatchPoll expects non-negative timeout");WatchState state;{std::lock_guard<std::mutex>lock(serviceStateMutex);auto it=watchers.find(static_cast<std::uint64_t>(n));if(it==watchers.end()){abiString(result,"");return 0;}state=it->second;}if(timeout>0)std::this_thread::sleep_for(std::chrono::milliseconds(timeout));auto next=watchFingerprint(state.path,state.recursive);if(next==state.fingerprint){abiString(result,"");return 0;}{std::lock_guard<std::mutex>lock(serviceStateMutex);auto it=watchers.find(static_cast<std::uint64_t>(n));if(it!=watchers.end())it->second.fingerprint=next;}abiString(result,state.path.generic_string());return 0;}
        case DesktopServiceKind::FsWatchClose:
            if(!abiIntArg(args,argc,0,n))return abiFailure(error,"fsWatchClose expects watcher handle");{std::lock_guard<std::mutex>lock(serviceStateMutex);abiBool(result,watchers.erase(static_cast<std::uint64_t>(n))>0);}return 0;
        case DesktopServiceKind::FsMmapRead:{
            if(!abiStringArg(args,argc,0,a))return abiFailure(error,"fsMmapRead expects path string");std::ifstream input(a,std::ios::binary);if(!input){abiInt(result,0);return 0;}std::vector<unsigned char>bytes((std::istreambuf_iterator<char>(input)),std::istreambuf_iterator<char>());const auto handle=nextServiceHandle.fetch_add(1);{std::lock_guard<std::mutex>lock(serviceStateMutex);mappings.emplace(handle,std::move(bytes));}abiInt(result,static_cast<std::int64_t>(handle));return 0;}
        case DesktopServiceKind::FsMmapLength:
            if(!abiIntArg(args,argc,0,n))return abiFailure(error,"fsMmapLength expects mapping handle");{std::lock_guard<std::mutex>lock(serviceStateMutex);auto it=mappings.find(static_cast<std::uint64_t>(n));abiInt(result,it==mappings.end()?0:static_cast<std::int64_t>(it->second.size()));}return 0;
        case DesktopServiceKind::FsMmapClose:
            if(!abiIntArg(args,argc,0,n))return abiFailure(error,"fsMmapClose expects mapping handle");{std::lock_guard<std::mutex>lock(serviceStateMutex);abiBool(result,mappings.erase(static_cast<std::uint64_t>(n))>0);}return 0;
    }
    abiNull(result);return abiFailure(error,"unknown desktop service");
}

DesktopServiceKind serviceKinds[]={
    DesktopServiceKind::FsExists,DesktopServiceKind::FsIsDirectory,DesktopServiceKind::FsCreateDirectory,DesktopServiceKind::FsCreateDirectories,
    DesktopServiceKind::FsRemove,DesktopServiceKind::FsRemoveRecursive,DesktopServiceKind::FsRename,DesktopServiceKind::FsCopy,
    DesktopServiceKind::FsReadText,DesktopServiceKind::FsWriteText,DesktopServiceKind::FsAppendText,DesktopServiceKind::FsFileSize,
    DesktopServiceKind::FsModifiedMillis,DesktopServiceKind::FsPermissions,DesktopServiceKind::FsSetPermissions,DesktopServiceKind::FsIsSymlink,
    DesktopServiceKind::FsReadSymlink,DesktopServiceKind::FsCreateSymlink,DesktopServiceKind::FsTempFile,DesktopServiceKind::FsTempDirectory,
    DesktopServiceKind::FsAtomicReplace,DesktopServiceKind::FsWatch,DesktopServiceKind::FsWatchPoll,DesktopServiceKind::FsWatchClose,
    DesktopServiceKind::FsMmapRead,DesktopServiceKind::FsMmapLength,DesktopServiceKind::FsMmapClose
};
const char*serviceNames[]={
    "fsExists","fsIsDirectory","fsCreateDirectory","fsCreateDirectories","fsRemove","fsRemoveRecursive","fsRename","fsCopy",
    "fsReadText","fsWriteText","fsAppendText","fsFileSize","fsModifiedMillis","fsPermissions","fsSetPermissions","fsIsSymlink",
    "fsReadSymlink","fsCreateSymlink","fsTempFile","fsTempDirectory","fsAtomicReplace","fsWatch","fsWatchPoll","fsWatchClose",
    "fsMmapRead","fsMmapLength","fsMmapClose"
};
std::vector<noqeri_abi_service> makeDesktopServices(){std::vector<noqeri_abi_service>out;out.reserve(sizeof(serviceKinds)/sizeof(serviceKinds[0]));for(std::size_t i=0;i<sizeof(serviceKinds)/sizeof(serviceKinds[0]);++i)out.push_back({serviceNames[i],desktopService,&serviceKinds[i]});return out;}
const std::vector<noqeri_abi_service> desktopServices=makeDesktopServices();
const noqeri_abi desktopAbi={NOQERI_ABI_VERSION,sizeof(noqeri_abi),nullptr,desktopWrite,desktopClock,desktopPlatform,desktopServices.size(),desktopServices.data()};

noqeri_abi_value toAbi(const NirValue& value){noqeri_abi_value out{};if(std::holds_alternative<std::monostate>(value))out.tag=NOQERI_ABI_NULL;else if(auto v=std::get_if<bool>(&value)){out.tag=NOQERI_ABI_BOOL;out.as.boolean=*v?1:0;}else if(auto v=std::get_if<std::int64_t>(&value)){out.tag=NOQERI_ABI_INT;out.as.integer=*v;}else if(auto v=std::get_if<double>(&value)){out.tag=NOQERI_ABI_FLOAT;out.as.floating=*v;}else if(auto v=std::get_if<std::string>(&value)){out.tag=NOQERI_ABI_STRING;out.as.string={v->data(),v->size()};}return out;}
NirValue fromAbi(const noqeri_abi_value& value){switch(value.tag){case NOQERI_ABI_NULL:return std::monostate{};case NOQERI_ABI_BOOL:return value.as.boolean!=0;case NOQERI_ABI_INT:return static_cast<std::int64_t>(value.as.integer);case NOQERI_ABI_FLOAT:return value.as.floating;case NOQERI_ABI_STRING:return value.as.string.data?std::string(value.as.string.data,value.as.string.size):std::string{};default:throw std::runtime_error("ABI returned invalid value tag");}}
}
const noqeri_abi* defaultNoqeriAbi(){return &desktopAbi;}
std::optional<NirValue> callNoqeriAbi(const noqeri_abi* abi,const std::string& name,const std::vector<NirValue>& args,std::string& error){
    if(!abi)return std::nullopt;
    if(abi->abi_version!=NOQERI_ABI_VERSION||abi->struct_size<sizeof(noqeri_abi)){error="Noqeri ABI version/size mismatch";return std::nullopt;}
    if(name=="print"){
        if(!abi->write){error="Noqeri ABI has no write callback";return std::nullopt;}
        for(std::size_t i=0;i<args.size();++i){if(i&&abi->write(abi->context," ",1)!=0){error="ABI write failed";return std::nullopt;}auto v=toAbi(args[i]);auto text=valueText(v);if(abi->write(abi->context,text.data(),text.size())!=0){error="ABI write failed";return std::nullopt;}}
        if(abi->write(abi->context,"\n",1)!=0){error="ABI write failed";return std::nullopt;}return NirValue(std::monostate{});
    }
    if(name=="clockMillis"){if(!args.empty()){error="clockMillis expects no arguments";return std::nullopt;}if(!abi->clock_millis){error="Noqeri ABI has no clock_millis callback";return std::nullopt;}return NirValue(static_cast<std::int64_t>(abi->clock_millis(abi->context)));}
    if(name=="platform"){if(!args.empty()){error="platform expects no arguments";return std::nullopt;}if(!abi->platform_name){error="Noqeri ABI has no platform_name callback";return std::nullopt;}auto s=abi->platform_name(abi->context);return NirValue(s.data?std::string(s.data,s.size):std::string{});}
    if(name=="textLength"){if(args.size()!=1||!std::holds_alternative<std::string>(args[0])){error="textLength expects one string argument";return std::nullopt;}return NirValue(static_cast<std::int64_t>(std::get<std::string>(args[0]).size()));}
    for(size_t i=0;i<abi->service_count;++i){const auto& service=abi->services[i];if(service.name&&service.invoke&&name==service.name){std::vector<noqeri_abi_value> converted;converted.reserve(args.size());for(const auto& value:args)converted.push_back(toAbi(value));noqeri_abi_value result{};noqeri_abi_error abiError{};int32_t status=service.invoke(service.context?service.context:abi->context,converted.data(),converted.size(),&result,&abiError);if(status!=0){error=abiError.message?abiError.message:"Noqeri ABI service failed";return std::nullopt;}return fromAbi(result);}}
    return std::nullopt;
}
const noqeri_abi* defaultHostApi(){return defaultNoqeriAbi();}
std::optional<NirValue> callAbiFunction(const noqeri_abi* abi,const std::string& name,const std::vector<NirValue>& args,std::string& error){return callNoqeriAbi(abi,name,args,error);}
}
extern "C" uint32_t noqeri_abi_version(void){return NOQERI_ABI_VERSION;}
extern "C" int32_t noqeri_run_source(const char* source,const noqeri_abi* abi,noqeri_abi_error* error){if(!source){if(error){error->code=1;error->message="source must not be null";}return 1;}auto result=noe::compileSource(source);if(result.diagnostics.hasErrors()){if(error){error->code=2;error->message="source failed to compile";}return 2;}int status=noe::Interpreter(abi).run(result.nir);if(status!=0&&error){error->code=3;error->message="runtime execution failed";}return status==0?0:3;}
