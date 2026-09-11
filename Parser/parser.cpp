#include "noe.hpp"
#include <stdexcept>

namespace noe {
namespace { struct ParseError : std::runtime_error { using std::runtime_error::runtime_error; }; }

Parser::Parser(std::vector<Token> tokens, Diagnostics& diagnostics)
    : tokens_(std::move(tokens)), diagnostics_(diagnostics) {}

Program Parser::parse() {
    Program p;
    while (!check(TokenKind::Eof)) {
        try { p.statements.push_back(declaration()); }
        catch (const ParseError&) { synchronize(); }
    }
    return p;
}

StmtPtr Parser::declaration() {
    if (match({TokenKind::Function})) return functionDeclaration();
    if (match({TokenKind::Let})) return letDeclaration(false);
    if (match({TokenKind::Const})) return letDeclaration(true);
    return statement();
}

StmtPtr Parser::functionDeclaration() {
    const Token& name = consume(TokenKind::Identifier, "expected function name");
    consume(TokenKind::LParen, "expected '(' after function name");
    std::vector<Parameter> params;
    if (!check(TokenKind::RParen)) {
        do {
            const Token& p = consume(TokenKind::Identifier, "expected parameter name");
            std::optional<std::string> type;
            if (match({TokenKind::Colon})) type = consume(TokenKind::Identifier, "expected parameter type").lexeme;
            params.push_back(Parameter{p.lexeme, type, p.span});
        } while (match({TokenKind::Comma}));
    }
    consume(TokenKind::RParen, "expected ')' after parameters");
    std::optional<std::string> ret;
    if (match({TokenKind::Colon})) ret = consume(TokenKind::Identifier, "expected return type").lexeme;
    consume(TokenKind::LBrace, "expected '{' before function body");
    auto fn = std::make_shared<FunctionStmt>();
    fn->span = name.span;
    fn->name = name.lexeme;
    fn->params = std::move(params);
    fn->returnType = ret;
    fn->body = block();
    return fn;
}

StmtPtr Parser::letDeclaration(bool isConst) {
    const Token& name = consume(TokenKind::Identifier, "expected variable name");
    std::optional<std::string> annotation;
    if (match({TokenKind::Colon})) annotation = consume(TokenKind::Identifier, "expected type name").lexeme;
    consume(TokenKind::Equal, "expected '=' after variable name");
    auto s = std::make_shared<LetStmt>();
    s->span = name.span;
    s->isConst = isConst;
    s->name = name.lexeme;
    s->annotation = annotation;
    s->initializer = expression();
    match({TokenKind::Semicolon});
    return s;
}

StmtPtr Parser::statement() {
    if (match({TokenKind::If})) return ifStatement();
    if (match({TokenKind::While})) return whileStatement();
    if (match({TokenKind::Return})) return returnStatement();
    if (match({TokenKind::LBrace})) return block();
    return expressionStatement();
}

StmtPtr Parser::ifStatement() {
    const Token start = previous();
    ExprPtr cond;
    if (match({TokenKind::LParen})) {
        cond = expression();
        consume(TokenKind::RParen, "expected ')' after if condition");
    } else cond = expression();
    StmtPtr thenBranch = statement();
    StmtPtr elseBranch;
    if (match({TokenKind::Else})) elseBranch = statement();
    auto s = std::make_shared<IfStmt>();
    s->span = start.span; s->condition = cond; s->thenBranch = thenBranch; s->elseBranch = elseBranch;
    return s;
}

StmtPtr Parser::whileStatement() {
    const Token start = previous();
    ExprPtr cond;
    if (match({TokenKind::LParen})) {
        cond = expression();
        consume(TokenKind::RParen, "expected ')' after while condition");
    } else cond = expression();
    auto s = std::make_shared<WhileStmt>();
    s->span = start.span; s->condition = cond; s->body = statement();
    return s;
}

StmtPtr Parser::returnStatement() {
    const Token start = previous();
    auto s = std::make_shared<ReturnStmt>();
    s->span = start.span;
    if (!check(TokenKind::Semicolon) && !check(TokenKind::RBrace) && !check(TokenKind::Eof)) s->value = expression();
    match({TokenKind::Semicolon});
    return s;
}

std::shared_ptr<BlockStmt> Parser::block() {
    auto b = std::make_shared<BlockStmt>();
    b->span = previous().span;
    while (!check(TokenKind::RBrace) && !check(TokenKind::Eof)) {
        try { b->statements.push_back(declaration()); }
        catch (const ParseError&) { synchronize(); }
    }
    consume(TokenKind::RBrace, "expected '}' after block");
    return b;
}

StmtPtr Parser::expressionStatement() {
    auto s = std::make_shared<ExprStmt>();
    s->expr = expression();
    s->span = s->expr->span;
    match({TokenKind::Semicolon});
    return s;
}

ExprPtr Parser::expression() { return assignment(); }
ExprPtr Parser::assignment() {
    auto left = logicalOr();
    if (match({TokenKind::Equal})) {
        Token eq = previous();
        auto value = assignment();
        auto name = std::dynamic_pointer_cast<NameExpr>(left);
        if (!name) {
            diagnostics_.error("NOE-P2004", eq.span, "invalid assignment target");
            return left;
        }
        auto b = std::make_shared<BinaryExpr>();
        b->span = eq.span; b->left = left; b->op = TokenKind::Equal; b->right = value;
        return b;
    }
    return left;
}

static ExprPtr makeBinary(ExprPtr l, TokenKind op, ExprPtr r, Span span) {
    auto b = std::make_shared<BinaryExpr>();
    b->left = std::move(l); b->op = op; b->right = std::move(r); b->span = span;
    return b;
}

ExprPtr Parser::logicalOr() { auto e=logicalAnd(); while(match({TokenKind::OrOr})) { auto op=previous(); e=makeBinary(e,op.kind,logicalAnd(),op.span);} return e; }
ExprPtr Parser::logicalAnd(){ auto e=equality(); while(match({TokenKind::AndAnd})) { auto op=previous(); e=makeBinary(e,op.kind,equality(),op.span);} return e; }
ExprPtr Parser::equality(){ auto e=comparison(); while(match({TokenKind::EqualEqual,TokenKind::BangEqual})) { auto op=previous(); e=makeBinary(e,op.kind,comparison(),op.span);} return e; }
ExprPtr Parser::comparison(){ auto e=term(); while(match({TokenKind::Less,TokenKind::LessEqual,TokenKind::Greater,TokenKind::GreaterEqual})) { auto op=previous(); e=makeBinary(e,op.kind,term(),op.span);} return e; }
ExprPtr Parser::term(){ auto e=factor(); while(match({TokenKind::Plus,TokenKind::Minus})) { auto op=previous(); e=makeBinary(e,op.kind,factor(),op.span);} return e; }
ExprPtr Parser::factor(){ auto e=unary(); while(match({TokenKind::Star,TokenKind::Slash,TokenKind::Percent})) { auto op=previous(); e=makeBinary(e,op.kind,unary(),op.span);} return e; }
ExprPtr Parser::unary(){ if(match({TokenKind::Bang,TokenKind::Minus,TokenKind::Plus})) { auto op=previous(); auto u=std::make_shared<UnaryExpr>(); u->op=op.kind; u->operand=unary(); u->span=op.span; return u;} return call(); }

ExprPtr Parser::call(){
    auto e=primary();
    while (match({TokenKind::LParen})) {
        auto c=std::make_shared<CallExpr>();
        c->callee=e; c->span=previous().span;
        if(!check(TokenKind::RParen)) do { c->args.push_back(expression()); } while(match({TokenKind::Comma}));
        consume(TokenKind::RParen,"expected ')' after arguments");
        e=c;
    }
    return e;
}

ExprPtr Parser::primary(){
    if(match({TokenKind::Integer})) { auto x=std::make_shared<LiteralExpr>(); x->span=previous().span; x->value=static_cast<std::int64_t>(std::stoll(previous().lexeme)); return x; }
    if(match({TokenKind::Float})) { auto x=std::make_shared<LiteralExpr>(); x->span=previous().span; x->value=std::stod(previous().lexeme); return x; }
    if(match({TokenKind::True,TokenKind::False})) { auto x=std::make_shared<LiteralExpr>(); x->span=previous().span; x->value=previous().kind==TokenKind::True; return x; }
    if(match({TokenKind::Null})) { auto x=std::make_shared<LiteralExpr>(); x->span=previous().span; x->value=std::monostate{}; return x; }
    if(match({TokenKind::String})) { auto x=std::make_shared<LiteralExpr>(); x->span=previous().span; auto s=previous().lexeme; x->value=s.size()>=2?s.substr(1,s.size()-2):std::string{}; return x; }
    if(match({TokenKind::Identifier})) { auto x=std::make_shared<NameExpr>(); x->span=previous().span; x->name=previous().lexeme; return x; }
    if(match({TokenKind::LParen})) { auto e=expression(); consume(TokenKind::RParen,"expected ')' after expression"); return e; }
    diagnostics_.error("NOE-P2001", peek().span, "expected expression");
    throw ParseError("parse");
}

bool Parser::match(std::initializer_list<TokenKind> kinds){ for(auto k:kinds) if(check(k)){advance(); return true;} return false; }
bool Parser::check(TokenKind kind) const { return peek().kind==kind; }
const Token& Parser::advance(){ if(!check(TokenKind::Eof)) ++current_; return previous(); }
const Token& Parser::previous() const { return tokens_[current_-1]; }
const Token& Parser::peek() const { return tokens_[current_]; }
const Token& Parser::consume(TokenKind kind,const std::string& message){ if(check(kind)) return advance(); diagnostics_.error("NOE-P2000",peek().span,message); throw ParseError("parse"); }
void Parser::synchronize(){ if(!check(TokenKind::Eof)) advance(); while(!check(TokenKind::Eof)){ if(previous().kind==TokenKind::Semicolon) return; switch(peek().kind){case TokenKind::Function:case TokenKind::Let:case TokenKind::Const:case TokenKind::If:case TokenKind::While:case TokenKind::Return:return;default:break;} advance(); } }

} // namespace noe
