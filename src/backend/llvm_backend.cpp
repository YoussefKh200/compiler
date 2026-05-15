#include "llvm_backend.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/Analysis/TargetLibraryInfo.h>

namespace compiler {

LLVMBackend::LLVMBackend() 
    : context_(std::make_unique<llvm::LLVMContext>()),
      llvmModule_(nullptr),
      targetMachine_(nullptr),
      currentIRFunction_(nullptr),
      currentLLVMFunction_(nullptr) {
}

LLVMBackend::~LLVMBackend() = default;

void LLVMBackend::initializeTargets() {
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();
}

bool LLVMBackend::generateModule(Module* irModule) {
    llvmModule_ = std::make_unique<llvm::Module>(irModule->name, *context_);
    
    // Set target triple if specified
    if (!targetTriple_.empty()) {
        llvmModule_->setTargetTriple(targetTriple_);
    }
    
    // Generate globals
    for (auto& global : irModule->globals) {
        // Create LLVM global variable
        auto* type = convertType(global->type);
        auto* llvmGlobal = new llvm::GlobalVariable(
            *llvmModule_, type, global->isConstant,
            llvm::GlobalValue::ExternalLinkage,
            nullptr, global->name
        );
        valueMap_[global.get()] = llvmGlobal;
    }
    
    // Generate function declarations
    for (auto& func : irModule->functions) {
        generateFunction(func.get());
    }
    
    // Verify module
    std::string error;
    llvm::raw_string_ostream errorStream(error);
    if (llvm::verifyModule(*llvmModule_, &errorStream)) {
        std::cerr << "LLVM module verification failed: " << error << "\n";
        return false;
    }
    
    return true;
}

bool LLVMBackend::generateFunction(Function* func) {
    currentIRFunction_ = func;
    
    // Create function type
    std::vector<llvm::Type*> paramTypes;
    for (auto& param : func->parameters) {
        paramTypes.push_back(convertType(param.second));
    }
    
    auto* retType = convertType(func->returnType);
    auto* funcType = llvm::FunctionType::get(retType, paramTypes, false);
    
    // Create function
    auto linkage = func->isExternal ? 
        llvm::Function::ExternalLinkage : 
        llvm::Function::InternalLinkage;
    
    currentLLVMFunction_ = llvm::Function::Create(
        funcType, linkage, func->name, llvmModule_.get()
    );
    
    functionMap_[func] = currentLLVMFunction_;
    
    // Set parameter names
    size_t idx = 0;
    for (auto& arg : currentLLVMFunction_->args()) {
        arg.setName(func->parameters[idx++].first);
    }
    
    if (func->isExternal) {
        return true;
    }
    
    // Create basic blocks
    for (auto& bb : func->basicBlocks) {
        auto* llvmBB = llvm::BasicBlock::Create(*context_, bb->name, currentLLVMFunction_);
        blockMap_[bb.get()] = llvmBB;
    }
    
    // Generate instructions
    for (auto& bb : func->basicBlocks) {
        if (!generateBasicBlock(bb.get())) {
            return false;
        }
    }
    
    return true;
}

bool LLVMBackend::generateBasicBlock(BasicBlock* bb) {
    auto* llvmBB = blockMap_[bb];
    llvm::IRBuilder<> builder(llvmBB);
    
    for (auto& inst : bb->instructions) {
        llvm::Value* val = generateInstruction(inst.get(), builder);
        if (!val && inst->type != IRType::Void) {
            return false;
        }
        valueMap_[inst.get()] = val;
    }
    
    return true;
}

llvm::Value* LLVMBackend::generateInstruction(Instruction* inst, llvm::IRBuilder<>& builder) {
    switch (inst->opcode) {
        case Opcode::Ret: {
            if (inst->operands.empty()) {
                return builder.CreateRetVoid();
            } else {
                return builder.CreateRet(getValue(inst->operands[0]));
            }
        }
        
        case Opcode::Br: {
            auto* dest = reinterpret_cast<BasicBlock*>(inst->operands[0]);
            return builder.CreateBr(blockMap_[dest]);
        }
        
        case Opcode::CondBr: {
            auto* cond = getValue(inst->operands[0]);
            auto* trueDest = reinterpret_cast<BasicBlock*>(inst->operands[1]);
            auto* falseDest = reinterpret_cast<BasicBlock*>(inst->operands[2]);
            return builder.CreateCondBr(cond, blockMap_[trueDest], blockMap_[falseDest]);
        }
        
        case Opcode::Add: {
            auto* lhs = getValue(inst->operands[0]);
            auto* rhs = getValue(inst->operands[1]);
            return builder.CreateAdd(lhs, rhs, inst->name);
        }
        
        case Opcode::Sub: {
            auto* lhs = getValue(inst->operands[0]);
            auto* rhs = getValue(inst->operands[1]);
            return builder.CreateSub(lhs, rhs, inst->name);
        }
        
        case Opcode::Mul: {
            auto* lhs = getValue(inst->operands[0]);
            auto* rhs = getValue(inst->operands[1]);
            return builder.CreateMul(lhs, rhs, inst->name);
        }
        
        case Opcode::SDiv: {
            auto* lhs = getValue(inst->operands[0]);
            auto* rhs = getValue(inst->operands[1]);
            return builder.CreateSDiv(lhs, rhs, inst->name);
        }
        
        case Opcode::FAdd: {
            auto* lhs = getValue(inst->operands[0]);
            auto* rhs = getValue(inst->operands[1]);
            return builder.CreateFAdd(lhs, rhs, inst->name);
        }
        
        case Opcode::FSub: {
            auto* lhs = getValue(inst->operands[0]);
            auto* rhs = getValue(inst->operands[1]);
            return builder.CreateFSub(lhs, rhs, inst->name);
        }
        
        case Opcode::FMul: {
            auto* lhs = getValue(inst->operands[0]);
            auto* rhs = getValue(inst->operands[1]);
            return builder.CreateFMul(lhs, rhs, inst->name);
        }
        
        case Opcode::FDiv: {
            auto* lhs = getValue(inst->operands[0]);
            auto* rhs = getValue(inst->operands[1]);
            return builder.CreateFDiv(lhs, rhs, inst->name);
        }
        
        case Opcode::And: {
            auto* lhs = getValue(inst->operands[0]);
            auto* rhs = getValue(inst->operands[1]);
            return builder.CreateAnd(lhs, rhs, inst->name);
        }
        
        case Opcode::Or: {
            auto* lhs = getValue(inst->operands[0]);
            auto* rhs = getValue(inst->operands[1]);
            return builder.CreateOr(lhs, rhs, inst->name);
        }
        
        case Opcode::Xor: {
            auto* lhs = getValue(inst->operands[0]);
            auto* rhs = getValue(inst->operands[1]);
            return builder.CreateXor(lhs, rhs, inst->name);
        }
        
        case Opcode::ICmpEq: {
            auto* lhs = getValue(inst->operands[0]);
            auto* rhs = getValue(inst->operands[1]);
            return builder.CreateICmpEQ(lhs, rhs, inst->name);
        }
        
        case Opcode::ICmpNe: {
            auto* lhs = getValue(inst->operands[0]);
            auto* rhs = getValue(inst->operands[1]);
            return builder.CreateICmpNE(lhs, rhs, inst->name);
        }
        
        case Opcode::ICmpSlt: {
            auto* lhs = getValue(inst->operands[0]);
            auto* rhs = getValue(inst->operands[1]);
            return builder.CreateICmpSLT(lhs, rhs, inst->name);
        }
        
        case Opcode::Alloca: {
            auto* type = convertType(inst->type);
            return builder.CreateAlloca(type, nullptr, inst->name);
        }
        
        case Opcode::Load: {
            auto* ptr = getValue(inst->operands[0]);
            return builder.CreateLoad(convertType(inst->type), ptr, inst->name);
        }
        
        case Opcode::Store: {
            auto* val = getValue(inst->operands[0]);
            auto* ptr = getValue(inst->operands[1]);
            return builder.CreateStore(val, ptr);
        }
        
        case Opcode::Call: {
            auto* callee = reinterpret_cast<Function*>(inst->operands[0]);
            auto* llvmCallee = functionMap_[callee];
            
            std::vector<llvm::Value*> args;
            for (size_t i = 1; i < inst->operands.size(); ++i) {
                args.push_back(getValue(inst->operands[i]));
            }
            
            return builder.CreateCall(llvmCallee, args, inst->name);
        }
        
        case Opcode::Phi: {
            auto* phi = builder.CreatePHI(convertType(inst->type), 
                                          inst->operands.size() / 2, inst->name);
            for (size_t i = 0; i < inst->operands.size(); i += 2) {
                auto* val = getValue(inst->operands[i]);
                auto* block = reinterpret_cast<BasicBlock*>(inst->operands[i + 1]);
                phi->addIncoming(val, blockMap_[block]);
            }
            return phi;
        }
        
        case Opcode::Trunc: {
            auto* val = getValue(inst->operands[0]);
            return builder.CreateTrunc(val, convertType(inst->type), inst->name);
        }
        
        case Opcode::ZExt: {
            auto* val = getValue(inst->operands[0]);
            return builder.CreateZExt(val, convertType(inst->type), inst->name);
        }
        
        case Opcode::SExt: {
            auto* val = getValue(inst->operands[0]);
            return builder.CreateSExt(val, convertType(inst->type), inst->name);
        }
        
        case Opcode::FPToSI: {
            auto* val = getValue(inst->operands[0]);
            return builder.CreateFPToSI(val, convertType(inst->type), inst->name);
        }
        
        case Opcode::SIToFP: {
            auto* val = getValue(inst->operands[0]);
            return builder.CreateSIToFP(val, convertType(inst->type), inst->name);
        }
        
        case Opcode::PtrToInt: {
            auto* val = getValue(inst->operands[0]);
            return builder.CreatePtrToInt(val, convertType(inst->type), inst->name);
        }
        
        case Opcode::IntToPtr: {
            auto* val = getValue(inst->operands[0]);
            return builder.CreateIntToPtr(val, convertType(inst->type), inst->name);
        }
        
        default:
            std::cerr << "Unknown opcode in LLVM backend: " << static_cast<int>(inst->opcode) << "\n";
            return nullptr;
    }
}

llvm::Type* LLVMBackend::convertType(IRType type) {
    switch (type) {
        case IRType::Void: return llvm::Type::getVoidTy(*context_);
        case IRType::I1: return llvm::Type::getInt1Ty(*context_);
        case IRType::I8: return llvm::Type::getInt8Ty(*context_);
        case IRType::I16: return llvm::Type::getInt16Ty(*context_);
        case IRType::I32: return llvm::Type::getInt32Ty(*context_);
        case IRType::I64: return llvm::Type::getInt64Ty(*context_);
        case IRType::F32: return llvm::Type::getFloatTy(*context_);
        case IRType::F64: return llvm::Type::getDoubleTy(*context_);
        case IRType::Ptr: return llvm::Type::getInt8PtrTy(*context_);
        default: return llvm::Type::getVoidTy(*context_);
    }
}

llvm::Value* LLVMBackend::getValue(Value* v) {
    if (auto* c = dynamic_cast<Constant*>(v)) {
        if (std::holds_alternative<int64_t>(c->data)) {
            int64_t val = std::get<int64_t>(c->data);
            switch (c->type) {
                case IRType::I1: return llvm::ConstantInt::getBool(*context_, val != 0);
                case IRType::I8: return llvm::ConstantInt::get(*context_, llvm::APInt(8, val));
                case IRType::I16: return llvm::ConstantInt::get(*context_, llvm::APInt(16, val));
                case IRType::I32: return llvm::ConstantInt::get(*context_, llvm::APInt(32, val));
                case IRType::I64: return llvm::ConstantInt::get(*context_, llvm::APInt(64, val));
                default: break;
            }
        } else if (std::holds_alternative<double>(c->data)) {
            return llvm::ConstantFP::get(*context_, llvm::APFloat(std::get<double>(c->data)));
        } else if (std::holds_alternative<bool>(c->data)) {
            return llvm::ConstantInt::getBool(*context_, std::get<bool>(c->data));
        }
    }
    
    auto it = valueMap_.find(v);
    if (it != valueMap_.end()) {
        return it->second;
    }
    
    return nullptr;
}

void LLVMBackend::runLLVMOptimizations(OptLevel level) {
    llvm::legacy::FunctionPassManager fpm(llvmModule_.get());
    llvm::legacy::PassManager mpm;
    
    // Add target information
    auto* tli = new llvm::TargetLibraryInfoImpl(llvm::Triple(llvmModule_->getTargetTriple()));
    mpm.add(new llvm::TargetLibraryInfoWrapperPass(*tli));
    
    switch (level) {
        case OptLevel::O0:
            break;
            
        case OptLevel::O1:
            fpm.add(llvm::createInstructionCombiningPass());
            fpm.add(llvm::createReassociatePass());
            fpm.add(llvm::createGVNPass());
            fpm.add(llvm::createCFGSimplificationPass());
            break;
            
        case OptLevel::O2:
        case OptLevel::Os:
            fpm.add(llvm::createInstructionCombiningPass());
            fpm.add(llvm::createReassociatePass());
            fpm.add(llvm::createGVNPass());
            fpm.add(llvm::createCFGSimplificationPass());
            fpm.add(llvm::createLoopSimplifyPass());
            fpm.add(llvm::createLoopRotatePass());
            fpm.add(llvm::createLICMPass());
            break;
            
        case OptLevel::O3:
            fpm.add(llvm::createAggressiveDCEPass());
            fpm.add(llvm::createInstructionCombiningPass());
            fpm.add(llvm::createReassociatePass());
            fpm.add(llvm::createGVNPass());
            fpm.add(llvm::createCFGSimplificationPass());
            fpm.add(llvm::createLoopSimplifyPass());
            fpm.add(llvm::createLoopRotatePass());
            fpm.add(llvm::createLICMPass());
            fpm.add(llvm::createLoopUnrollPass());
            fpm.add(llvm::createLoopVectorizePass());
            fpm.add(llvm::createSLPVectorizerPass());
            break;
            
        default:
            break;
    }
    
    fpm.doInitialization();
    for (auto& func : llvmModule_->functions()) {
        fpm.run(func);
    }
    fpm.doFinalization();
    
    mpm.run(*llvmModule_);
}

bool LLVMBackend::emitObjectFile(const std::string& filename) {
    auto targetTriple = llvmModule_->getTargetTriple();
    if (targetTriple.empty()) {
        targetTriple = llvm::sys::getDefaultTargetTriple();
    }
    
    std::string error;
    auto* target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
    if (!target) {
        std::cerr << "Target lookup failed: " << error << "\n";
        return false;
    }
    
    auto cpu = "generic";
    auto features = "";
    
    llvm::TargetOptions opt;
    auto rm = llvm::Reloc::Model::PIC_;
    targetMachine_ = target->createTargetMachine(targetTriple, cpu, features, opt, rm);
    
    llvmModule_->setDataLayout(targetMachine_->createDataLayout());
    
    std::error_code ec;
    llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_None);
    if (ec) {
        std::cerr << "Could not open file: " << ec.message() << "\n";
        return false;
    }
    
    llvm::legacy::PassManager pass;
    if (targetMachine_->addPassesToEmitFile(pass, dest, nullptr, llvm::CGFT_ObjectFile)) {
        std::cerr << "Target machine cannot emit object file\n";
        return false;
    }
    
    pass.run(*llvmModule_);
    dest.flush();
    
    return true;
}

bool LLVMBackend::emitAssembly(const std::string& filename) {
    std::error_code ec;
    llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_None);
    if (ec) {
        std::cerr << "Could not open file: " << ec.message() << "\n";
        return false;
    }
    
    llvmModule_->print(dest, nullptr);
    return true;
}

bool LLVMBackend::emitLLVMIR(const std::string& filename) {
    std::error_code ec;
    llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_None);
    if (ec) {
        std::cerr << "Could not open file: " << ec.message() << "\n";
        return false;
    }
    
    llvmModule_->print(dest, nullptr);
    return true;
}

void* LLVMBackend::getFunctionPointer(const std::string& name) {
    // This would require JIT compilation
    // Return address from JIT session
    return nullptr;
}

void LLVMBackend::setTargetTriple(const std::string& triple) {
    targetTriple_ = triple;
}

void LLVMBackend::setCPU(const std::string& cpu) {
    // Store for target machine creation
}

void LLVMBackend::setFeatures(const std::string& features) {
    // Store for target machine creation
}

} // namespace compiler
