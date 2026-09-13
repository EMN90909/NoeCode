#include "noe.hpp"
#include <iostream>

namespace noe {
void Diagnostics::error(std::string code, Span span, std::string message, std::string help) {
    items_.push_back({std::move(code), std::move(message), span, std::move(help)});
}
void Diagnostics::print(const std::string& sourceName) const {
    for (const auto& d : items_) {
        std::cerr << d.code << ": " << d.message << "\n --> " << sourceName << ':' << d.span.line << ':' << d.span.column << "\n";
        if (!d.help.empty()) std::cerr << " help: " << d.help << "\n";
    }
}
}

static const char* formatName(noe::ObjectFormat f) {
    switch (f) {
        case noe::ObjectFormat::Elf64: return "elf64";
        case noe::ObjectFormat::Coff64: return "coff64";
        case noe::ObjectFormat::MachO64: return "macho64";
        case noe::ObjectFormat::Wasm: return "wasm";
        case noe::ObjectFormat::Assembly: return "assembly";
        default: return "none";
    }
}
static const char* archName(noe::Architecture a) {
    switch (a) {
        case noe::Architecture::X86_64: return "x86_64";
        case noe::Architecture::AArch64: return "aarch64";
        case noe::Architecture::Wasm32: return "wasm32";
        default: return "unknown";
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: noqeri-object-inspect <object>\n";
        return 2;
    }
    noe::Diagnostics diagnostics;
    auto info = noe::ObjectInspector::inspect(argv[1], diagnostics);
    if (!info) {
        diagnostics.print(argv[1]);
        return 1;
    }
    std::cout << "format=" << formatName(info->format)
              << "\narchitecture=" << archName(info->architecture)
              << "\nsize=" << info->size << "\n";
    return 0;
}
