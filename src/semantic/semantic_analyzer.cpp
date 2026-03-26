#include "semantic_analyzer.h"
#include "../diagnostics/error_reporter.h"

namespace compiler {

SemanticAnalyzer::SemanticAnalyzer() 
    : hadErrors_(false), inFunction_(false) {
    symbolTable_.initializeBuiltins();
}

bool SemanticAnalyzer::analyze(Program& program) {
    hadErrors_ = false;
    program.accept(*this);
    return !hadErrors_;
}

void SemanticAnalyzer::visit(Program& node) {
    for (auto& decl : node.declarations) {
        decl->accept(*this);
    }
}

void SemanticAnalyzer::visit(FunctionDecl& node) {
    bool wasInFunction = inFunction_;
    inFunction_ = true;
    
    // Resolve return type
    std::shared_ptr<SemanticType> retType;
    if (node.returnType) {
        retType = resolveASTType(*node.returnType);
    } else {
        retType = SemanticType::getPrimitive(SemanticType::Kind::Void);
    }
    
    currentReturnType_ = retType;
    
    // Create function type
    std::vector<std::shared_ptr<SemanticType>> paramTypes;
    for (auto& param : node.parameters) {
        paramTypes.push_back(resolveASTType(*param.type));
    }
    
    auto funcType = std::make_shared<FunctionType>(paramTypes, retType);
    
    // Add to symbol table before analyzing body (for recursion)
    auto symbol = std::make_shared<Symbol>(node.name, SymbolKind::Function, funcType, node.location);
    symbolTable_.define(symbol);
    
    // Analyze body
    if (node.body) {
        symbolTable_.enterScope();
        
        // Add parameters to scope
        for (size_t i = 0; i < node.parameters.size(); ++i) {
            auto paramSymbol = std::make_shared<Symbol>(
                node.parameters[i].name,
                SymbolKind::Variable,
                paramTypes[i],
                node.location,
                node.parameters[i].isMutable
            );
            symbolTable_.define(paramSymbol);
        }
        
        node.body->accept(*this);
        symbolTable_.exitScope();
    }
    
    inFunction_ = wasInFunction;
    currentReturnType_ = nullptr;
}

void SemanticAnalyzer::visit(StructDecl& node) {
    std::vector<StructType::Field> fields;
    for (auto& field : node.fields) {
        auto fieldType = resolveASTType(*field.type);
        fields.push_back({field.name, fieldType, 0}); // Offset computed later
    }
    
    auto structType = std::make_shared<StructType>(node.name, std::move(fields));
    auto symbol = std::make_shared<Symbol>(node.name, SymbolKind::Struct, structType, node.location);
    symbolTable_.define(symbol);
}

void SemanticAnalyzer::visit(EnumDecl& node) {
    std::vector<EnumType::Variant> variants;
    for (auto& variant : node.variants) {
        std::vector<std::shared_ptr<SemanticType>> types;
        for (auto& type : variant.types) {
            types.push_back(resolveASTType(*type));
        }
        variants.push_back({variant.name, std::move(types)});
    }
    
    auto enumType = std::make_shared<EnumType>(node.name, std::move(variants));
    auto symbol = std::make_shared<Symbol>(node.name, SymbolKind::Enum, enumType, node.location);
    symbolTable_.define(symbol);
}

void SemanticAnalyzer::visit(BlockStmt& node) {
    symbolTable_.enterScope();
    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }
    symbolTable_.exitScope();
}

void SemanticAnalyzer::visit(VarDeclStmt& node) {
    std::shared_ptr<SemanticType> varType = nullptr;
    
    if (node.type) {
        varType = resolveASTType(*node.type);
    }
    
    if (node.initializer) {
        auto initType = checkExpr(*node.initializer);
        if (!varType) {
            varType = initType;
        } else if (!TypeConversions::isImplicitlyConvertible(initType, varType)) {
            reportTypeError(node.location, "Cannot initialize variable of type '" + 
                          varType->toString() + "' with value of type '" + initType->toString() + "'");
        }
    } else if (!varType) {
        reportTypeError(node.location, "Variable declaration must have type annotation or initializer");
        varType = SemanticType::getPrimitive(SemanticType::Kind::Error);
    }
    
    auto symbol = std::make_shared<Symbol>(
        node.name,
        node.isConstant ? SymbolKind::Constant : SymbolKind::Variable,
        varType,
        node.location,
        true
    );
    symbol->type->isMutable = node.isMutable;
    
    if (!symbolTable_.define(symbol)) {
        reportTypeError(node.location, "Variable '" + node.name + "' already declared in this scope");
    }
}

void SemanticAnalyzer::visit(IfStmt& node) {
    auto condType = checkExpr(*node.condition);
    if (condType->kind != SemanticType::Kind::Bool) {
        reportTypeError(node.condition->location, "If condition must be boolean, got '" + 
                       condType->toString() + "'");
    }
    
    node.thenBranch->accept(*this);
    if (node.elseBranch) {
        node.elseBranch->accept(*this);
    }
}

void SemanticAnalyzer::visit(WhileStmt& node) {
    auto condType = checkExpr(*node.condition);
    if (condType->kind != SemanticType::Kind::Bool) {
        reportTypeError(node.condition->location, "While condition must be boolean, got '" + 
                       condType->toString() + "'");
    }
    
    pushLoop(LoopType::While);
    node.body->accept(*this);
    popLoop();
}

void SemanticAnalyzer::visit(ForStmt& node) {
    symbolTable_.enterScope();
    
    if (node.initializer) {
        node.initializer->accept(*this);
    }
    
    if (node.condition) {
        auto condType = checkExpr(*node.condition);
        if (condType->kind != SemanticType::Kind::Bool) {
            reportTypeError(node.condition->location, "For condition must be boolean");
        }
    }
    
    if (node.increment) {
        checkExpr(*node.increment);
    }
    
    pushLoop(LoopType::For);
    node.body->accept(*this);
    popLoop();
    
    symbolTable_.exitScope();
}

void SemanticAnalyzer::visit(ReturnStmt& node) {
    if (!inFunction_) {
        reportTypeError(node.location, "Return statement outside of function");
        return;
    }
    
    if (node.value) {
        auto valueType = checkExpr(*node.value);
        if (!TypeConversions::isImplicitlyConvertible(valueType, currentReturnType_)) {
            reportTypeError(node.location, "Cannot return value of type '" + valueType->toString() + 
                          "' from function returning '" + currentReturnType_->toString() + "'");
        }
    } else if (currentReturnType_->kind != SemanticType::Kind::Void) {
        reportTypeError(node.location, "Expected return value of type '" + 
                       currentReturnType_->toString() + "'");
    }
}

void SemanticAnalyzer::visit(BreakStmt& node) {
    if (!inLoop()) {
        reportTypeError(node.location, "Break statement outside of loop");
    }
}

void SemanticAnalyzer::visit(ContinueStmt& node) {
    if (!inLoop()) {
        reportTypeError(node.location, "Continue statement outside of loop");
    }
}

void SemanticAnalyzer::visit(ExprStmt& node) {
    checkExpr(*node.expression);
}

void SemanticAnalyzer::visit(BinaryExpr& node) {
    auto resultType = checkBinaryOp(node);
    exprTypes_[&node] = resultType;
}

std::shared_ptr<SemanticType> SemanticAnalyzer::checkBinaryOp(BinaryExpr& node) {
    auto leftType = checkExpr(*node.left);
    auto rightType = checkExpr(*node.right);
    
    switch (node.op) {
        case BinaryExpr::Op::Add:
        case BinaryExpr::Op::Sub:
        case BinaryExpr::Op::Mul:
        case BinaryExpr::Op::Div:
        case BinaryExpr::Op::Mod:
            if (!leftType->isNumeric() || !rightType->isNumeric()) {
                reportTypeError(node.location, "Arithmetic operators require numeric operands");
                return SemanticType::getPrimitive(SemanticType::Kind::Error);
            }
            return TypeConversions::getCommonType(leftType, rightType);
            
        case BinaryExpr::Op::Eq:
        case BinaryExpr::Op::Ne:
            if (!TypeConversions::isImplicitlyConvertible(leftType, rightType) &&
                !TypeConversions::isImplicitlyConvertible(rightType, leftType)) {
                reportTypeError(node.location, "Cannot compare incompatible types");
            }
            return SemanticType::getPrimitive(SemanticType::Kind::Bool);
            
        case BinaryExpr::Op::Lt:
        case BinaryExpr::Op::Le:
        case BinaryExpr::Op::Gt:
        case BinaryExpr::Op::Ge:
            if (!leftType->isNumeric() || !rightType->isNumeric()) {
                reportTypeError(node.location, "Comparison operators require numeric operands");
            }
            return SemanticType::getPrimitive(SemanticType::Kind::Bool);
            
        case BinaryExpr::Op::And:
        case BinaryExpr::Op::Or:
            if (leftType->kind != SemanticType::Kind::Bool || 
                rightType->kind != SemanticType::Kind::Bool) {
                reportTypeError(node.location, "Logical operators require boolean operands");
            }
            return SemanticType::getPrimitive(SemanticType::Kind::Bool);
            
        case BinaryExpr::Op::BitAnd:
        case BinaryExpr::Op::BitOr:
        case BinaryExpr::Op::BitXor:
        case BinaryExpr::Op::Shl:
        case BinaryExpr::Op::Shr:
            if (!leftType->isInteger() || !rightType->isInteger()) {
                reportTypeError(node.location, "Bitwise operators require integer operands");
            }
            return leftType;
    }
    
    return SemanticType::getPrimitive(SemanticType::Kind::Error);
}

void SemanticAnalyzer::visit(UnaryExpr& node) {
    auto resultType = checkUnaryOp(node);
    exprTypes_[&node] = resultType;
}

std::shared_ptr<SemanticType> SemanticAnalyzer::checkUnaryOp(UnaryExpr& node) {
    auto operandType = checkExpr(*node.operand);
    
    switch (node.op) {
        case UnaryExpr::Op::Neg:
            if (!operandType->isNumeric()) {
                reportTypeError(node.location, "Unary minus requires numeric operand");
            }
            return operandType;
            
        case UnaryExpr::Op::Not:
            if (operandType->kind != SemanticType::Kind::Bool) {
                reportTypeError(node.location, "Logical not requires boolean operand");
            }
            return operandType;
            
        case UnaryExpr::Op::BitNot:
            if (!operandType->isInteger()) {
                reportTypeError(node.location, "Bitwise not requires integer operand");
            }
            return operandType;
            
        case UnaryExpr::Op::Deref:
            if (operandType->kind != SemanticType::Kind::Pointer) {
                reportTypeError(node.location, "Cannot dereference non-pointer type");
                return SemanticType::getPrimitive(SemanticType::Kind::Error);
            }
            return std::static_pointer_cast<PointerType>(operandType)->pointee;
            
        case UnaryExpr::Op::Ref:
            return std::make_shared<PointerType>(operandType, false);
            
        case UnaryExpr::Op::Inc:
        case UnaryExpr::Op::Dec:
            if (!operandType->isNumeric()) {
                reportTypeError(node.location, "Increment/decrement requires numeric operand");
            }
            if (!operandType->isMutable) {
                reportTypeError(node.location, "Cannot modify immutable value");
            }
            return operandType;
    }
    
    return SemanticType::getPrimitive(SemanticType::Kind::Error);
}

void SemanticAnalyzer::visit(LiteralExpr& node) {
    std::shared_ptr<SemanticType> type;
    switch (node.valueType) {
        case LiteralExpr::ValueType::Int:
            type = SemanticType::getPrimitive(SemanticType::Kind::Int32);
            break;
        case LiteralExpr::ValueType::Float:
            type = SemanticType::getPrimitive(SemanticType::Kind::Float64);
            break;
        case LiteralExpr::ValueType::Bool:
            type = SemanticType::getPrimitive(SemanticType::Kind::Bool);
            break;
        case LiteralExpr::ValueType::String:
            type = SemanticType::getPrimitive(SemanticType::Kind::String);
            break;
        case LiteralExpr::ValueType::Null:
            type = std::make_shared<PointerType>(
                SemanticType::getPrimitive(SemanticType::Kind::Void));
            break;
    }
    exprTypes_[&node] = type;
}

void SemanticAnalyzer::visit(IdentifierExpr& node) {
    auto symbol = symbolTable_.resolve(node.name);
    if (!symbol) {
        reportTypeError(node.location, "Undefined identifier: '" + node.name + "'");
        exprTypes_[&node] = SemanticType::getPrimitive(SemanticType::Kind::Error);
        return;
    }
    exprTypes_[&node] = symbol->type;
}

void SemanticAnalyzer::visit(CallExpr& node) {
    auto type = checkCall(node);
    exprTypes_[&node] = type;
}

std::shared_ptr<SemanticType> SemanticAnalyzer::checkCall(CallExpr& node) {
    auto calleeType = checkExpr(*node.callee);
    
    if (calleeType->kind != SemanticType::Kind::Function) {
        reportTypeError(node.location, "Cannot call non-function type");
        return SemanticType::getPrimitive(SemanticType::Kind::Error);
    }
    
    auto funcType = std::static_pointer_cast<FunctionType>(calleeType);
    
    if (node.arguments.size() != funcType->params.size()) {
        reportTypeError(node.location, "Argument count mismatch: expected " + 
                       std::to_string(funcType->params.size()) + ", got " + 
                       std::to_string(node.arguments.size()));
    } else {
        for (size_t i = 0; i < node.arguments.size(); ++i) {
            auto argType = checkExpr(*node.arguments[i]);
            if (!TypeConversions::isImplicitlyConvertible(argType, funcType->params[i])) {
                reportTypeError(node.arguments[i]->location, "Argument type mismatch at position " + 
                               std::to_string(i));
            }
        }
    }
    
    return funcType->returnType;
}

void SemanticAnalyzer::visit(IndexExpr& node) {
    auto baseType = checkExpr(*node.base);
    auto indexType = checkExpr(*node.index);
    
    if (indexType->kind != SemanticType::Kind::Int32 && 
        indexType->kind != SemanticType::Kind::Int64) {
        reportTypeError(node.index->location, "Array index must be integer");
    }
    
    if (baseType->kind == SemanticType::Kind::Array) {
        exprTypes_[&node] = std::static_pointer_cast<ArrayType>(baseType)->elementType;
    } else if (baseType->kind == SemanticType::Kind::Pointer) {
        exprTypes_[&node] = std::static_pointer_cast<PointerType>(baseType)->pointee;
    } else {
        reportTypeError(node.location, "Cannot index non-array type");
        exprTypes_[&node] = SemanticType::getPrimitive(SemanticType::Kind::Error);
    }
}

void SemanticAnalyzer::visit(MemberExpr& node) {
    auto objType = checkExpr(*node.object);
    std::shared_ptr<SemanticType> resultType;
    
    if (objType->kind == SemanticType::Kind::Struct) {
        auto structType = std::static_pointer_cast<StructType>(objType);
        auto field = structType->getField(node.member);
        if (!field) {
            reportTypeError(node.location, "Struct '" + structType->name + 
                          "' has no member '" + node.member + "'");
            resultType = SemanticType::getPrimitive(SemanticType::Kind::Error);
        } else {
            resultType = field->type;
        }
    } else if (objType->kind == SemanticType::Kind::Pointer) {
        // Auto-dereference for ->
        auto pointee = std::static_pointer_cast<PointerType>(objType)->pointee;
        if (pointee->kind == SemanticType::Kind::Struct) {
            auto structType = std::static_pointer_cast<StructType>(pointee);
            auto field = structType->getField(node.member);
            if (!field) {
                reportTypeError(node.location, "Struct '" + structType->name + 
                              "' has no member '" + node.member + "'");
                resultType = SemanticType::getPrimitive(SemanticType::Kind::Error);
            } else {
                resultType = field->type;
            }
        } else {
            reportTypeError(node.location, "Member access on non-struct pointer");
            resultType = SemanticType::getPrimitive(SemanticType::Kind::Error);
        }
    } else {
        reportTypeError(node.location, "Member access on non-struct type");
        resultType = SemanticType::getPrimitive(SemanticType::Kind::Error);
    }
    
    exprTypes_[&node] = resultType;
}

void SemanticAnalyzer::visit(AssignExpr& node) {
    auto targetType = checkExpr(*node.target);
    auto valueType = checkExpr(*node.value);
    
    if (!targetType->isMutable) {
        reportTypeError(node.location, "Cannot assign to immutable value");
    }
    
    if (!TypeConversions::isImplicitlyConvertible(valueType, targetType)) {
        reportTypeError(node.location, "Cannot assign value of type '" + valueType->toString() + 
                       "' to target of type '" + targetType->toString() + "'");
    }
    
    exprTypes_[&node] = targetType;
}

void SemanticAnalyzer::visit(ArrayExpr& node) {
    std::shared_ptr<SemanticType> elemType = nullptr;
    
    for (auto& elem : node.elements) {
        auto type = checkExpr(*elem);
        if (!elemType) {
            elemType = type;
        } else if (!checkTypesEqual(elemType, type)) {
            reportTypeError(elem->location, "Array elements must have consistent type");
        }
    }
    
    if (!elemType) {
        elemType = SemanticType::getPrimitive(SemanticType::Kind::Void);
    }
    
    exprTypes_[&node] = std::make_shared<ArrayType>(elemType, node.elements.size());
}

void SemanticAnalyzer::visit(PrimitiveType& node) {
    // Type resolution handled in resolveASTType
}

void SemanticAnalyzer::visit(ArrayType& node) {
    // Type resolution handled in resolveASTType
}

void SemanticAnalyzer::visit(PointerType& node) {
    // Type resolution handled in resolveASTType
}

void SemanticAnalyzer::visit(FunctionType& node) {
    // Type resolution handled in resolveASTType
}

void SemanticAnalyzer::visit(NamedType& node) {
    // Type resolution handled in resolveASTType
}

std::shared_ptr<SemanticType> SemanticAnalyzer::resolveASTType(compiler::Type& astType) {
    switch (astType.kind) {
        case compiler::Type::Kind::Primitive: {
            auto& prim = static_cast<PrimitiveType&>(astType);
            switch (prim.typeCode) {
                case PrimitiveType::TypeCode::Void: return SemanticType::getPrimitive(SemanticType::Kind::Void);
                case PrimitiveType::TypeCode::Bool: return SemanticType::getPrimitive(SemanticType::Kind::Bool);
                case PrimitiveType::TypeCode::I8: return SemanticType::getPrimitive(SemanticType::Kind::Int8);
                case PrimitiveType::TypeCode::I16: return SemanticType::getPrimitive(SemanticType::Kind::Int16);
                case PrimitiveType::TypeCode::I32: return SemanticType::getPrimitive(SemanticType::Kind::Int32);
                case PrimitiveType::TypeCode::I64: return SemanticType::getPrimitive(SemanticType::Kind::Int64);
                case PrimitiveType::TypeCode::U8: return SemanticType::getPrimitive(SemanticType::Kind::UInt8);
                case PrimitiveType::TypeCode::U16: return SemanticType::getPrimitive(SemanticType::Kind::UInt16);
                case PrimitiveType::TypeCode::U32: return SemanticType::getPrimitive(SemanticType::Kind::UInt32);
                case PrimitiveType::TypeCode::U64: return SemanticType::getPrimitive(SemanticType::Kind::UInt64);
                case PrimitiveType::TypeCode::F32: return SemanticType::getPrimitive(SemanticType::Kind::Float32);
                case PrimitiveType::TypeCode::F64: return SemanticType::getPrimitive(SemanticType::Kind::Float64);
                case PrimitiveType::TypeCode::Char: return SemanticType::getPrimitive(SemanticType::Kind::Char);
                case PrimitiveType::TypeCode::String: return SemanticType::getPrimitive(SemanticType::Kind::String);
            }
            break;
        }
        case compiler::Type::Kind::Array: {
            auto& arr = static_cast<ArrayType&>(astType);
            auto elemType = resolveASTType(*arr.elementType);
            return std::make_shared<ArrayType>(elemType, arr.size);
        }
        case compiler::Type::Kind::Pointer: {
            auto& ptr = static_cast<PointerType&>(astType);
            auto pointee = resolveASTType(*ptr.pointeeType);
            return std::make_shared<PointerType>(pointee, ptr.isMutable);
        }
        case compiler::Type::Kind::Function: {
            auto& func = static_cast<FunctionType&>(astType);
            std::vector<std::shared_ptr<SemanticType>> params;
            for (auto& p : func.paramTypes) {
                params.push_back(resolveASTType(*p));
            }
            auto ret = resolveASTType(*func.returnType);
            return std::make_shared<FunctionType>(params, ret);
        }
        case compiler::Type::Kind::Struct: {
            auto& named = static_cast<NamedType&>(astType);
            auto symbol = symbolTable_.resolveType(named.name);
            if (symbol && symbol->type) {
                return symbol->type;
            }
            reportTypeError(astType.location, "Unknown type: '" + named.name + "'");
            return SemanticType::getPrimitive(SemanticType::Kind::Error);
        }
        default:
            break;
    }
    return SemanticType::getPrimitive(SemanticType::Kind::Error);
}

std::shared_ptr<SemanticType> SemanticAnalyzer::checkExpr(Expr& expr) {
    expr.accept(*this);
    auto it = exprTypes_.find(&expr);
    if (it != exprTypes_.end()) {
        return it->second;
    }
    return SemanticType::getPrimitive(SemanticType::Kind::Error);
}

bool SemanticAnalyzer::checkTypesEqual(std::shared_ptr<SemanticType> a, 
                                       std::shared_ptr<SemanticType> b) {
    return a->equals(*b);
}

void SemanticAnalyzer::reportTypeError(SourceLocation loc, const std::string& msg) {
    REPORT_ERROR(loc, msg);
    hadErrors_ = true;
}

void SemanticAnalyzer::pushLoop(LoopType type) {
    loopStack_.push({type, false, false});
}

void SemanticAnalyzer::popLoop() {
    if (!loopStack_.empty()) {
        loopStack_.pop();
    }
}

bool SemanticAnalyzer::inLoop() const {
    return !loopStack_.empty();
}

// SymbolTable implementation
SymbolTable::SymbolTable() {
    globalScope_ = std::make_shared<Scope>();
    currentScope_ = globalScope_;
    allScopes_.push_back(globalScope_);
}

void SymbolTable::enterScope() {
    auto newScope = std::make_shared<Scope>(currentScope_);
    allScopes_.push_back(newScope);
    currentScope_ = newScope;
}

void SymbolTable::exitScope() {
    if (currentScope_->getParent()) {
        currentScope_ = currentScope_->getParent();
    }
}

bool SymbolTable::define(std::shared_ptr<Symbol> symbol) {
    return currentScope_->define(symbol);
}

std::shared_ptr<Symbol> SymbolTable::resolve(const std::string& name) {
    return currentScope_->resolve(name);
}

std::shared_ptr<Symbol> SymbolTable::resolveType(const std::string& name) {
    auto symbol = resolve(name);
    if (symbol && (symbol->kind == SymbolKind::Type || 
                   symbol->kind == SymbolKind::Struct ||
                   symbol->kind == SymbolKind::Enum)) {
        return symbol;
    }
    return nullptr;
}

void SymbolTable::initializeBuiltins() {
    // Add primitive types
    auto addPrimitive = [&](const std::string& name, SemanticType::Kind kind) {
        auto type = SemanticType::getPrimitive(kind);
        auto symbol = std::make_shared<Symbol>(name, SymbolKind::Type, type, SourceLocation());
        define(symbol);
    };
    
    addPrimitive("void", SemanticType::Kind::Void);
    addPrimitive("bool", SemanticType::Kind::Bool);
    addPrimitive("i8", SemanticType::Kind::Int8);
    addPrimitive("i16", SemanticType::Kind::Int16);
    addPrimitive("i32", SemanticType::Kind::Int32);
    addPrimitive("i64", SemanticType::Kind::Int64);
    addPrimitive("u8", SemanticType::Kind::UInt8);
    addPrimitive("u16", SemanticType::Kind::UInt16);
    addPrimitive("u32", SemanticType::Kind::UInt32);
    addPrimitive("u64", SemanticType::Kind::UInt64);
    addPrimitive("f32", SemanticType::Kind::Float32);
    addPrimitive("f64", SemanticType::Kind::Float64);
    addPrimitive("char", SemanticType::Kind::Char);
    addPrimitive("string", SemanticType::Kind::String);
}

bool Scope::define(std::shared_ptr<Symbol> symbol) {
    if (symbols_.find(symbol->name) != symbols_.end()) {
        return false;
    }
    symbols_[symbol->name] = symbol;
    return true;
}

std::shared_ptr<Symbol> Scope::resolve(const std::string& name) {
    auto it = symbols_.find(name);
    if (it != symbols_.end()) {
        return it->second;
    }
    if (parent_) {
        return parent_->resolve(name);
    }
    return nullptr;
}

std::shared_ptr<Symbol> Scope::resolveLocal(const std::string& name) {
    auto it = symbols_.find(name);
    if (it != symbols_.end()) {
        return it->second;
    }
    return nullptr;
}

// TypeConversions implementation
bool TypeConversions::isImplicitlyConvertible(std::shared_ptr<SemanticType> from,
                                              std::shared_ptr<SemanticType> to) {
    if (from->equals(*to)) return true;
    
    // Numeric promotions
    if (from->isNumeric() && to->isNumeric()) {
        // Allow widening conversions
        if (from->isInteger() && to->isInteger()) {
            // Signed to signed, unsigned to unsigned
            if (from->isSigned() == to->isSigned()) {
                return from->getSize() <= to->getSize();
            }
        }
        // Integer to float always allowed
        if (from->isInteger() && to->isFloat()) return true;
    }
    
    // Pointer conversions
    if (from->kind == SemanticType::Kind::Pointer && 
        to->kind == SemanticType::Kind::Pointer) {
        auto fromPtr = std::static_pointer_cast<PointerType>(from);
        auto toPtr = std::static_pointer_cast<PointerType>(to);
        // Allow pointer to void*
        if (toPtr->pointee->kind == SemanticType::Kind::Void) return true;
        // Allow if pointee types are the same (ignoring mutability for now)
        return fromPtr->pointee->equals(*toPtr->pointee);
    }
    
    return false;
}

bool TypeConversions::isExplicitlyConvertible(std::shared_ptr<SemanticType> from,
                                              std::shared_ptr<SemanticType> to) {
    if (isImplicitlyConvertible(from, to)) return true;
    
    // Allow explicit numeric narrowing
    if (from->isNumeric() && to->isNumeric()) return true;
    
    // Allow pointer to integer and back
    if ((from->isPointer() || from->isInteger()) && 
        (to->isPointer() || to->isInteger())) return true;
    
    return false;
}

std::shared_ptr<SemanticType> TypeConversions::getCommonType(std::shared_ptr<SemanticType> a,
                                                             std::shared_ptr<SemanticType> b) {
    if (a->equals(*b)) return a;
    
    if (a->isNumeric() && b->isNumeric()) {
        // Return the wider type
        if (a->getSize() > b->getSize()) return a;
        if (b->getSize() > a->getSize()) return b;
        
        // Same size, prefer signed over unsigned, float over int
        if (a->isFloat() && !b->isFloat()) return a;
        if (b->isFloat() && !a->isFloat()) return b;
        
        return a; // Default to first
    }
    
    return a; // Fallback
}

std::shared_ptr<SemanticType> TypeConversions::promoteInteger(std::shared_ptr<SemanticType> type) {
    if (type->kind == SemanticType::Kind::Int8 || 
        type->kind == SemanticType::Kind::Int16) {
        return SemanticType::getPrimitive(SemanticType::Kind::Int32);
    }
    if (type->kind == SemanticType::Kind::UInt8 || 
        type->kind == SemanticType::Kind::UInt16) {
        return SemanticType::getPrimitive(SemanticType::Kind::UInt32);
    }
    return type;
}

// SemanticType implementations
std::string SemanticType::toString() const {
    switch (kind) {
        case Kind::Void: return "void";
        case Kind::Bool: return "bool";
        case Kind::Int8: return "i8";
        case Kind::Int16: return "i16";
        case Kind::Int32: return "i32";
        case Kind::Int64: return "i64";
        case Kind::UInt8: return "u8";
        case Kind::UInt16: return "u16";
        case Kind::UInt32: return "u32";
        case Kind::UInt64: return "u64";
        case Kind::Float32: return "f32";
        case Kind::Float64: return "f64";
        case Kind::Char: return "char";
        case Kind::String: return "string";
        case Kind::Error: return "<error>";
        default: return "<complex>";
    }
}

bool SemanticType::equals(const SemanticType& other) const {
    return kind == other.kind && isMutable == other.isMutable;
}

size_t SemanticType::getSize() const {
    switch (kind) {
        case Kind::Void: return 0;
        case Kind::Bool: return 1;
        case Kind::Int8: case Kind::UInt8: return 1;
        case Kind::Int16: case Kind::UInt16: return 2;
        case Kind::Int32: case Kind::UInt32: case Kind::Float32: return 4;
        case Kind::Int64: case Kind::UInt64: case Kind::Float64: return 8;
        case Kind::Char: return 1;
        case Kind::String: return 8; // Pointer
        case Kind::Pointer: return 8;
        case Kind::Reference: return 8;
        default: return 0;
    }
}

bool SemanticType::isInteger() const {
    return kind >= Kind::Int8 && kind <= Kind::UInt64;
}

bool SemanticType::isSigned() const {
    return kind >= Kind::Int8 && kind <= Kind::Int64;
}

bool SemanticType::isUnsigned() const {
    return kind >= Kind::UInt8 && kind <= Kind::UInt64;
}

bool SemanticType::isFloat() const {
    return kind == Kind::Float32 || kind == Kind::Float64;
}

bool SemanticType::isNumeric() const {
    return isInteger() || isFloat();
}

bool SemanticType::isPointer() const {
    return kind == Kind::Pointer;
}

bool SemanticType::isReference() const {
    return kind == Kind::Reference;
}

bool SemanticType::isArray() const {
    return kind == Kind::Array;
}

bool SemanticType::isFunction() const {
    return kind == Kind::Function;
}

bool SemanticType::isComposite() const {
    return kind == Kind::Struct || kind == Kind::Enum;
}

std::shared_ptr<SemanticType> SemanticType::getPrimitive(Kind k) {
    static std::unordered_map<Kind, std::shared_ptr<SemanticType>> primitives;
    auto it = primitives.find(k);
    if (it != primitives.end()) return it->second;
    
    auto type = std::make_shared<SemanticType>(k);
    primitives[k] = type;
    return type;
}

std::string PointerType::toString() const {
    return (isMutable ? "&mut " : "&") + pointee->toString();
}

bool PointerType::equals(const SemanticType& other) const {
    if (other.kind != Kind::Pointer) return false;
    auto& otherPtr = static_cast<const PointerType&>(other);
    return pointee->equals(*otherPtr.pointee) && isMutable == otherPtr.isMutable;
}

std::string ArrayType::toString() const {
    return "[" + elementType->toString() + "; " + std::to_string(size) + "]";
}

bool ArrayType::equals(const SemanticType& other) const {
    if (other.kind != Kind::Array) return false;
    auto& otherArr = static_cast<const ArrayType&>(other);
    return elementType->equals(*otherArr.elementType) && size == otherArr.size;
}

size_t ArrayType::getSize() const {
    return elementType->getSize() * size;
}

std::string FunctionType::toString() const {
    std::string result = "fn(";
    for (size_t i = 0; i < params.size(); ++i) {
        if (i > 0) result += ", ";
        result += params[i]->toString();
    }
    result += ") -> " + returnType->toString();
    return result;
}

bool FunctionType::equals(const SemanticType& other) const {
    if (other.kind != Kind::Function) return false;
    auto& otherFunc = static_cast<const FunctionType&>(other);
    if (params.size() != otherFunc.params.size()) return false;
    if (!returnType->equals(*otherFunc.returnType)) return false;
    for (size_t i = 0; i < params.size(); ++i) {
        if (!params[i]->equals(*otherFunc.params[i])) return false;
    }
    return true;
}

std::optional<StructType::Field> StructType::getField(const std::string& name) const {
    auto it = fieldMap.find(name);
    if (it != fieldMap.end()) {
        return fields[it->second];
    }
    return std::nullopt;
}

std::string StructType::toString() const {
    return "struct " + name;
}

bool StructType::equals(const SemanticType& other) const {
    if (other.kind != Kind::Struct) return false;
    auto& otherStruct = static_cast<const StructType&>(other);
    return name == otherStruct.name;
}

size_t StructType::getSize() const {
    size_t size = 0;
    for (const auto& field : fields) {
        size += field.type->getSize();
        // Alignment padding would go here
    }
    return size;
}

std::string EnumType::toString() const {
    return "enum " + name;
}

bool EnumType::equals(const SemanticType& other) const {
    if (other.kind != Kind::Enum) return false;
    auto& otherEnum = static_cast<const EnumType&>(other);
    return name == otherEnum.name;
}

} // namespace compiler
