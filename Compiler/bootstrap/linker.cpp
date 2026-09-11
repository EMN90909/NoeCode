#include "noe.hpp"
#include <cstdlib>
#include <filesystem>
#include <string>
namespace noe { namespace { std::string shellQuote(const std::filesystem::path&path){std::string s=path.string();std::string out="'";for(char c:s){if(c=='\'')out+="'\\''";else out+=c;}out+="'";return out;} }
bool LinkerDriver::link(const std::filesystem::path&assembly,const std::filesystem::path&output,Diagnostics&diagnostics)const{
#if !defined(__linux__) || !defined(__x86_64__)
(void)assembly;(void)output;diagnostics.error("NQR-K5100",{},"bootstrap linker driver currently supports Linux x86-64 only");return false;
#else
if(!std::filesystem::exists(assembly)){diagnostics.error("NQR-K5101",{},"assembly input not found: "+assembly.string());return false;}std::filesystem::create_directories(output.parent_path().empty()?std::filesystem::path("."):output.parent_path());auto object=output;object+=".nqr.o";std::string assemble="as --64 "+shellQuote(assembly)+" -o "+shellQuote(object);if(std::system(assemble.c_str())!=0){diagnostics.error("NQR-K5102",{},"system assembler failed","install GNU binutils or inspect the emitted .s file");return false;}std::string link="ld -m elf_x86_64 -o "+shellQuote(output)+" "+shellQuote(object);int rc=std::system(link.c_str());std::error_code ec;std::filesystem::remove(object,ec);if(rc!=0){diagnostics.error("NQR-K5103",{},"ELF linker failed","install GNU binutils and verify the target is Linux x86-64");return false;}return true;
#endif
} }
