#ifndef TYPE_SYSTEM_H
#define TYPE_SYSTEM_H

#include "../ast/ast_nodes.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace compiler {

class TypeChecker;

// Semantic type representation (distinct from AST types)
class SemanticType {
public:
    enum class Kind {
        Void, Bool,
        Int8, Int16, Int32, Int64,
        UInt8, UInt16, UInt32, UInt64,
        Float32, Float64,
        Char, String,
        Pointer, Reference, Array,
        Function, Struct, Enum, Generic, Error
    };
    
    Kind kind;
    bool isMutable;
    
    explicit SemanticType(Kind k, bool mut = false) : kind(k), isMutable(mut) {}
    virtual ~SemanticType() = default;
    
    virtual std::string toString() const;
    virtual bool equals(const SemanticType& other) const;
    virtual size_t getSize() const;
    
    bool isInteger() const;
    bool isSigned() const;
    bool isUnsigned() const;
    bool isFloat() const;
    bool isNumeric() const;
    bool isPointer() const;
    bool isReference() const;
    bool isArray() const;
    bool isFunction() const;
    bool isComposite() const;
    
    static std::shared_ptr<SemanticType> getPrimitive(Kind k);
};

class PointerType : public SemanticType {
public:
    std::shared_ptr<SemanticType> pointee;
    
    PointerType(std::shared_ptr<SemanticType> p, bool mut = false)
        : SemanticType(Kind::Pointer, mut), pointee(std::move(p)) {}
    
    std::string toString() const override;
    bool equals(const SemanticType& other) const override;
};

class ArrayType : public SemanticType {
public:
    std::shared_ptr<SemanticType> elementType;
    size_t size; // 0 for dynamic
    
    ArrayType(std::shared_ptr<SemanticType> elem, size_t s = 0)
        : SemanticType(Kind::Array), elementType(std::move(elem)), size(s) {}
    
    std::string toString() const override;
    bool equals(const SemanticType& other) const override;
    size_t getSize() const override;
};

class FunctionType : public SemanticType {
public:
    std::vector<std::shared_ptr<SemanticType>> params;
    std::shared_ptr<SemanticType> returnType;
    
    FunctionType(std::vector<std::shared_ptr<SemanticType>> p,
                 std::shared_ptr<SemanticType> r)
        : SemanticType(Kind::Function), params(std::move(p)), returnType(std::move(r)) {}
    
    std::string toString() const override;
    bool equals(const SemanticType& other) const override;
};

class StructType : public SemanticType {
public:
    struct Field {
        std::string name;
        std::shared_ptr<SemanticType> type;
        size_t offset;
    };
    
    std::string name;
    std::vector<Field> fields;
    std::unordered_map<std::string, size_t> fieldMap;
    
    StructType(std::string n, std::vector<Field> f)
        : SemanticType(Kind::Struct), name(std::move(n)), fields(std::move(f)) {
        for (size_t i = 0; i < fields.size(); ++i) {
            fieldMap[fields[i].name] = i;
        }
    }
    
    std::optional<Field> getField(const std::string& name) const;
    std::string toString() const override;
    bool equals(const SemanticType& other) const override;
    size_t getSize() const override;
};

class EnumType : public SemanticType {
public:
    struct Variant {
        std::string name;
        std::vector<std::shared_ptr<SemanticType>> types;
    };
    
    std::string name;
    std::vector<Variant> variants;
    
    EnumType(std::string n, std::vector<Variant> v)
        : SemanticType(Kind::Enum), name(std::move(n)), variants(std::move(v)) {}
    
    std::string toString() const override;
    bool equals(const SemanticType& other) const override;
};

// Type conversion utilities
class TypeConversions {
public:
    static bool isImplicitlyConvertible(std::shared_ptr<SemanticType> from,
                                       std::shared_ptr<SemanticType> to);
    static bool isExplicitlyConvertible(std::shared_ptr<SemanticType> from,
                                        std::shared_ptr<SemanticType> to);
    static std::shared_ptr<SemanticType> getCommonType(std::shared_ptr<SemanticType> a,
                                                       std::shared_ptr<SemanticType> b);
    static std::shared_ptr<SemanticType> promoteInteger(std::shared_ptr<SemanticType> type);
};

} // namespace compiler

#endif // TYPE_SYSTEM_H
