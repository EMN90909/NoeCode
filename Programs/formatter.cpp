#include "noe.hpp"
#include <algorithm>
#include <string>

namespace noe {
namespace {
bool isOperator(TokenKind k) {
    switch (k) {
        case TokenKind::Plus: case TokenKind::Minus: case TokenKind::Star: case TokenKind::Slash: case TokenKind::Percent:
        case TokenKind::Equal: case TokenKind::EqualEqual: case TokenKind::BangEqual: case TokenKind::Less: case TokenKind::LessEqual:
        case TokenKind::Greater: case TokenKind::GreaterEqual: case TokenKind::AndAnd: case TokenKind::OrOr: return true;
        default: return false;
    }
}
}

std::string Formatter::format(const std::string& source, Diagnostics& diagnostics) const {
    Lexer lexer(source, diagnostics); auto tokens = lexer.lex();
    if (diagnostics.hasErrors()) return source;
    std::string out; int indent = 0; bool lineStart = true; TokenKind prev = TokenKind::Eof;
    auto writeIndent = [&] { if (lineStart) { out.append(static_cast<std::size_t>(indent) * 4, ' '); lineStart = false; } };
    auto trimSpace = [&] { while (!out.empty() && (out.back() == ' ' || out.back() == '\t')) out.pop_back(); };
    auto space = [&] { if (!out.empty() && out.back() != ' ' && out.back() != '\n') out.push_back(' '); };
    auto newline = [&] { trimSpace(); if (out.empty() || out.back() != '\n') out.push_back('\n'); lineStart = true; };

    for (const auto& t : tokens) {
        if (t.kind == TokenKind::Eof) break;
        if (t.kind == TokenKind::RBrace) { if (!lineStart) newline(); indent = std::max(0, indent - 1); writeIndent(); out.push_back('}'); newline(); prev = t.kind; continue; }
        if (t.kind == TokenKind::LBrace) { writeIndent(); space(); out.push_back('{'); newline(); ++indent; prev = t.kind; continue; }
        if (t.kind == TokenKind::Semicolon) { trimSpace(); out.push_back(';'); newline(); prev = t.kind; continue; }
        writeIndent();
        if (t.kind == TokenKind::Comma) { trimSpace(); out += ", "; }
        else if (t.kind == TokenKind::Colon) { trimSpace(); out += ": "; }
        else if (t.kind == TokenKind::Dot) { trimSpace(); out.push_back('.'); }
        else if (t.kind == TokenKind::LParen) { if (prev == TokenKind::If || prev == TokenKind::While) space(); else trimSpace(); out.push_back('('); }
        else if (t.kind == TokenKind::RParen) { trimSpace(); out.push_back(')'); }
        else if (isOperator(t.kind)) { trimSpace(); space(); out += t.lexeme; out.push_back(' '); }
        else {
            bool tightAfter = prev == TokenKind::LParen || prev == TokenKind::Dot || prev == TokenKind::Colon || prev == TokenKind::Comma;
            if (!tightAfter && prev != TokenKind::Eof && prev != TokenKind::LBrace && !isOperator(prev)) space();
            out += t.lexeme;
        }
        prev = t.kind;
    }
    if (!lineStart) newline();
    return out;
}
} // namespace noe
