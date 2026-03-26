#ifndef IR_BUILDER_H
#define IR_BUILDER_H

#include "ir.h"
#include <stack>

namespace compiler {

class IRBuilder {
public:
    explicit IRBuilder(Module* mod) : module_(mod), currentBlock_(nullptr) {}
    
    // Block management
    void setInsertPoint(BasicBlock* bb) { currentBlock_ = bb; }
    BasicBlock* getInsertPoint() const { return currentBlock_; }
    
    // Constants
    Value* getInt32(int32_t val);
    Value* getInt64(int64_t val);
    Value* getFloat(double val);
    Value* getBool(bool val);
    Value* getNull();
    
    // Allocations
    Value* createAlloca(IRType type, const std::string& name = "");
    Value* createLoad(Value* ptr, const std::string& name = "");
    Value* createStore(Value* val, Value* ptr);
    Value* createGEP(Value* ptr, Value* index, const std::string& name = "");
    
    // Arithmetic
    Value* createAdd(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createSub(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createMul(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createSDiv(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createUDiv(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createSRem(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createFAdd(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createFSub(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createFMul(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createFDiv(Value* lhs, Value* rhs, const std::string& name = "");
    
    // Bitwise
    Value* createAnd(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createOr(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createXor(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createShl(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createLShr(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createAShr(Value* lhs, Value* rhs, const std::string& name = "");
    
    // Comparisons
    Value* createICmpEQ(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createICmpNE(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createICmpSLT(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createICmpSLE(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createICmpSGT(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createICmpSGE(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createFCmpOEQ(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createFCmpOLT(Value* lhs, Value* rhs, const std::string& name = "");
    
    // Control flow
    Value* createRet(Value* val = nullptr);
    Value* createBr(BasicBlock* dest);
    Value* createCondBr(Value* cond, BasicBlock* trueDest, BasicBlock* falseDest);
    
    // Calls
    Value* createCall(Function* func, const std::vector<Value*>& args, 
                      const std::string& name = "");
    
    // Casts
    Value* createTrunc(Value* val, IRType destType, const std::string& name = "");
    Value* createZExt(Value* val, IRType destType, const std::string& name = "");
    Value* createSExt(Value* val, IRType destType, const std::string& name = "");
    Value* createFPToSI(Value* val, IRType destType, const std::string& name = "");
    Value* createSIToFP(Value* val, IRType destType, const std::string& name = "");
    Value* createPtrToInt(Value* val, IRType destType, const std::string& name = "");
    Value* createIntToPtr(Value* val, IRType destType, const std::string& name = "");
    
    // PHI
    Value* createPHI(IRType type, const std::vector<std::pair<Value*, BasicBlock*>>& incoming,
                     const std::string& name = "");
    
    // Utilities
    std::string getUniqueName(const std::string& base);
    
private:
    Module* module_;
    BasicBlock* currentBlock_;
    
    Instruction* createInstruction(Opcode op, IRType type, 
                                   const std::vector<Value*>& operands,
                                   const std::string& name);
};

} // namespace compiler

#endif // IR_BUILDER_H
