#ifndef AST_NODES_H
#define AST_NODES_H

#include "../diagnostics/source_location.h"
#include <vector>
#include <string>
#include <memory>
#include <variant>

namespace compiler {

// Forward declarations
class ASTVisitor;
class Type;

// Base AST node
class ASTNode {
public:
    SourceLocation location;
    
    explicit ASTNode(SourceLocation loc) : location(loc) {}
    virtual ~ASTNode() = default;
    
    virtual void accept(ASTVisitor& visitor) = 0;
    virtual std::string toString() const = 0;
};

// Expression nodes
class Expr : public ASTNode {
public:
    using ASTNode::ASTNode;
    virtual ~Expr() = default;
};

class BinaryExpr : public Expr {
public:
    enum class Op {
        Add, Sub, Mul, Div, Mod,
        Eq, Ne, Lt, Le, Gt, Ge,
        And, Or,
        BitAnd, BitOr, BitXor, Shl, Shr
    };
    
    Op op;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    
    BinaryExpr(SourceLocation loc, Op o, std::unique_ptr<Expr> l, std::unique_ptr<Expr> r)
        : Expr(loc), op(o), left(std::move(l)), right(std::move(r)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class UnaryExpr : public Expr {
public:
    enum class Op {
        Neg, Not, BitNot, Deref, Ref, Inc, Dec
    };
    
    Op op;
    std::unique_ptr<Expr> operand;
    
    UnaryExpr(SourceLocation loc, Op o, std::unique_ptr<Expr> expr)
        : Expr(loc), op(o), operand(std::move(expr)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class LiteralExpr : public Expr {
public:
    enum class ValueType { Int, Float, Bool, String, Null };
    
    ValueType valueType;
    std::variant<int64_t, double, bool, std::string> value;
    
    LiteralExpr(SourceLocation loc, int64_t v)
        : Expr(loc), valueType(ValueType::Int), value(v) {}
    LiteralExpr(SourceLocation loc, double v)
        : Expr(loc), valueType(ValueType::Float), value(v) {}
    LiteralExpr(SourceLocation loc, bool v)
        : Expr(loc), valueType(ValueType::Bool), value(v) {}
    LiteralExpr(SourceLocation loc, std::string v)
        : Expr(loc), valueType(ValueType::String), value(std::move(v)) {}
    LiteralExpr(SourceLocation loc)
        : Expr(loc), valueType(ValueType::Null), value(0) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class IdentifierExpr : public Expr {
public:
    std::string name;
    
    IdentifierExpr(SourceLocation loc, std::string n)
        : Expr(loc), name(std::move(n)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class CallExpr : public Expr {
public:
    std::unique_ptr<Expr> callee;
    std::vector<std::unique_ptr<Expr>> arguments;
    
    CallExpr(SourceLocation loc, std::unique_ptr<Expr> c, std::vector<std::unique_ptr<Expr>> args)
        : Expr(loc), callee(std::move(c)), arguments(std::move(args)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class IndexExpr : public Expr {
public:
    std::unique_ptr<Expr> base;
    std::unique_ptr<Expr> index;
    
    IndexExpr(SourceLocation loc, std::unique_ptr<Expr> b, std::unique_ptr<Expr> i)
        : Expr(loc), base(std::move(b)), index(std::move(i)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class MemberExpr : public Expr {
public:
    std::unique_ptr<Expr> object;
    std::string member;
    bool isArrow; // -> or .
    
    MemberExpr(SourceLocation loc, std::unique_ptr<Expr> obj, std::string m, bool arrow)
        : Expr(loc), object(std::move(obj)), member(std::move(m)), isArrow(arrow) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class AssignExpr : public Expr {
public:
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> value;
    
    AssignExpr(SourceLocation loc, std::unique_ptr<Expr> t, std::unique_ptr<Expr> v)
        : Expr(loc), target(std::move(t)), value(std::move(v)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class ArrayExpr : public Expr {
public:
    std::vector<std::unique_ptr<Expr>> elements;
    
    explicit ArrayExpr(SourceLocation loc, std::vector<std::unique_ptr<Expr>> elems = {})
        : Expr(loc), elements(std::move(elems)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

// Statement nodes
class Stmt : public ASTNode {
public:
    using ASTNode::ASTNode;
    virtual ~Stmt() = default;
};

class ExprStmt : public Stmt {
public:
    std::unique_ptr<Expr> expression;
    
    explicit ExprStmt(SourceLocation loc, std::unique_ptr<Expr> expr)
        : Stmt(loc), expression(std::move(expr)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class VarDeclStmt : public Stmt {
public:
    std::string name;
    std::unique_ptr<Type> type; // optional (null for inferred)
    std::unique_ptr<Expr> initializer;
    bool isMutable;
    bool isConstant;
    
    VarDeclStmt(SourceLocation loc, std::string n, std::unique_ptr<Type> t,
                std::unique_ptr<Expr> init, bool mut, bool constant)
        : Stmt(loc), name(std::move(n)), type(std::move(t)), 
          initializer(std::move(init)), isMutable(mut), isConstant(constant) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class BlockStmt : public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> statements;
    
    explicit BlockStmt(SourceLocation loc, std::vector<std::unique_ptr<Stmt>> stmts = {})
        : Stmt(loc), statements(std::move(stmts)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class IfStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;
    
    IfStmt(SourceLocation loc, std::unique_ptr<Expr> cond, 
           std::unique_ptr<Stmt> thenB, std::unique_ptr<Stmt> elseB)
        : Stmt(loc), condition(std::move(cond)), thenBranch(std::move(thenB)), 
          elseBranch(std::move(elseB)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class WhileStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;
    
    WhileStmt(SourceLocation loc, std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> b)
        : Stmt(loc), condition(std::move(cond)), body(std::move(b)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class ForStmt : public Stmt {
public:
    std::unique_ptr<Stmt> initializer;
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> increment;
    std::unique_ptr<Stmt> body;
    
    ForStmt(SourceLocation loc, std::unique_ptr<Stmt> init,
            std::unique_ptr<Expr> cond, std::unique_ptr<Expr> inc,
            std::unique_ptr<Stmt> b)
        : Stmt(loc), initializer(std::move(init)), condition(std::move(cond)),
          increment(std::move(inc)), body(std::move(b)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class ReturnStmt : public Stmt {
public:
    std::unique_ptr<Expr> value; // null for void return
    
    explicit ReturnStmt(SourceLocation loc, std::unique_ptr<Expr> val = nullptr)
        : Stmt(loc), value(std::move(val)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class BreakStmt : public Stmt {
public:
    explicit BreakStmt(SourceLocation loc) : Stmt(loc) {}
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class ContinueStmt : public Stmt {
public:
    explicit ContinueStmt(SourceLocation loc) : Stmt(loc) {}
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

// Declaration nodes
class Decl : public ASTNode {
public:
    std::string name;
    
    Decl(SourceLocation loc, std::string n) : ASTNode(loc), name(std::move(n)) {}
    virtual ~Decl() = default;
};

class ParamDecl {
public:
    std::string name;
    std::unique_ptr<Type> type;
    bool isMutable;
    
    ParamDecl(std::string n, std::unique_ptr<Type> t, bool mut = false)
        : name(std::move(n)), type(std::move(t)), isMutable(mut) {}
};

class FunctionDecl : public Decl {
public:
    std::vector<ParamDecl> parameters;
    std::unique_ptr<Type> returnType;
    std::unique_ptr<Stmt> body; // null for extern functions
    
    FunctionDecl(SourceLocation loc, std::string n, std::vector<ParamDecl> params,
                 std::unique_ptr<Type> ret, std::unique_ptr<Stmt> b)
        : Decl(loc, std::move(n)), parameters(std::move(params)), 
          returnType(std::move(ret)), body(std::move(b)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class StructDecl : public Decl {
public:
    struct Field {
        std::string name;
        std::unique_ptr<Type> type;
    };
    
    std::vector<Field> fields;
    
    StructDecl(SourceLocation loc, std::string n, std::vector<Field> f)
        : Decl(loc, std::move(n)), fields(std::move(f)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class EnumDecl : public Decl {
public:
    struct Variant {
        std::string name;
        std::vector<std::unique_ptr<Type>> types;
    };
    
    std::vector<Variant> variants;
    
    EnumDecl(SourceLocation loc, std::string n, std::vector<Variant> v)
        : Decl(loc, std::move(n)), variants(std::move(v)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

// Type nodes
class Type : public ASTNode {
public:
    enum class Kind {
        Primitive,      // i32, f64, bool, etc.
        Array,          // [T; n]
        Pointer,        // &T, *T
        Reference,      // ref T
        Function,       // fn(T) -> R
        Struct,         // struct Name
        Enum,           // enum Name
        Generic,        // T (type parameter)
        Infer           // _ (inferred)
    };
    
    Kind kind;
    
    explicit Type(SourceLocation loc, Kind k) : ASTNode(loc), kind(k) {}
    virtual ~Type() = default;
};

class PrimitiveType : public Type {
public:
    enum class TypeCode {
        Void, Bool,
        I8, I16, I32, I64,
        U8, U16, U32, U64,
        F32, F64,
        Char, String
    };
    
    TypeCode typeCode;
    
    PrimitiveType(SourceLocation loc, TypeCode tc)
        : Type(loc, Kind::Primitive), typeCode(tc) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
    
    static std::string getTypeName(TypeCode tc);
};

class ArrayType : public Type {
public:
    std::unique_ptr<Type> elementType;
    size_t size; // 0 for dynamic arrays
    
    ArrayType(SourceLocation loc, std::unique_ptr<Type> elem, size_t s = 0)
        : Type(loc, Kind::Array), elementType(std::move(elem)), size(s) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class PointerType : public Type {
public:
    std::unique_ptr<Type> pointeeType;
    bool isMutable;
    
    PointerType(SourceLocation loc, std::unique_ptr<Type> p, bool mut)
        : Type(loc, Kind::Pointer), pointeeType(std::move(p)), isMutable(mut) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class FunctionType : public Type {
public:
    std::vector<std::unique_ptr<Type>> paramTypes;
    std::unique_ptr<Type> returnType;
    
    FunctionType(SourceLocation loc, std::vector<std::unique_ptr<Type>> params,
                 std::unique_ptr<Type> ret)
        : Type(loc, Kind::Function), paramTypes(std::move(params)), returnType(std::move(ret)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class NamedType : public Type {
public:
    std::string name;
    std::vector<std::unique_ptr<Type>> typeArgs;
    
    NamedType(SourceLocation loc, std::string n, std::vector<std::unique_ptr<Type>> args = {})
        : Type(loc, Kind::Struct), name(std::move(n)), typeArgs(std::move(args)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

// Program (root node)
class Program : public ASTNode {
public:
    std::vector<std::unique_ptr<Decl>> declarations;
    
    explicit Program(SourceLocation loc, std::vector<std::unique_ptr<Decl>> decls = {})
        : ASTNode(loc), declarations(std::move(decls)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

} // namespace compiler

#endif // AST_NODES_H
