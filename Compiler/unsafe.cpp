#include "noe.hpp"
#include <cctype>
#include <string>
#include <unordered_set>
#include <vector>

namespace noe {
namespace {

struct Lexeme {
    std::string text;
    std::size_t begin = 0;
    std::size_t end = 0;
    std::size_t line = 1;
    std::size_t column = 1;
};

bool identStart(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool identContinue(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

std::vector<Lexeme> scan(const std::string& source) {
    std::vector<Lexeme> out;
    std::size_t i = 0, line = 1, column = 1;
    auto advance = [&](char c) {
        ++i;
        if (c == '\n') { ++line; column = 1; }
        else ++column;
    };

    while (i < source.size()) {
        char c = source[i];
        if (std::isspace(static_cast<unsigned char>(c))) { advance(c); continue; }
        if (c == '/' && i + 1 < source.size() && source[i + 1] == '/') {
            while (i < source.size() && source[i] != '\n') advance(source[i]);
            continue;
        }
        if (c == '/' && i + 1 < source.size() && source[i + 1] == '*') {
            advance(source[i]); advance(source[i]);
            while (i + 1 < source.size() && !(source[i] == '*' && source[i + 1] == '/')) advance(source[i]);
            if (i + 1 < source.size()) { advance(source[i]); advance(source[i]); }
            continue;
        }
        if (c == '"') {
            const auto begin = i, l = line, col = column;
            advance(c);
            while (i < source.size()) {
                char n = source[i];
                if (n == '\\' && i + 1 < source.size()) { advance(n); advance(source[i]); continue; }
                advance(n);
                if (n == '"') break;
            }
            out.push_back({"<string>", begin, i, l, col});
            continue;
        }
        if (identStart(c)) {
            const auto begin = i, l = line, col = column;
            advance(c);
            while (i < source.size() && identContinue(source[i])) advance(source[i]);
            out.push_back({source.substr(begin, i - begin), begin, i, l, col});
            continue;
        }
        const auto begin = i, l = line, col = column;
        std::string text(1, c);
        advance(c);
        if (i < source.size()) {
            const std::string pair = text + source[i];
            if (pair == "==" || pair == "!=" || pair == "<=" || pair == ">=" || pair == "&&" || pair == "||") {
                text = pair;
                advance(source[i]);
            }
        }
        out.push_back({text, begin, i, l, col});
    }
    return out;
}

Span spanOf(const Lexeme& token, SourceId sourceId) {
    return Span{token.begin, token.end, token.line, token.column, sourceId};
}

bool unaryPointerContext(const std::vector<Lexeme>& tokens, std::size_t i) {
    if (tokens[i].text != "*" || i + 1 >= tokens.size()) return false;
    if (!(identStart(tokens[i + 1].text.empty() ? '\0' : tokens[i + 1].text[0]) || tokens[i + 1].text == "(")) return false;
    if (i == 0) return true;
    const auto& p = tokens[i - 1].text;
    static const std::unordered_set<std::string> prefix = {
        "(", "{", "[", ",", "=", ":", ";", "return", "let", "const", "!", "+", "-", "/", "%", "&&", "||"
    };
    return prefix.count(p) != 0;
}

} // namespace

UnsafePreprocessResult preprocessUnsafeSyntax(const std::string& source,
                                              Diagnostics& diagnostics,
                                              SourceId sourceId) {
    UnsafePreprocessResult result{source, false};
    const auto tokens = scan(source);
    std::vector<int> unsafeAtDepth;
    int braceDepth = 0;
    std::unordered_set<std::string> externNames;

    for (std::size_t i = 0; i + 2 < tokens.size(); ++i) {
        if (tokens[i].text == "extern" && tokens[i + 1].text == "function") externNames.insert(tokens[i + 2].text);
    }

    auto inUnsafe = [&]() { return !unsafeAtDepth.empty(); };
    auto requireUnsafe = [&](const Lexeme& token, const std::string& operation) {
        if (!inUnsafe()) {
            diagnostics.error("NQR-T3060", spanOf(token, sourceId),
                              operation + " requires an explicit unsafe block",
                              "wrap the smallest audited region in `unsafe { ... }`");
        }
    };

    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        if (token.text == "unsafe") {
            if (i + 1 >= tokens.size() || tokens[i + 1].text != "{") {
                diagnostics.error("NQR-P2060", spanOf(token, sourceId),
                                  "unsafe must be followed by a block",
                                  "write `unsafe { ... }`");
                continue;
            }
            for (std::size_t p = token.begin; p < token.end; ++p) result.source[p] = ' ';
            result.foundUnsafeBlock = true;
            unsafeAtDepth.push_back(braceDepth + 1);
            continue;
        }

        if (token.text == "{") {
            ++braceDepth;
            continue;
        }
        if (token.text == "}") {
            if (!unsafeAtDepth.empty() && unsafeAtDepth.back() == braceDepth) unsafeAtDepth.pop_back();
            if (braceDepth > 0) --braceDepth;
            continue;
        }

        if (token.text == "asm") requireUnsafe(token, "inline assembly");
        else if (token.text == "intrinsic") requireUnsafe(token, "machine intrinsic access");
        else if (token.text == "host" || token.text == "abi") requireUnsafe(token, "unrestricted host/ABI invocation");
        else if (token.text == "volatile") requireUnsafe(token, "volatile memory access");
        else if (token.text == "*" && unaryPointerContext(tokens, i)) requireUnsafe(token, "raw pointer dereference");
        else if (token.text == "as" && i + 1 < tokens.size()) {
            const auto& target = tokens[i + 1].text;
            if (target == "*" || target == "usize" || target == "isize") requireUnsafe(token, "pointer/integer reinterpretation cast");
        } else if (externNames.count(token.text) && i + 1 < tokens.size() && tokens[i + 1].text == "(") {
            requireUnsafe(token, "call to extern function '" + token.text + "'");
        }
    }

    return result;
}

} // namespace noe
