#ifndef IR_H
#define IR_H

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <variant>

namespace compiler {

// Forward declarations
class BasicBlock;
class Function;
class Module;
class IRBuilder;

// Value types
enum class IRType {
    Void,
    I1, I8, I16, I32, I64,
    F32, F64,
    Ptr,
    Array,
    Struct,
    Function
};

// Instruction opcodes
enum class Opcode {
    // Terminators
    Ret, Br, CondBr, Switch, Unreachable,
    
    // Binary arithmetic
    Add, Sub, Mul, SDiv, UDiv, SRem, URem,
    FAdd, FSub, FMul, FDiv, FRem,
    
    // Bitwise
    And, Or, Xor, Shl, LShr, AShr,
    
    // Memory
    Alloca, Load, Store, GEP, // GetElementPtr
    
    // Comparison
    ICmpEq, ICmpNe, ICmpSlt, ICmpSle, ICmpSgt, ICmpSge,
    ICmpUlt, ICmpUle, ICmpUgt, ICmpUge,
    FCmpOEq, FCmpONe, FCmpOLT, FCmpOLE, FCmpOGT, FCmpOGE,
    
    // Conversions
    Trunc, ZExt, SExt, FPToUI, FPToSI, UIToFP, SIToFP,
    FPTrunc, FPExt, PtrToInt, IntToPtr, Bitcast,
    
    // Other
    Phi, Call, Select, ExtractValue, InsertValue,
    
    // SSA construction
    Alloc, // Virtual allocation for SSA construction
};

// Value base class (SSA values)
class Value {
public:
    std::string name;
    IRType type;
    uint32_t id;
    
    Value(std::string n, IRType t, uint32_t i) 
        : name(std::move(n)), type(t), id(i) {}
    virtual ~Value() = default;
    
    virtual std::string toString() const;
    bool isConstant() const;
};

// Constant values
class Constant : public Value {
public:
    std::variant<int64_t, double, bool> data;
    
    Constant(IRType t, int64_t v, uint32_t id) 
        : Value("", t, id), data(v) {}
    Constant(IRType t, double v, uint32_t id) 
        : Value("", t, id), data(v) {}
    Constant(IRType t, bool v, uint32_t id) 
        : Value("", t, id), data(v) {}
    
    std::string toString() const override;
};

// Instruction
class Instruction : public Value {
public:
    Opcode opcode;
    BasicBlock* parent;
    std::vector<Value*> operands;
    
    Instruction(std::string n, Opcode op, IRType t, uint32_t id, BasicBlock* bb)
        : Value(std::move(n), t, id), opcode(op), parent(bb) {}
    
    void addOperand(Value* v) { operands.push_back(v); }
    std::string toString() const override;
};

// Basic Block
class BasicBlock : public Value {
public:
    Function* parent;
    std::vector<std::unique_ptr<Instruction>> instructions;
    std::vector<BasicBlock*> predecessors;
    std::vector<BasicBlock*> successors;
    
    BasicBlock(std::string n, uint32_t id, Function* f)
        : Value(std::move(n), IRType::Void, id), parent(f) {}
    
    void addInstruction(std::unique_ptr<Instruction> inst);
    Instruction* getTerminator() const;
    bool isTerminated() const;
    
    // SSA
    std::unordered_map<std::string, Value*> symbolTable; // For SSA construction
};

// Function
class Function : public Value {
public:
    Module* parent;
    IRType returnType;
    std::vector<std::pair<std::string, IRType>> parameters;
    std::vector<std::unique_ptr<BasicBlock>> basicBlocks;
    std::unordered_map<std::string, Value*> symbolTable;
    bool isExternal;
    
    Function(std::string n, IRType ret, uint32_t id, Module* m, bool ext = false)
        : Value(std::move(n), IRType::Function, id), parent(m), 
          returnType(ret), isExternal(ext) {}
    
    BasicBlock* createBasicBlock(const std::string& name);
    BasicBlock* getEntryBlock() const;
    size_t getInstructionCount() const;
};

// Global variable
class GlobalVariable : public Value {
public:
    bool isConstant;
    std::unique_ptr<Constant> initializer;
    
    GlobalVariable(std::string n, IRType t, uint32_t id, bool constant = false)
        : Value(std::move(n), t, id), isConstant(constant) {}
};

// Module (compilation unit)
class Module {
public:
    std::string name;
    std::vector<std::unique_ptr<Function>> functions;
    std::vector<std::unique_ptr<GlobalVariable>> globals;
    uint32_t nextValueId;
    
    explicit Module(std::string n) : name(std::move(n)), nextValueId(0) {}
    
    Function* createFunction(const std::string& name, IRType retType, bool external = false);
    GlobalVariable* createGlobal(const std::string& name, IRType type, bool constant = false);
    uint32_t getNextId() { return nextValueId++; }
    
    std::string toString() const;
};

// IR Type utilities
std::string irTypeToString(IRType type);
size_t getTypeSize(IRType type);

} // namespace compiler

#endif // IR_H
