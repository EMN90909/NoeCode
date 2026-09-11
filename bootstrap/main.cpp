#include "noe.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace noe;

static void usage() {
    std::cout << "Noe compiler " << NOE_COMPILER_VERSION << "\n"
              << "usage: noe <command> [path]\n\n"
              << "  new <name> [dir]       create a Noe project (project.noe, no TOML)\n"
              << "  lex <file>             print lexer tokens\n"
              << "  check [file]           parse and type-check source\n"
              << "  nir [file]             lower and optimize source to NIR\n"
              << "  run [file]             execute optimized NIR\n"
              << "  build [file] [output]  build Linux x86-64 native executable\n"
              << "  format <file>          canonical-format source in place\n"
              << "  manifest [file]        read project.noe\n"
              << "  lock [file]            generate deterministic noe.lock\n"
              << "  test [dir]             compile and run .noe tests\n"
              << "  doctor [dir]           verify production-track repository health\n"
              << "  release-check [dir]    alias for doctor after running tests externally\n"
              << "  lsp                    run JSON-RPC language server\n";
}

struct SourceSelection { std::filesystem::path source; std::string projectName; };
static std::optional<SourceSelection> selectSource(int argc, char** argv, Diagnostics& d) {
    if (argc >= 3) return SourceSelection{argv[2], std::filesystem::path(argv[2]).stem().string()};
    auto m = PackageManager{}.loadManifest("project.noe", d);
    if (!m) return std::nullopt;
    return SourceSelection{m->entry, m->name};
}

int main(int argc, char** argv) {
    if (argc < 2) { usage(); return 0; }
    std::string cmd = argv[1];
    if (cmd == "--version" || cmd == "version") {
        std::cout << "Noe " << NOE_COMPILER_VERSION << "\n"
                  << "language=" << NOE_LANGUAGE_VERSION << "\n"
                  << "target=" << NOE_SUPPORTED_TARGET << "\n";
        return 0;
    }
    if (cmd == "lsp") return LanguageServer{}.run();
    if (cmd == "doctor" || cmd == "release-check") return runProductionDoctor(argc >= 3 ? argv[2] : ".");
    try {
        if (cmd == "new") {
            if (argc < 3) { std::cerr << "NOE-C0002: new requires a project name\n"; return 1; }
            std::filesystem::path dir = argc >= 4 ? argv[3] : argv[2];
            Diagnostics d; if (!PackageManager{}.createProject(dir, argv[2], d)) { d.print(dir.string()); return 1; }
            std::cout << "created Noe project " << dir << "\n"; return 0;
        }
        if (cmd == "test") return TestRunner{}.runDirectory(argc >= 3 ? argv[2] : "tests");
        if (cmd == "manifest" || cmd == "lock") {
            std::filesystem::path path = argc >= 3 ? argv[2] : "project.noe";
            Diagnostics d; auto p = PackageManager{}.loadManifest(path, d);
            if (!p) { d.print(path.string()); return 1; }
            if (cmd == "manifest") {
                std::cout << "name=" << p->name << "\nversion=" << p->version << "\nentry=" << p->entry
                          << "\nprofile=" << p->profile << "\ntarget=" << p->target << "\ndependencies=" << p->dependencies.size() << "\n";
                return 0;
            }
            auto lock = path.parent_path() / "noe.lock";
            if (!PackageManager{}.writeLock(*p, lock, d)) { d.print(path.string()); return 1; }
            std::cout << "wrote " << lock << "\n"; return 0;
        }
        if (cmd == "format") {
            if (argc < 3) { usage(); return 1; }
            std::filesystem::path path = argv[2]; auto source = readTextFile(path);
            Diagnostics d; auto text = Formatter{}.format(source, d);
            if (d.hasErrors()) { d.print(path.string()); return 1; }
            std::ofstream out(path, std::ios::trunc); out << text; std::cout << "formatted " << path << "\n"; return 0;
        }
        Diagnostics selectionDiagnostics;
        auto selected = selectSource(argc, argv, selectionDiagnostics);
        if (!selected) { selectionDiagnostics.print("project.noe"); return 1; }
        auto path = selected->source; auto source = readTextFile(path);
        if (cmd == "lex") {
            Diagnostics d; Lexer l(source, d); auto tokens = l.lex();
            if (d.hasErrors()) { d.print(path.string()); return 1; }
            for (auto& t : tokens) std::cout << t.span.line << ':' << t.span.column << "  " << tokenKindName(t.kind) << "  " << t.lexeme << "\n";
            return 0;
        }
        auto c = compileSource(source);
        if (c.diagnostics.hasErrors()) { c.diagnostics.print(path.string()); return 1; }
        if (cmd == "check") { std::cout << "check succeeded: " << path << "\n"; return 0; }
        if (cmd == "nir") { std::cout << printNir(c.nir); return 0; }
        if (cmd == "run") return Interpreter{}.run(c.nir);
        if (cmd == "build") {
            std::filesystem::path output;
            if (argc >= 4) output = argv[3]; else output = std::filesystem::path("build") / selected->projectName;
            std::filesystem::create_directories(output.parent_path().empty() ? std::filesystem::path(".") : output.parent_path());
            auto assembly = output; assembly += ".s";
            Diagnostics d;
            if (!NativeBackend{}.emitAssembly(c.nir, assembly, d)) { d.print(path.string()); return 1; }
            if (!LinkerDriver{}.link(assembly, output, d)) { d.print(path.string()); return 1; }
            if (std::getenv("NOE_KEEP_ASM") == nullptr) { std::error_code ec; std::filesystem::remove(assembly, ec); }
            std::cout << "built native executable: " << output << "\n"; return 0;
        }
        usage(); return 1;
    } catch (const std::exception& e) {
        std::cerr << "NOE-C0001: " << e.what() << "\n"; return 1;
    }
}
