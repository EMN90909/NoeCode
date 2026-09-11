#include "noe.hpp"
#include <algorithm>
#include <sstream>

namespace noe {
namespace {
std::string opText(TokenKind k){
    switch(k){
        case TokenKind::Plus:return "+"; case TokenKind::Minus:return "-"; case TokenKind::Star:return "*"; case TokenKind::Slash:return "/"; case TokenKind::Percent:return "%";
        case TokenKind::Bang:return "!"; case TokenKind::EqualEqual:return "=="; case TokenKind::BangEqual:return "!=";
        case TokenKind::Less:return "<"; case TokenKind::LessEqual:return "<="; case TokenKind::Greater:return ">"; case TokenKind::GreaterEqual:return ">=";
        case TokenKind::AndAnd:return "&&"; case TokenKind::OrOr:return "||"; default:return "?";
    }
}
std::string literalText(const NirValue& v){
    if(std::holds_alternative<std::monostate>(v)) return "null";
    if(auto p=std::get_if<std::int64_t>(&v)) return std::to_string(*p);
    if(auto p=std::get_if<double>(&v)){ std::ostringstream o; o<<*p; return o.str(); }
    if(auto p=std::get_if<bool>(&v)) return *p?"true":"false";
    return '"'+std::get<std::string>(v)+'"';
}
}

Reg Lowerer::emit(NirFunction& fn, NirInstruction ins){
    if(ins.dest) fn.nextReg=std::max(fn.nextReg,Reg(*ins.dest+1));
    fn.code.push_back(std::move(ins));
    return fn.code.back().dest.value_or(0);
}

NirProgram Lowerer::lower(const Program& program){
    NirProgram out;
    out.entry.name="__entry";
    for(const auto& s:program.statements){
        if(auto f=std::dynamic_pointer_cast<FunctionStmt>(s)){
            NirFunction nf;
            nf.name=f->name;
            for(auto& p:f->params) nf.params.push_back(p.name);
            for(auto& x:f->body->statements) lowerStmt(nf,x);
            if(nf.code.empty()||nf.code.back().op!=NirOp::Return){ NirInstruction r; r.op=NirOp::Return; nf.code.push_back(r); }
            out.functions.push_back(std::move(nf));
        } else lowerStmt(out.entry,s);
    }
    NirInstruction r; r.op=NirOp::Return; out.entry.code.push_back(r);
    return out;
}

void Lowerer::lowerStmt(NirFunction& fn,const StmtPtr& stmt){
    if(auto s=std::dynamic_pointer_cast<LetStmt>(stmt)){ Reg r=lowerExpr(fn,s->initializer); NirInstruction i; i.op=NirOp::Store;i.text=s->name;i.args={r}; emit(fn,std::move(i)); return; }
    if(auto s=std::dynamic_pointer_cast<ExprStmt>(stmt)){ lowerExpr(fn,s->expr); return; }
    if(auto s=std::dynamic_pointer_cast<BlockStmt>(stmt)){ for(auto& x:s->statements) lowerStmt(fn,x); return; }
    if(auto s=std::dynamic_pointer_cast<ReturnStmt>(stmt)){ NirInstruction i; i.op=NirOp::Return; if(s->value) i.args={lowerExpr(fn,s->value)}; emit(fn,std::move(i)); return; }
    if(auto s=std::dynamic_pointer_cast<IfStmt>(stmt)){
        Reg c=lowerExpr(fn,s->condition);
        NirInstruction j; j.op=NirOp::JumpIfFalse;j.args={c}; auto jpos=fn.code.size(); emit(fn,std::move(j));
        lowerStmt(fn,s->thenBranch);
        if(s->elseBranch){
            NirInstruction end;end.op=NirOp::Jump;auto epos=fn.code.size();emit(fn,std::move(end));
            fn.code[jpos].target=fn.code.size();
            lowerStmt(fn,s->elseBranch);
            fn.code[epos].target=fn.code.size();
        } else fn.code[jpos].target=fn.code.size();
        return;
    }
    if(auto s=std::dynamic_pointer_cast<WhileStmt>(stmt)){
        auto start=fn.code.size();
        Reg c=lowerExpr(fn,s->condition);
        NirInstruction j;j.op=NirOp::JumpIfFalse;j.args={c};auto jpos=fn.code.size();emit(fn,std::move(j));
        lowerStmt(fn,s->body);
        NirInstruction back;back.op=NirOp::Jump;back.target=start;emit(fn,std::move(back));
        fn.code[jpos].target=fn.code.size();
    }
}

Reg Lowerer::lowerExpr(NirFunction& fn,const ExprPtr& expr){
    if(auto e=std::dynamic_pointer_cast<LiteralExpr>(expr)){ NirInstruction i;i.op=NirOp::Const;i.dest=fn.nextReg++;i.literal=e->value;emit(fn,i);return *i.dest; }
    if(auto e=std::dynamic_pointer_cast<NameExpr>(expr)){ NirInstruction i;i.op=NirOp::Load;i.dest=fn.nextReg++;i.text=e->name;emit(fn,i);return *i.dest; }
    if(auto e=std::dynamic_pointer_cast<UnaryExpr>(expr)){ Reg a=lowerExpr(fn,e->operand);NirInstruction i;i.op=NirOp::Unary;i.dest=fn.nextReg++;i.text=opText(e->op);i.args={a};emit(fn,i);return *i.dest; }
    if(auto e=std::dynamic_pointer_cast<BinaryExpr>(expr)){
        if(e->op==TokenKind::Equal){ auto n=std::dynamic_pointer_cast<NameExpr>(e->left);Reg r=lowerExpr(fn,e->right);NirInstruction st;st.op=NirOp::Store;st.text=n?n->name:"<invalid>";st.args={r};emit(fn,st);return r; }
        Reg l=lowerExpr(fn,e->left),r=lowerExpr(fn,e->right);NirInstruction i;i.op=NirOp::Binary;i.dest=fn.nextReg++;i.text=opText(e->op);i.args={l,r};emit(fn,i);return *i.dest;
    }
    if(auto e=std::dynamic_pointer_cast<CallExpr>(expr)){ auto n=std::dynamic_pointer_cast<NameExpr>(e->callee);NirInstruction i;i.op=NirOp::Call;i.dest=fn.nextReg++;i.text=n?n->name:"<call>";for(auto&a:e->args)i.args.push_back(lowerExpr(fn,a));emit(fn,i);return *i.dest; }
    NirInstruction i;i.op=NirOp::Const;i.dest=fn.nextReg++;i.literal=std::monostate{};emit(fn,i);return *i.dest;
}

std::string printNir(const NirProgram& p){
    std::ostringstream out;
    auto one=[&](const NirFunction& f){
        out<<"function "<<f.name<<'(';
        for(std::size_t i=0;i<f.params.size();++i){if(i)out<<", ";out<<f.params[i];}
        out<<") {\n";
        for(std::size_t pc=0;pc<f.code.size();++pc){
            auto&i=f.code[pc]; out<<"  "<<pc<<": "; if(i.dest)out<<'%'<<*i.dest<<" = ";
            switch(i.op){
                case NirOp::Const:out<<"const "<<literalText(i.literal);break;
                case NirOp::Load:out<<"load "<<i.text;break;
                case NirOp::Store:out<<"store "<<i.text<<", %"<<i.args[0];break;
                case NirOp::Unary:out<<"unary "<<i.text<<" %"<<i.args[0];break;
                case NirOp::Binary:out<<"binary "<<i.text<<" %"<<i.args[0]<<", %"<<i.args[1];break;
                case NirOp::Call:out<<"call "<<i.text<<'(';for(std::size_t a=0;a<i.args.size();++a){if(a)out<<", ";out<<'%'<<i.args[a];}out<<')';break;
                case NirOp::Jump:out<<"jump "<<i.target;break;
                case NirOp::JumpIfFalse:out<<"jump_if_false %"<<i.args[0]<<", "<<i.target;break;
                case NirOp::Return:out<<"return"<<(i.args.empty()?"":" %"+std::to_string(i.args[0]));break;
                case NirOp::Nop:out<<"nop";break;
            }
            out<<"\n";
        }
        out<<"}\n";
    };
    one(p.entry);
    for(auto&f:p.functions)one(f);
    return out.str();
}

} // namespace noe
