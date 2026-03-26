#ifndef PARSER_H
#define PARSER_H

#include "../lexer/lexer.h"
#include "../ast/ast_nodes.h"
#include "../utils/memory_pool.h"
#include <memory>
#include <vector>
#include <unordered_map>

namespace compiler {

class ParseError : public std::exception {
public:
    explicit ParseError(const std::string& msg) : message_(msg) {}
    const char* what() const noexcept override { return message_.c_str(); }
private:
    std::string message_;
};

class Parser {
public:
    explicit Parser(Lexer& lexer, MemoryPool& pool);
    
    std::unique_ptr<Program> parseProgram();
    
    // Error handling
    bool hadError() const;
    void synchronize();

private:
    Lexer& lexer_;
    MemoryPool& pool_;
    Token current_;
    Token previous_;
    bool panicMode_;
    bool hadError_;
    
    // Token handling
    void advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool match(std::initializer_list<TokenType> types);
    Token consume(TokenType type, const std::string& message);
    
    // Error recovery
    void error(const std::string& message);
    void errorAt(const Token& token, const std::string& message);
    
    // Grammar productions
    std::unique_ptr<Decl> parseDeclaration();
    std::unique_ptr<FunctionDecl> parseFunctionDecl();
    std::unique_ptr<StructDecl> parseStructDecl();
    std::unique_ptr<EnumDecl> parseEnumDecl();
    
    std::unique_ptr<Stmt> parseStatement();
    std::unique_ptr<Stmt> parseExprStatement();
    std::unique_ptr<Stmt> parseVarDeclaration();
    std::unique_ptr<Stmt> parseBlockStatement();
    std::unique_ptr<Stmt> parseIfStatement();
    std::unique_ptr<Stmt> parseWhileStatement();
    std::unique_ptr<Stmt> parseForStatement();
    std::unique_ptr<Stmt> parseReturnStatement();
    std::unique_ptr<Stmt> parseBreakStatement();
    std::unique_ptr<Stmt> parseContinueStatement();
    
    std::unique_ptr<Expr> parseExpression();
    std::unique_ptr<Expr> parseAssignment();
    std::unique_ptr<Expr> parseOr();
    std::unique_ptr<Expr> parseAnd();
    std::unique_ptr<Expr> parseEquality();
    std::unique_ptr<Expr> parseComparison();
    std::unique_ptr<Expr> parseBitwiseOr();
    std::unique_ptr<Expr> parseBitwiseXor();
    std::unique_ptr<Expr> parseBitwiseAnd();
    std::unique_ptr<Expr> parseShift();
    std::unique_ptr<Expr> parseTerm();
    std::unique_ptr<Expr> parseFactor();
    std::unique_ptr<Expr> parseUnary();
    std::unique_ptr<Expr> parsePostfix();
    std::unique_ptr<Expr> parsePrimary();
    std::unique_ptr<Expr> parseCall(std::unique_ptr<Expr> callee);
    std::unique_ptr<Expr> parseIndex(std::unique_ptr<Expr> base);
    std::unique_ptr<Expr> parseMember(std::unique_ptr<Expr> object);
    
    std::unique_ptr<Type> parseType();
    std::unique_ptr<Type> parsePrimitiveType();
    std::unique_ptr<Type> parseArrayType();
    std::unique_ptr<Type> parsePointerType();
    std::unique_ptr<Type> parseFunctionType();
    
    // Helpers
    bool isTypeToken(TokenType type) const;
    std::vector<std::unique_ptr<Expr>> parseArguments();
    std::vector<ParamDecl> parseParameters();
    BinaryExpr::Op getBinaryOp(TokenType type) const;
    UnaryExpr::Op getUnaryOp(TokenType type) const;
    
    // AST node creation helpers
    template<typename T, typename... Args>
    T* makeNode(Args&&... args) {
        return pool_.allocate<T>(std::forward<Args>(args)...);
    }
};

} // namespace compiler

#endif // PARSER_H
