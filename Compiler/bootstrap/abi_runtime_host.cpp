#include "noe.hpp"
#include "abi.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
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
const noqeri_abi desktopAbi={NOQERI_ABI_VERSION,sizeof(noqeri_abi),nullptr,desktopWrite,desktopClock,desktopPlatform,0,nullptr};
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
