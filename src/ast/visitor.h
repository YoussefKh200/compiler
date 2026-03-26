#ifndef VISITOR_H
#define VISITOR_H

#include "ast_nodes.h"

namespace compiler {

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    
    // Expressions
    virtual void visit(BinaryExpr& node) = 0;
    virtual void visit(UnaryExpr& node) = 0;
    virtual void visit(LiteralExpr& node) = 0;
    virtual void visit(IdentifierExpr& node) = 0;
    virtual void visit(CallExpr& node) = 0;
    virtual void visit(IndexExpr& node) = 0;
    virtual void visit(MemberExpr& node) = 0;
    virtual void visit(AssignExpr& node) = 0;
    virtual void visit(ArrayExpr& node) = 0;
    
    // Statements
    virtual void visit(ExprStmt& node) = 0;
    virtual void visit(VarDeclStmt& node) = 0;
    virtual void visit(BlockStmt& node) = 0;
    virtual void visit(IfStmt& node) = 0;
    virtual void visit(WhileStmt& node) = 0;
    virtual void visit(ForStmt& node) = 0;
    virtual void visit(ReturnStmt& node) = 0;
    virtual void visit(BreakStmt& node) = 0;
    virtual void visit(ContinueStmt& node) = 0;
    
    // Declarations
    virtual void visit(FunctionDecl& node) = 0;
    virtual void visit(StructDecl& node) = 0;
    virtual void visit(EnumDecl& node) = 0;
    
    // Types
    virtual void visit(PrimitiveType& node) = 0;
    virtual void visit(ArrayType& node) = 0;
    virtual void visit(PointerType& node) = 0;
    virtual void visit(FunctionType& node) = 0;
    virtual void visit(NamedType& node) = 0;
    
    // Program
    virtual void visit(Program& node) = 0;
};

// Accept implementations
inline void BinaryExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
inline void UnaryExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
inline void LiteralExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
inline void IdentifierExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
inline void CallExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
inline void IndexExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
inline void MemberExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
inline void AssignExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }
inline void ArrayExpr::accept(ASTVisitor& visitor) { visitor.visit(*this); }

inline void ExprStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
inline void VarDeclStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
inline void BlockStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
inline void IfStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
inline void WhileStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
inline void ForStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
inline void ReturnStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
inline void BreakStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }
inline void ContinueStmt::accept(ASTVisitor& visitor) { visitor.visit(*this); }

inline void FunctionDecl::accept(ASTVisitor& visitor) { visitor.visit(*this); }
inline void StructDecl::accept(ASTVisitor& visitor) { visitor.visit(*this); }
inline void EnumDecl::accept(ASTVisitor& visitor) { visitor.visit(*this); }

inline void PrimitiveType::accept(ASTVisitor& visitor) { visitor.visit(*this); }
inline void ArrayType::accept(ASTVisitor& visitor) { visitor.visit(*this); }
inline void PointerType::accept(ASTVisitor& visitor) { visitor.visit(*this); }
inline void FunctionType::accept(ASTVisitor& visitor) { visitor.visit(*this); }
inline void NamedType::accept(ASTVisitor& visitor) { visitor.visit(*this); }

inline void Program::accept(ASTVisitor& visitor) { visitor.visit(*this); }

} // namespace compiler

#endif // VISITOR_H
