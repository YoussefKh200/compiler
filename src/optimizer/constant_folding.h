#ifndef CONSTANT_FOLDING_H
#define CONSTANT_FOLDING_H

#include "optimizer.h"

namespace compiler {

class ConstantFoldingPass {
public:
    bool run(Function* func);
    
private:
    bool foldInstruction(Instruction* inst);
    Constant* evaluateBinaryOp(Opcode op, Constant* lhs, Constant* rhs);
    Constant* evaluateCast(Opcode op, Constant* val, IRType destType);
    
    template<typename T>
    T getValue(Constant* c);
    
    template<typename T>
    Constant* createConstant(T val, IRType type);
};

} // namespace compiler

#endif // CONSTANT_FOLDING_H
