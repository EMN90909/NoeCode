#include "noe.hpp"
#include <cctype>
#include <unordered_map>

namespace noe {

Lexer::Lexer(std::string source, Diagnostics& diagnostics)
    : source_(std::move(source)), diagnostics_(diagnostics) {}

char Lexer::peek(std::size_t offset) const {
    const auto i = current_ + offset;
    return i < source_.size() ? source_[i] : '\0';
}

char Lexer::advance() {
    if (current_ >= source_.size()) return '\0';
    char c = source_[current_++];
    if (c == '\n') { ++line_; column_ = 1; }
    else { ++column_; }
    return c;
}

bool Lexer::match(char expected) {
    if (peek() != expected) return false;
    advance();
    return true;
}

Token Lexer::make(TokenKind kind, std::size_t start, std::size_t line, std::size_t col) {
    return Token{kind, source_.substr(start, current_ - start), Span{start, current_, line, col}};
}

void Lexer::skipWhitespaceAndComments() {
    for (;;) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { advance(); continue; }
        if (c == '/' && peek(1) == '/') {
            while (peek() != '\n' && peek() != '\0') advance();
            continue;
        }
        if (c == '/' && peek(1) == '*') {
            advance(); advance();
            while (peek() != '\0' && !(peek() == '*' && peek(1) == '/')) advance();
            if (peek() == '\0') {
                diagnostics_.error("NOE-L1002", Span{current_, current_, line_, column_}, "unterminated block comment");
                return;
            }
            advance(); advance();
            continue;
        }
        break;
    }
}

Token Lexer::identifier() {
    const auto start = current_ - 1;
    const auto line = line_;
    const auto col = column_ - 1;
    while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') advance();
    const std::string text = source_.substr(start, current_ - start);
    static const std::unordered_map<std::string, TokenKind> keywords = {
        {"let", TokenKind::Let}, {"const", TokenKind::Const}, {"function", TokenKind::Function},
        {"if", TokenKind::If}, {"else", TokenKind::Else}, {"while", TokenKind::While},
        {"return", TokenKind::Return}, {"true", TokenKind::True}, {"false", TokenKind::False},
        {"null", TokenKind::Null}, {"import", TokenKind::Import}, {"record", TokenKind::Record}, {"class", TokenKind::Class}
    };
    auto it = keywords.find(text);
    return Token{it == keywords.end() ? TokenKind::Identifier : it->second, text, Span{start, current_, line, col}};
}

Token Lexer::number() {
    const auto start = current_ - 1;
    const auto line = line_;
    const auto col = column_ - 1;
    while (std::isdigit(static_cast<unsigned char>(peek()))) advance();
    TokenKind kind = TokenKind::Integer;
    if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peek(1)))) {
        kind = TokenKind::Float;
        advance();
        while (std::isdigit(static_cast<unsigned char>(peek()))) advance();
    }
    return Token{kind, source_.substr(start, current_ - start), Span{start, current_, line, col}};
}

Token Lexer::stringLiteral() {
    const auto start = current_ - 1;
    const auto line = line_;
    const auto col = column_ - 1;
    while (peek() != '"' && peek() != '\0') {
        if (peek() == '\\' && peek(1) != '\0') { advance(); advance(); continue; }
        advance();
    }
    if (peek() == '\0') {
        diagnostics_.error("NOE-L1001", Span{start, current_, line, col}, "unterminated string literal");
        return Token{TokenKind::String, source_.substr(start, current_ - start), Span{start, current_, line, col}};
    }
    advance();
    return Token{TokenKind::String, source_.substr(start, current_ - start), Span{start, current_, line, col}};
}

Token Lexer::scanToken() {
    skipWhitespaceAndComments();
    const auto start = current_;
    const auto line = line_;
    const auto col = column_;
    if (current_ >= source_.size()) return Token{TokenKind::Eof, "", Span{current_, current_, line_, column_}};
    char c = advance();
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') return identifier();
    if (std::isdigit(static_cast<unsigned char>(c))) return number();
    switch (c) {
        case '(': return make(TokenKind::LParen,start,line,col); case ')': return make(TokenKind::RParen,start,line,col);
        case '{': return make(TokenKind::LBrace,start,line,col); case '}': return make(TokenKind::RBrace,start,line,col);
        case ',': return make(TokenKind::Comma,start,line,col); case ':': return make(TokenKind::Colon,start,line,col);
        case ';': return make(TokenKind::Semicolon,start,line,col); case '.': return make(TokenKind::Dot,start,line,col);
        case '+': return make(TokenKind::Plus,start,line,col); case '-': return make(TokenKind::Minus,start,line,col);
        case '*': return make(TokenKind::Star,start,line,col); case '%': return make(TokenKind::Percent,start,line,col);
        case '/': return make(TokenKind::Slash,start,line,col);
        case '!': return make(match('=') ? TokenKind::BangEqual : TokenKind::Bang,start,line,col);
        case '=': return make(match('=') ? TokenKind::EqualEqual : TokenKind::Equal,start,line,col);
        case '<': return make(match('=') ? TokenKind::LessEqual : TokenKind::Less,start,line,col);
        case '>': return make(match('=') ? TokenKind::GreaterEqual : TokenKind::Greater,start,line,col);
        case '&': if (match('&')) return make(TokenKind::AndAnd,start,line,col); break;
        case '|': if (match('|')) return make(TokenKind::OrOr,start,line,col); break;
        case '"': return stringLiteral();
    }
    diagnostics_.error("NOE-L1000", Span{start,current_,line,col}, std::string("unexpected character '") + c + "'");
    return make(TokenKind::Eof,start,line,col);
}

std::vector<Token> Lexer::lex() {
    std::vector<Token> out;
    for (;;) {
        Token t = scanToken();
        out.push_back(t);
        if (t.kind == TokenKind::Eof) break;
    }
    return out;
}

} // namespace noe
