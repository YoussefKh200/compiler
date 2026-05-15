#include "ir_builder.h"

namespace compiler {

Value* IRBuilder::getInt32(int32_t val) {
    auto* c = new Constant(IRType::I32, static_cast<int64_t>(val), module_->getNextId());
    // In real implementation, manage this with the module
    return c;
}

Value* IRBuilder::getInt64(int64_t val) {
    return new Constant(IRType::I64, val, module_->getNextId());
}

Value* IRBuilder::getFloat(double val) {
    return new Constant(IRType::F64, val, module_->getNextId());
}

Value* IRBuilder::getBool(bool val) {
    return new Constant(IRType::I1, val, module_->getNextId());
}

Value* IRBuilder::getNull() {
    return new Constant(IRType::Ptr, int64_t(0), module_->getNextId());
}

Instruction* IRBuilder::createInstruction(Opcode op, IRType type, 
                                            const std::vector<Value*>& operands,
                                            const std::string& name) {
    auto inst = std::make_unique<Instruction>(
        name.empty() ? getUniqueName("tmp") : name,
        op, type, module_->getNextId(), currentBlock_
    );
    
    for (auto* op : operands) {
        inst->addOperand(op);
    }
    
    auto* ptr = inst.get();
    currentBlock_->addInstruction(std::move(inst));
    return ptr;
}

Value* IRBuilder::createAlloca(IRType type, const std::string& name) {
    return createInstruction(Opcode::Alloca, type, {}, name.empty() ? "alloca" : name);
}

Value* IRBuilder::createLoad(Value* ptr, const std::string& name) {
    // Type would be determined from ptr in real implementation
    return createInstruction(Opcode::Load, IRType::I32, {ptr}, name);
}

Value* IRBuilder::createStore(Value* val, Value* ptr) {
    return createInstruction(Opcode::Store, IRType::Void, {val, ptr}, "");
}

Value* IRBuilder::createGEP(Value* ptr, Value* index, const std::string& name) {
    return createInstruction(Opcode::GEP, IRType::Ptr, {ptr, index}, name);
}

Value* IRBuilder::createAdd(Value* lhs, Value* rhs, const std::string& name) {
    return createInstruction(Opcode::Add, lhs->type, {lhs, rhs}, name);
}

Value* IRBuilder::createSub(Value* lhs, Value* rhs, const std::string& name) {
    return createInstruction(Opcode::Sub, lhs->type, {lhs, rhs}, name);
}

Value* IRBuilder::createMul(Value* lhs, Value* rhs, const std::string& name) {
    return createInstruction(Opcode::Mul, lhs->type, {lhs, rhs}, name);
}

Value* IRBuilder::createSDiv(Value* lhs, Value* rhs, const std::string& name) {
    return createInstruction(Opcode::SDiv, lhs->type, {lhs, rhs}, name);
}

Value* IRBuilder::createUDiv(Value* lhs, Value* rhs, const std::string& name) {
    return createInstruction(Opcode::UDiv, lhs->type, {lhs, rhs}, name);
}

Value* IRBuilder::createSRem(Value* lhs, Value* rhs, const std::string& name) {
    return createInstruction(Opcode::SRem, lhs->type, {lhs, rhs}, name);
}

Value* IRBuilder::createFAdd(Value* lhs, Value* rhs, const std::string& name) {
    return createInstruction(Opcode::FAdd, lhs->type, {lhs, rhs}, name);
}

Value* IRBuilder::createFSub(Value* lhs, Value* rhs, const std::string& name) {
    return createInstruction(Opcode::FSub, lhs->type, {lhs, rhs}, name);
}

Value* IRBuilder::createFMul(Value* lhs, Value* rhs, const std::string& name) {
    return createInstruction(Opcode::FMul, lhs->type, {lhs, rhs}, name);
}

Value* IRBuilder::createFDiv(Value* lhs, Value* rhs, const std::string& name) {
    return createInstruction(Opcode::FDiv, lhs->type, {lhs, rhs}, name);
}

Value* IRBuilder::createAnd(Value* lhs, Value* rhs, const std::string& name) {
    return createInstruction(Opcode::And, lhs->type, {lhs, rhs}, name);
}

Value* IRBuilder::createOr(Value* lhs, Value* rhs, const std::string& name) {
    return createInstruction(Opcode::Or, lhs->type, {lhs, rhs}, name);
}

Value* IRBuilder::createXor(Value* lhs, Value* rhs, const std::string& name) {
    return createInstruction(Opcode::Xor, lhs->type, {lhs, rhs}, name);
}

Value* IRBuilder::createShl(Value* lhs, Value* rhs, const std::string& name) {
    return createInstruction(Opcode::Shl, lhs->type, {lhs, rhs}, name);
}

Value* IRBuilder::createLShr(Value* lhs, Value* rhs, const std::string& name) {
    return createInstruction(Opcode::LShr, lhs->type, {lhs, rhs}, name);
}

Value* IRBuilder::createAShr(Value* lhs, Value* rhs, const std::string& name) {
    return createInstruction(Opcode::AShr, lhs->type, {lhs, rhs}, name);
}

Value* IRBuilder::createICmpEQ(Value* lhs, Value* rhs, const std::string& name) {
    return createInstruction(Opcode::ICmpEq, IRType::I1, {lhs, rhs}, name);
}

Value* IRBuilder::createICmpNE(Value* lhs, Value* rhs, const std::string& name) {
    return createInstruction(Opcode::ICmpNe, IRType::I1, {lhs, rhs}, name);
}

Value* IRBuilder::createICmpSLT(Value* lhs, Value* rhs, const std::string& name) {
    return createInstruction(Opcode::ICmpSlt, IRType::I1, {lhs, rhs}, name);
}

Value* IRBuilder::createICmpSLE(Value* lhs, Value* rhs, const std::string& name) {
    return createInstruction(Opcode::ICmpSle, IRType::I1, {lhs, rhs}, name);
}

Value* IRBuilder::createICmpSGT(Value* lhs, Value* rhs, const std::string& name) {
    return createInstruction(Opcode::ICmpSgt, IRType::I1, {lhs, rhs}, name);
}

Value* IRBuilder::createICmpSGE(Value* lhs, Value* rhs, const std::string& name) {
    return createInstruction(Opcode::ICmpSge, IRType::I1, {lhs, rhs}, name);
}

Value* IRBuilder::createFCmpOEQ(Value* lhs, Value* rhs, const std::string& name) {
    return createInstruction(Opcode::FCmpOEq, IRType::I1, {lhs, rhs}, name);
}

Value* IRBuilder::createFCmpOLT(Value* lhs, Value* rhs, const std::string& name) {
    return createInstruction(Opcode::FCmpOLT, IRType::I1, {lhs, rhs}, name);
}

Value* IRBuilder::createRet(Value* val) {
    if (val) {
        return createInstruction(Opcode::Ret, IRType::Void, {val}, "");
    }
    return createInstruction(Opcode::Ret, IRType::Void, {}, "");
}

Value* IRBuilder::createBr(BasicBlock* dest) {
    auto* inst = createInstruction(Opcode::Br, IRType::Void, {}, "");
    inst->addOperand(reinterpret_cast<Value*>(dest)); // Hack: store block as value
    return inst;
}

Value* IRBuilder::createCondBr(Value* cond, BasicBlock* trueDest, BasicBlock* falseDest) {
    auto* inst = createInstruction(Opcode::CondBr, IRType::Void, {cond}, "");
    inst->addOperand(reinterpret_cast<Value*>(trueDest));
    inst->addOperand(reinterpret_cast<Value*>(falseDest));
    return inst;
}

Value* IRBuilder::createCall(Function* func, const std::vector<Value*>& args, 
                              const std::string& name) {
    std::vector<Value*> operands;
    operands.push_back(reinterpret_cast<Value*>(func));
    for (auto* arg : args) {
        operands.push_back(arg);
    }
    return createInstruction(Opcode::Call, func->returnType, operands, name);
}

Value* IRBuilder::createTrunc(Value* val, IRType destType, const std::string& name) {
    return createInstruction(Opcode::Trunc, destType, {val}, name);
}

Value* IRBuilder::createZExt(Value* val, IRType destType, const std::string& name) {
    return createInstruction(Opcode::ZExt, destType, {val}, name);
}

Value* IRBuilder::createSExt(Value* val, IRType destType, const std::string& name) {
    return createInstruction(Opcode::SExt, destType, {val}, name);
}

Value* IRBuilder::createFPToSI(Value* val, IRType destType, const std::string& name) {
    return createInstruction(Opcode::FPToSI, destType, {val}, name);
}

Value* IRBuilder::createSIToFP(Value* val, IRType destType, const std::string& name) {
    return createInstruction(Opcode::SIToFP, destType, {val}, name);
}

Value* IRBuilder::createPtrToInt(Value* val, IRType destType, const std::string& name) {
    return createInstruction(Opcode::PtrToInt, destType, {val}, name);
}

Value* IRBuilder::createIntToPtr(Value* val, IRType destType, const std::string& name) {
    return createInstruction(Opcode::IntToPtr, destType, {val}, name);
}

Value* IRBuilder::createPHI(IRType type, const std::vector<std::pair<Value*, BasicBlock*>>& incoming,
                              const std::string& name) {
    std::vector<Value*> operands;
    for (const auto& [val, block] : incoming) {
        operands.push_back(val);
        operands.push_back(reinterpret_cast<Value*>(block));
    }
    return createInstruction(Opcode::Phi, type, operands, name);
}

std::string IRBuilder::getUniqueName(const std::string& base) {
    static int counter = 0;
    return base + std::to_string(counter++);
}

} // namespace compiler
