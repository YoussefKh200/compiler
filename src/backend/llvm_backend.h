#ifndef LLVM_BACKEND_H
#define LLVM_BACKEND_H

#include "../ir/ir.h"
#include <memory>
#include <string>
#include <unordered_map>

// Forward declarations for LLVM
namespace llvm {
    class LLVMContext;
    class Module;
    class Function;
    class BasicBlock;
    class Value;
    class Type;
    class Instruction;
    class TargetMachine;
    class DataLayout;
    class raw_pwrite_stream;
    template<typename T> class ArrayRef;
}

namespace compiler {

class LLVMBackend {
public:
    LLVMBackend();
    ~LLVMBackend();
    
    // Initialize LLVM targets
    static void initializeTargets();
    
    // Code generation
    bool generateModule(Module* irModule);
    bool generateFunction(Function* func);
    bool generateBasicBlock(BasicBlock* bb);
    llvm::Value* generateInstruction(Instruction* inst);
    
    // Output
    bool emitObjectFile(const std::string& filename);
    bool emitAssembly(const std::string& filename);
    bool emitLLVMIR(const std::string& filename);
    
    // JIT support
    void* getFunctionPointer(const std::string& name);
    
    // Optimization
    void runLLVMOptimizations(OptLevel level);
    
    // Target info
    void setTargetTriple(const std::string& triple);
    void setCPU(const std::string& cpu);
    void setFeatures(const std::string& features);

private:
    std::unique_ptr<llvm::LLVMContext> context_;
    std::unique_ptr<llvm::Module> llvmModule_;
    std::unique_ptr<llvm::TargetMachine> targetMachine_;
    
    std::unordered_map<Function*, llvm::Function*> functionMap_;
    std::unordered_map<Value*, llvm::Value*> valueMap_;
    std::unordered_map<BasicBlock*, llvm::BasicBlock*> blockMap_;
    
    // Current function context
    Function* currentIRFunction_;
    llvm::Function* currentLLVMFunction_;
    
    // Type conversion
    llvm::Type* convertType(IRType type);
    
    // Value conversion
    llvm::Value* getValue(Value* v);
    
    // Instruction generation helpers
    llvm::Instruction* createBinaryOp(Opcode op, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Instruction* createCast(Opcode op, llvm::Value* val, llvm::Type* destType);
    llvm::CmpInst::Predicate getPredicate(Opcode op);
};

} // namespace compiler

#endif // LLVM_BACKEND_H
