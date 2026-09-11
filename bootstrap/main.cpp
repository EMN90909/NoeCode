#include "noe.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace noe;

static void usage(){
    std::cout << "Noe bootstrap compiler 0.0.3\n"
              << "usage: noe <command> [path]\n\n"
              << "  lex <file>       print lexer tokens\n"
              << "  check <file>     parse and type-check a source file\n"
              << "  nir <file>       lower and optimize source to NIR\n"
              << "  run <file>       execute optimized NIR\n"
              << "  format <file>    canonical-format a source file in place\n"
              << "  manifest [file]  read project.noe\n"
              << "  test [dir]       compile and run .noe tests\n"
              << "  build [file]     reserved for custom native backend (phase 4+)\n"
              << "  lsp              start language-server bootstrap\n";
}

int main(int argc,char**argv){
    if(argc<2){usage();return 0;}
    std::string cmd=argv[1];
    if(cmd=="--version"||cmd=="version"){std::cout<<"Noe 0.0.3-bootstrap\n";return 0;}
    if(cmd=="lsp")return LanguageServer{}.run();
    try{
        if(cmd=="test")return TestRunner{}.runDirectory(argc>=3?argv[2]:"tests");
        if(cmd=="manifest"){
            Diagnostics d;
            auto p=PackageManager{}.loadManifest(argc>=3?argv[2]:"project.noe",d);
            if(!p){d.print("project.noe");return 1;}
            std::cout<<"name="<<p->name<<"\nversion="<<p->version<<"\nentry="<<p->entry<<"\n";
            return 0;
        }
        if(argc<3){usage();return 1;}
        std::filesystem::path path=argv[2];
        std::string source=readTextFile(path);
        if(cmd=="lex"){
            Diagnostics d; Lexer l(source,d); auto tokens=l.lex();
            if(d.hasErrors()){d.print(path.string());return 1;}
            for(auto&t:tokens)std::cout<<t.span.line<<':'<<t.span.column<<"  "<<tokenKindName(t.kind)<<"  "<<t.lexeme<<"\n";
            return 0;
        }
        if(cmd=="format"){
            Diagnostics d; auto text=Formatter{}.format(source,d);
            if(d.hasErrors()){d.print(path.string());return 1;}
            std::ofstream out(path,std::ios::trunc);out<<text;
            std::cout<<"formatted "<<path<<"\n";
            return 0;
        }
        auto c=compileSource(source);
        if(c.diagnostics.hasErrors()){c.diagnostics.print(path.string());return 1;}
        if(cmd=="check"){std::cout<<"check succeeded: "<<path<<"\n";return 0;}
        if(cmd=="nir"){std::cout<<printNir(c.nir);return 0;}
        if(cmd=="run")return Interpreter{}.run(c.nir);
        if(cmd=="build"){
            Diagnostics d; NativeBackend b; std::filesystem::path out=path.stem();
            if(!b.emitAssembly(c.nir,out,d)){d.print(path.string());return 1;}
            return 0;
        }
        usage();return 1;
    }catch(const std::exception&e){std::cerr<<"NOE-C0001: "<<e.what()<<"\n";return 1;}
}
