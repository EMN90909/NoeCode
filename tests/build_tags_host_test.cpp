#include "noqeri.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>

int main(){
    using namespace noe;
    const auto root=std::filesystem::temp_directory_path()/("noqeri-build-tags-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
    auto write=[&](const char*name,const char*text){std::ofstream out(root/name,std::ios::trunc);out<<text;};
    write("main.nqr","import \"linux_impl\"\nimport \"windows_impl\"\nfunction main(): int { return platform_value() }\n");
    write("linux_impl.nqr","// noqeri:target linux\nfunction platform_value(): int { return 1 }\n");
    write("windows_impl.nqr","// noqeri:target windows\nfunction platform_value(): int { return 2 }\n");
    CompileOptions linuxOptions;linuxOptions.target="x86_64-unknown-linux";
    auto linux=compileFile(root/"main.nqr",linuxOptions);
    if(linux.diagnostics.hasErrors()){linux.diagnostics.print((root/"main.nqr").string());std::filesystem::remove_all(root);return 1;}
    CompileOptions windowsOptions;windowsOptions.target="x86_64-pc-windows";
    auto windows=compileFile(root/"main.nqr",windowsOptions);
    if(windows.diagnostics.hasErrors()){windows.diagnostics.print((root/"main.nqr").string());std::filesystem::remove_all(root);return 2;}
    write("excluded.nqr","// noqeri:not-target linux\nfunction main(): int { return 0 }\n");
    auto excluded=compileFile(root/"excluded.nqr",linuxOptions);
    if(!excluded.diagnostics.hasErrors()){std::cerr<<"excluded entry unexpectedly compiled\n";std::filesystem::remove_all(root);return 3;}
    std::filesystem::remove_all(root);
    std::cout<<"build tag checks passed\n";
    return 0;
}
