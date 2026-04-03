#ifndef JIT_ENGINE_H
#define JIT_ENGINE_H

#include "../backend/llvm_backend.h"
#include <memory>
#include <functional>
#include <string>

// LLVM ORC JIT forward declarations
namespace llvm {
    namespace orc {
        class LLJIT;
        class JITDylib;
    }
    class Error;
}

namespace compiler {

// JIT compilation result
struct JITResult {
    bool success;
    std::string errorMessage;
    void* functionPointer;
    
    explicit JITResult(void* ptr = nullptr) 
        : success(ptr != nullptr), functionPointer(ptr) {}
    explicit JITResult(const std::string& error) 
        : success(false), errorMessage(error), functionPointer(nullptr) {}
};

class JITEngine {
public:
    JITEngine();
    ~JITEngine();
    
    // Initialize JIT
    bool initialize();
    
    // Add IR module to JIT
    bool addModule(Module* irModule);
    bool addLLVMModule(std::unique_ptr<llvm::Module> module);
    
    // Lookup function
    JITResult lookup(const std::string& name);
    
    // Execute function (templated for different signatures)
    template<typename Ret, typename... Args>
    Ret execute(const std::string& name, Args... args) {
        auto result = lookup(name);
        if (!result.success) {
            throw std::runtime_error("Function not found: " + name);
        }
        
        using FuncType = Ret(*)(Args...);
        auto* func = reinterpret_cast<FuncType>(result.functionPointer);
        return func(args...);
    }
    
    // REPL support
    void beginSession();
    void endSession();
    bool isSessionActive() const;
    
    // High-level API
    template<typename Ret = void, typename... Args>
    Ret compileAndRun(const std::string& source, Args... args) {
        // 1. Parse source
        // 2. Generate IR
        // 3. Add to JIT
        // 4. Execute main function
        // Implementation in .cpp
        return Ret();
    }

private:
    std::unique_ptr<llvm::orc::LLJIT> jit_;
    std::unique_ptr<llvm::LLVMContext> context_;
    bool sessionActive_;
    
    // Session management
    llvm::orc::JITDylib* currentDylib_;
};

} // namespace compiler

#endif // JIT_ENGINE_H
