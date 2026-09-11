#include "noe.hpp"
#include <cstdlib>
#include <filesystem>
#include <string>
namespace noe {
namespace {
std::string quote(const std::filesystem::path& path){std::string s=path.string();std::string out="\"";for(char c:s){if(c=='\"')out+="\\\"";else out+=c;}out+='\"';return out;}
std::string replaceAll(std::string text,const std::string& key,const std::string& value){std::size_t pos=0;while((pos=text.find(key,pos))!=std::string::npos){text.replace(pos,key.size(),value);pos+=value.size();}return text;}
}
bool LinkerDriver::link(const std::filesystem::path& assembly,const std::filesystem::path& output,Diagnostics& diagnostics)const{
    if(!std::filesystem::exists(assembly)){diagnostics.error("NQR-K5101",{},"assembly input not found: "+assembly.string());return false;}
    const char* configured=std::getenv("NOQERI_NATIVE_ASSEMBLER");
    if(!configured||!*configured){diagnostics.error("NQR-K5100",{},"Noqeri native output is freestanding and is not linked to an OS automatically","use 'noqeri build' to keep the .s artifact, or set NOQERI_NATIVE_ASSEMBLER to a command template containing {input} and {output}");return false;}
    std::filesystem::create_directories(output.parent_path().empty()?std::filesystem::path("."):output.parent_path());
    std::string command=configured;
    command=replaceAll(command,"{input}",quote(assembly));
    command=replaceAll(command,"{output}",quote(output));
    if(std::system(command.c_str())!=0){diagnostics.error("NQR-K5103",{},"configured assembler failed","the compiler does not assume ELF, Mach-O, COFF, Linux, Windows, macOS, or a kernel linker; configure the adapter for your target");return false;}
    return true;
}
}
