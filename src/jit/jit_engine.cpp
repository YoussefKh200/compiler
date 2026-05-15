#include "jit_engine.h"
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Error.h>

namespace compiler {

JITEngine::JITEngine() : sessionActive_(false), currentDylib_(nullptr) {}

JITEngine::~JITEngine() = default;

bool JITEngine::initialize() {
    llvm::Error err = llvm::Error::success();
    
    auto jitBuilder = llvm::orc::LLJITBuilder();
    jit_ = cantFail(jitBuilder.create());
    
    context_ = std::make_unique<llvm::LLVMContext>();
    
    return true;
}

bool JITEngine::addModule(Module* irModule) {
    // First generate LLVM IR from our IR
    LLVMBackend backend;
    if (!backend.generateModule(irModule)) {
        return false;
    }
    
    // Get the LLVM module and transfer ownership to JIT
    // This is simplified - real implementation needs module transfer
    auto llvmModule = std::make_unique<llvm::Module>("jit_module", *context_);
    
    auto tsm = llvm::orc::ThreadSafeModule(std::move(llvmModule), 
                                           std::make_unique<llvm::LLVMContext>());
    
    if (auto err = jit_->addIRModule(std::move(tsm))) {
        llvm::handleAllErrors(std::move(err), [](const llvm::ErrorInfoBase& ei) {
            std::cerr << "JIT Error: " << ei.message() << "\n";
        });
        return false;
    }
    
    return true;
}

bool JITEngine::addLLVMModule(std::unique_ptr<llvm::Module> module) {
    auto tsm = llvm::orc::ThreadSafeModule(std::move(module), 
                                           std::make_unique<llvm::LLVMContext>());
    
    if (auto err = jit_->addIRModule(std::move(tsm))) {
        llvm::handleAllErrors(std::move(err), [](const llvm::ErrorInfoBase& ei) {
            std::cerr << "JIT Error: " << ei.message() << "\n";
        });
        return false;
    }
    
    return true;
}

JITResult JITEngine::lookup(const std::string& name) {
    auto symbol = jit_->lookup(name);
    if (!symbol) {
        return JITResult("Symbol not found: " + name);
    }
    
    auto addr = symbol->getAddress();
    return JITResult(reinterpret_cast<void*>(addr));
}

void JITEngine::beginSession() {
    if (!sessionActive_) {
        // Create new dylib for this session
        auto& dylib = jit_->getMainJITDylib();
        currentDylib_ = &dylib;
        sessionActive_ = true;
    }
}

void JITEngine::endSession() {
    if (sessionActive_) {
        // Clear the session's dylib
        if (currentDylib_) {
            // Remove symbols from current dylib
        }
        sessionActive_ = false;
        currentDylib_ = nullptr;
    }
}

bool JITEngine::isSessionActive() const {
    return sessionActive_;
}

} // namespace compiler
