#include "noe.hpp"
#include <algorithm>
#include <sstream>

namespace noe {
std::string Formatter::format(const std::string& source, Diagnostics& diagnostics) const {
    Lexer lexer(source, diagnostics);
    auto tokens=lexer.lex();
    if(diagnostics.hasErrors()) return source;
    std::ostringstream out;
    int indent=0;
    bool lineStart=true;
    TokenKind prev=TokenKind::Eof;
    auto writeIndent=[&]{ if(lineStart){ for(int i=0;i<indent;++i)out<<"    "; lineStart=false; } };
    auto newline=[&]{ auto s=out.str(); if(!s.empty()&&s.back()!='\n')out<<'\n'; lineStart=true; };
    for(const auto&t:tokens){
        if(t.kind==TokenKind::Eof)break;
        if(t.kind==TokenKind::RBrace){ if(!lineStart)newline(); indent=std::max(0,indent-1); writeIndent(); out<<'}'; newline(); prev=t.kind; continue; }
        if(t.kind==TokenKind::LBrace){ if(!lineStart)out<<' '; writeIndent(); out<<'{'; newline(); ++indent; prev=t.kind; continue; }
        if(t.kind==TokenKind::Semicolon){ out<<';'; newline(); prev=t.kind; continue; }
        writeIndent();
        bool tightBefore=t.kind==TokenKind::Comma||t.kind==TokenKind::RParen||t.kind==TokenKind::Dot||prev==TokenKind::LParen||prev==TokenKind::Dot;
        bool operatorToken=t.kind==TokenKind::Plus||t.kind==TokenKind::Minus||t.kind==TokenKind::Star||t.kind==TokenKind::Slash||t.kind==TokenKind::Percent||t.kind==TokenKind::Equal||t.kind==TokenKind::EqualEqual||t.kind==TokenKind::BangEqual||t.kind==TokenKind::Less||t.kind==TokenKind::LessEqual||t.kind==TokenKind::Greater||t.kind==TokenKind::GreaterEqual||t.kind==TokenKind::AndAnd||t.kind==TokenKind::OrOr;
        if(t.kind==TokenKind::Comma){ out<<','<<' '; }
        else if(operatorToken){ out<<' '<<t.lexeme<<' '; }
        else {
            if(!tightBefore && prev!=TokenKind::Eof && prev!=TokenKind::Comma && prev!=TokenKind::Colon && prev!=TokenKind::LBrace && prev!=TokenKind::Semicolon && t.kind!=TokenKind::LParen) out<<' ';
            out<<t.lexeme;
            if(t.kind==TokenKind::Colon)out<<' ';
        }
        prev=t.kind;
    }
    if(!lineStart)newline();
    return out.str();
}
} // namespace noe
