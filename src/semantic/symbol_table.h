#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "type_system.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

namespace compiler {

enum class SymbolKind {
    Variable,
    Constant,
    Function,
    Type,
    Struct,
    Enum,
    Trait,
    Module
};

struct Symbol {
    std::string name;
    SymbolKind kind;
    std::shared_ptr<SemanticType> type;
    SourceLocation location;
    bool isPublic;
    void* irValue; // For code generation
    
    Symbol(std::string n, SymbolKind k, std::shared_ptr<SemanticType> t,
           SourceLocation loc, bool pub = false)
        : name(std::move(n)), kind(k), type(std::move(t)), 
          location(loc), isPublic(pub), irValue(nullptr) {}
};

class Scope {
public:
    explicit Scope(std::shared_ptr<Scope> parent = nullptr) 
        : parent_(std::move(parent)) {}
    
    bool define(std::shared_ptr<Symbol> symbol);
    std::shared_ptr<Symbol> resolve(const std::string& name);
    std::shared_ptr<Symbol> resolveLocal(const std::string& name);
    
    std::shared_ptr<Scope> getParent() const { return parent_; }
    const std::unordered_map<std::string, std::shared_ptr<Symbol>>& getSymbols() const {
        return symbols_;
    }

private:
    std::shared_ptr<Scope> parent_;
    std::unordered_map<std::string, std::shared_ptr<Symbol>> symbols_;
};

class SymbolTable {
public:
    SymbolTable();
    
    void enterScope();
    void exitScope();
    std::shared_ptr<Scope> currentScope() const { return currentScope_; }
    
    bool define(std::shared_ptr<Symbol> symbol);
    std::shared_ptr<Symbol> resolve(const std::string& name);
    std::shared_ptr<Symbol> resolveType(const std::string& name);
    
    // Built-in types
    void initializeBuiltins();

private:
    std::shared_ptr<Scope> globalScope_;
    std::shared_ptr<Scope> currentScope_;
    std::vector<std::shared_ptr<Scope>> allScopes_;
};

} // namespace compiler

#endif // SYMBOL_TABLE_H
