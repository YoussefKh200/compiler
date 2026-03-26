#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "../ast/visitor.h"
#include "symbol_table.h"
#include "type_system.h"
#include <stack>
#include <vector>

namespace compiler {

enum class LoopType { None, While, For };

struct LoopContext {
    LoopType type;
    bool hasBreak;
    bool hasContinue;
};

class SemanticAnalyzer : public ASTVisitor {
public:
    SemanticAnalyzer();
    
    bool analyze(Program& program);
    bool hadErrors() const { return hadErrors_; }
    
    // Visitor implementations
    void visit(BinaryExpr& node) override;
    void visit(UnaryExpr& node) override;
    void visit(LiteralExpr& node) override;
    void visit(IdentifierExpr& node) override;
    void visit(CallExpr& node) override;
    void visit(IndexExpr& node) override;
    void visit(MemberExpr& node) override;
    void visit(AssignExpr& node) override;
    void visit(ArrayExpr& node) override;
    
    void visit(ExprStmt& node) override;
    void visit(VarDeclStmt& node) override;
    void visit(BlockStmt& node) override;
    void visit(IfStmt& node) override;
    void visit(WhileStmt& node) override;
    void visit(ForStmt& node) override;
    void visit(ReturnStmt& node) override;
    void visit(BreakStmt& node) override;
    void visit(ContinueStmt& node) override;
    
    void visit(FunctionDecl& node) override;
    void visit(StructDecl& node) override;
    void visit(EnumDecl& node) override;
    
    void visit(PrimitiveType& node) override;
    void visit(ArrayType& node) override;
    void visit(PointerType& node) override;
    void visit(FunctionType& node) override;
    void visit(NamedType& node) override;
    
    void visit(Program& node) override;

private:
    SymbolTable symbolTable_;
    std::shared_ptr<SemanticType> currentReturnType_;
    std::stack<LoopContext> loopStack_;
    bool hadErrors_;
    bool inFunction_;
    
    // Type tracking for expressions
    std::unordered_map<Expr*, std::shared_ptr<SemanticType>> exprTypes_;
    
    // Helper methods
    std::shared_ptr<SemanticType> resolveASTType(compiler::Type& astType);
    std::shared_ptr<SemanticType> checkExpr(Expr& expr);
    bool checkTypesEqual(std::shared_ptr<SemanticType> a, std::shared_ptr<SemanticType> b);
    void reportTypeError(SourceLocation loc, const std::string& msg);
    
    // Type checking helpers
    std::shared_ptr<SemanticType> checkBinaryOp(BinaryExpr& node);
    std::shared_ptr<SemanticType> checkUnaryOp(UnaryExpr& node);
    std::shared_ptr<SemanticType> checkCall(CallExpr& node);
    std::shared_ptr<SemanticType> checkMemberAccess(MemberExpr& node);
    
    // Control flow
    void pushLoop(LoopType type);
    void popLoop();
    bool inLoop() const;
};

} // namespace compiler

#endif // SEMANTIC_ANALYZER_H
