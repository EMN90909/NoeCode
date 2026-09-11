#include "noe.hpp"
#include <iostream>

namespace noe {
int LanguageServer::run(){
    std::cerr << "Noe language server bootstrap mode (phase 3/8).\n"
              << "Compiler-front-end diagnostics are reusable by editors; JSON-RPC transport and document synchronization are scheduled for a later phase.\n";
    return 0;
}
} // namespace noe
