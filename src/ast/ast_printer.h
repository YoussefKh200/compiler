#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include "visitor.h"
#include <sstream>
#include <iostream>

namespace compiler {

class ASTPrinter : public ASTVisitor {
public:
    explicit ASTPrinter(std::ostream& out = std::cout, int indent = 2)
        : out_(out), indentSize_(indent), currentIndent_(0) {}
    
    std::string print(ASTNode& node) {
        buffer_.str("");
        node.accept(*this);
        return buffer_.str();
    }
    
    void visit(BinaryExpr& node) override {
        printIndent();
        buffer_ << "BinaryExpr(" << binaryOpToString(node.op) << ")\n";
        indent();
        node.left->accept(*this);
        node.right->accept(*this);
        dedent();
    }
    
    void visit(UnaryExpr& node) override {
        printIndent();
        buffer_ << "UnaryExpr(" << unaryOpToString(node.op) << ")\n";
        indent();
        node.operand->accept(*this);
        dedent();
    }
    
    void visit(LiteralExpr& node) override {
        printIndent();
        buffer_ << "LiteralExpr(";
        switch (node.valueType) {
            case LiteralExpr::ValueType::Int: buffer_ << std::get<int64_t>(node.value); break;
            case LiteralExpr::ValueType::Float: buffer_ << std::get<double>(node.value); break;
            case LiteralExpr::ValueType::Bool: buffer_ << (std::get<bool>(node.value) ? "true" : "false"); break;
            case LiteralExpr::ValueType::String: buffer_ << "\"" << std::get<std::string>(node.value) << "\""; break;
            case LiteralExpr::ValueType::Null: buffer_ << "null"; break;
        }
        buffer_ << ")\n";
    }
    
    void visit(IdentifierExpr& node) override {
        printIndent();
        buffer_ << "IdentifierExpr(" << node.name << ")\n";
    }
    
    void visit(CallExpr& node) override {
        printIndent();
        buffer_ << "CallExpr\n";
        indent();
        node.callee->accept(*this);
        for (auto& arg : node.arguments) {
            arg->accept(*this);
        }
        dedent();
    }
    
    void visit(IndexExpr& node) override {
        printIndent();
        buffer_ << "IndexExpr\n";
        indent();
        node.base->accept(*this);
        node.index->accept(*this);
        dedent();
    }
    
    void visit(MemberExpr& node) override {
        printIndent();
        buffer_ << "MemberExpr(" << (node.isArrow ? "->" : ".") << node.member << ")\n";
        indent();
        node.object->accept(*this);
        dedent();
    }
    
    void visit(AssignExpr& node) override {
        printIndent();
        buffer_ << "AssignExpr\n";
        indent();
        node.target->accept(*this);
        node.value->accept(*this);
        dedent();
    }
    
    void visit(ArrayExpr& node) override {
        printIndent();
        buffer_ << "ArrayExpr[" << node.elements.size() << "]\n";
        indent();
        for (auto& elem : node.elements) {
            elem->accept(*this);
        }
        dedent();
    }
    
    void visit(ExprStmt& node) override {
        printIndent();
        buffer_ << "ExprStmt\n";
        indent();
        node.expression->accept(*this);
        dedent();
    }
    
    void visit(VarDeclStmt& node) override {
        printIndent();
        buffer_ << "VarDeclStmt(" << (node.isConstant ? "const " : node.isMutable ? "mut " : "")
                << node.name << ")\n";
        indent();
        if (node.type) node.type->accept(*this);
        if (node.initializer) node.initializer->accept(*this);
        dedent();
    }
    
    void visit(BlockStmt& node) override {
        printIndent();
        buffer_ << "BlockStmt[" << node.statements.size() << "]\n";
        indent();
        for (auto& stmt : node.statements) {
            stmt->accept(*this);
        }
        dedent();
    }
    
    void visit(IfStmt& node) override {
        printIndent();
        buffer_ << "IfStmt\n";
        indent();
        node.condition->accept(*this);
        node.thenBranch->accept(*this);
        if (node.elseBranch) node.elseBranch->accept(*this);
        dedent();
    }
    
    void visit(WhileStmt& node) override {
        printIndent();
        buffer_ << "WhileStmt\n";
        indent();
        node.condition->accept(*this);
        node.body->accept(*this);
        dedent();
    }
    
    void visit(ForStmt& node) override {
        printIndent();
        buffer_ << "ForStmt\n";
        indent();
        if (node.initializer) node.initializer->accept(*this);
        if (node.condition) node.condition->accept(*this);
        if (node.increment) node.increment->accept(*this);
        node.body->accept(*this);
        dedent();
    }
    
    void visit(ReturnStmt& node) override {
        printIndent();
        buffer_ << "ReturnStmt\n";
        indent();
        if (node.value) node.value->accept(*this);
        dedent();
    }
    
    void visit(BreakStmt& node) override {
        printIndent();
        buffer_ << "BreakStmt\n";
    }
    
    void visit(ContinueStmt& node) override {
        printIndent();
        buffer_ << "ContinueStmt\n";
    }
    
    void visit(FunctionDecl& node) override {
        printIndent();
        buffer_ << "FunctionDecl(" << node.name << ", params=" << node.parameters.size() << ")\n";
        indent();
        if (node.returnType) node.returnType->accept(*this);
        if (node.body) node.body->accept(*this);
        dedent();
    }
    
    void visit(StructDecl& node) override {
        printIndent();
        buffer_ << "StructDecl(" << node.name << ")\n";
        indent();
        for (auto& field : node.fields) {
            printIndent();
            buffer_ << "Field(" << field.name << ")\n";
        }
        dedent();
    }
    
    void visit(EnumDecl& node) override {
        printIndent();
        buffer_ << "EnumDecl(" << node.name << ")\n";
        indent();
        for (auto& variant : node.variants) {
            printIndent();
            buffer_ << "Variant(" << variant.name << ")\n";
        }
        dedent();
    }
    
    void visit(PrimitiveType& node) override {
        printIndent();
        buffer_ << "PrimitiveType(" << PrimitiveType::getTypeName(node.typeCode) << ")\n";
    }
    
    void visit(ArrayType& node) override {
        printIndent();
        buffer_ << "ArrayType[" << node.size << "]\n";
        indent();
        node.elementType->accept(*this);
        dedent();
    }
    
    void visit(PointerType& node) override {
        printIndent();
        buffer_ << "PointerType(" << (node.isMutable ? "mut" : "const") << ")\n";
        indent();
        node.pointeeType->accept(*this);
        dedent();
    }
    
    void visit(FunctionType& node) override {
        printIndent();
        buffer_ << "FunctionType\n";
        indent();
        for (auto& param : node.paramTypes) {
            param->accept(*this);
        }
        node.returnType->accept(*this);
        dedent();
    }
    
    void visit(NamedType& node) override {
        printIndent();
        buffer_ << "NamedType(" << node.name << ")\n";
    }
    
    void visit(Program& node) override {
        printIndent();
        buffer_ << "Program[" << node.declarations.size() << "]\n";
        indent();
        for (auto& decl : node.declarations) {
            decl->accept(*this);
        }
        dedent();
    }

private:
    std::ostream& out_;
    std::stringstream buffer_;
    int indentSize_;
    int currentIndent_;
    
    void indent() { currentIndent_ += indentSize_; }
    void dedent() { currentIndent_ -= indentSize_; }
    
    void printIndent() {
        for (int i = 0; i < currentIndent_; ++i) buffer_ << ' ';
    }
    
    std::string binaryOpToString(BinaryExpr::Op op) {
        switch (op) {
            case BinaryExpr::Op::Add: return "+";
            case BinaryExpr::Op::Sub: return "-";
            case BinaryExpr::Op::Mul: return "*";
            case BinaryExpr::Op::Div: return "/";
            case BinaryExpr::Op::Mod: return "%";
            case BinaryExpr::Op::Eq: return "==";
            case BinaryExpr::Op::Ne: return "!=";
            case BinaryExpr::Op::Lt: return "<";
            case BinaryExpr::Op::Le: return "<=";
            case BinaryExpr::Op::Gt: return ">";
            case BinaryExpr::Op::Ge: return ">=";
            case BinaryExpr::Op::And: return "&&";
            case BinaryExpr::Op::Or: return "||";
            case BinaryExpr::Op::BitAnd: return "&";
            case BinaryExpr::Op::BitOr: return "|";
            case BinaryExpr::Op::BitXor: return "^";
            case BinaryExpr::Op::Shl: return "<<";
            case BinaryExpr::Op::Shr: return ">>";
        }
        return "unknown";
    }
    
    std::string unaryOpToString(UnaryExpr::Op op) {
        switch (op) {
            case UnaryExpr::Op::Neg: return "-";
            case UnaryExpr::Op::Not: return "!";
            case UnaryExpr::Op::BitNot: return "~";
            case UnaryExpr::Op::Deref: return "*";
            case UnaryExpr::Op::Ref: return "&";
            case UnaryExpr::Op::Inc: return "++";
            case UnaryExpr::Op::Dec: return "--";
        }
        return "unknown";
    }
};

} // namespace compiler

#endif // AST_PRINTER_H
