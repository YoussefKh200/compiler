#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "../ir/ir.h"
#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace compiler {

// Optimization level
enum class OptLevel {
    O0, // None
    O1, // Basic
    O2, // Standard
    O3, // Aggressive
    Os, // Size
    Oz  // Aggressive size
};

// Pass base class
class OptimizationPass {
public:
    virtual ~OptimizationPass() = default;
    virtual std::string getName() const = 0;
    virtual bool runOnFunction(Function* func) = 0;
    virtual bool runOnModule(Module* mod) { return false; }
};

// Pass manager
class PassManager {
public:
    void addPass(std::unique_ptr<OptimizationPass> pass);
    void runOnFunction(Function* func);
    void runOnModule(Module* mod);
    void clearPasses();
    
    void setOptLevel(OptLevel level) { optLevel_ = level; }
    OptLevel getOptLevel() const { return optLevel_; }
    
    // Statistics
    struct Stats {
        std::string passName;
        bool changed;
        double timeMs;
    };
    const std::vector<Stats>& getStats() const { return stats_; }
    
private:
    std::vector<std::unique_ptr<OptimizationPass>> passes_;
    OptLevel optLevel_ = OptLevel::O2;
    std::vector<Stats> stats_;
};

// Individual passes
class ConstantFoldingPass : public OptimizationPass {
public:
    std::string getName() const override { return "ConstantFolding"; }
    bool runOnFunction(Function* func) override;
};

class DeadCodeEliminationPass : public OptimizationPass {
public:
    std::string getName() const override { return "DCE"; }
    bool runOnFunction(Function* func) override;
};

class LoopInvariantCodeMotionPass : public OptimizationPass {
public:
    std::string getName() const override { return "LICM"; }
    bool runOnFunction(Function* func) override;
};

class InlineExpansionPass : public OptimizationPass {
public:
    std::string getName() const override { return "Inlining"; }
    bool runOnFunction(Function* func) override;
    void setThreshold(size_t threshold) { threshold_ = threshold; }
    
private:
    size_t threshold_ = 100; // Max instruction count to inline
};

class StrengthReductionPass : public OptimizationPass {
public:
    std::string getName() const override { return "StrengthReduction"; }
    bool runOnFunction(Function* func) override;
};

class TailCallEliminationPass : public OptimizationPass {
public:
    std::string getName() const override { return "TailCallElim"; }
    bool runOnFunction(Function* func) override;
};

// Analysis passes
class DominatorTreePass : public OptimizationPass {
public:
    std::string getName() const override { return "DominatorTree"; }
    bool runOnFunction(Function* func) override;
    // Store results for other passes
    static std::unordered_map<Function*, class DominatorTree*> results;
};

class LoopAnalysisPass : public OptimizationPass {
public:
    std::string getName() const override { return "LoopAnalysis"; }
    bool runOnFunction(Function* func) override;
};

} // namespace compiler

#endif // OPTIMIZER_H
