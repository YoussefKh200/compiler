#ifndef AI_OPTIMIZER_H
#define AI_OPTIMIZER_H

#include "../ir/ir.h"
#include "../optimizer/optimizer.h"
#include <vector>
#include <string>
#include <memory>
#include <future>

namespace compiler {

// Optimization features extracted from IR
struct IRFeatures {
    // Function characteristics
    size_t basicBlockCount;
    size_t instructionCount;
    size_t loopCount;
    size_t callCount;
    size_t memoryOpCount;
    size_t branchCount;
    
    // Complexity metrics
    double cyclomaticComplexity;
    double averageBlockSize;
    double instructionDensity;
    
    // Pattern indicators
    bool hasRecursion;
    bool hasIndirectCalls;
    bool hasExceptionHandling;
    bool hasVectorizableLoops;
    bool hasMemoryAliasing;
    
    // Historical performance (if available)
    double previousExecutionTime;
    size_t previousCacheMisses;
    
    std::vector<double> toVector() const;
};

// Optimization recommendation
struct OptimizationRecommendation {
    std::string passName;
    double predictedBenefit; // 0.0 to 1.0
    int priority; // 1-10
    std::string rationale;
    std::unordered_map<std::string, double> parameters; // Pass-specific params
};

class AIOptimizer {
public:
    AIOptimizer();
    ~AIOptimizer();
    
    // Initialize AI model
    bool initialize(const std::string& modelPath = "");
    
    // Feature extraction
    IRFeatures extractFeatures(Function* func);
    
    // Optimization ranking
    std::vector<OptimizationRecommendation> rankOptimizations(Function* func);
    
    // Apply recommended optimizations
    bool optimizeWithAI(Function* func, PassManager& pm);
    
    // Learning from results
    void reportExecutionTime(const std::string& functionName, double timeMs);
    void reportOptimizationResult(const std::string& passName, bool improved);
    
    // Model management
    bool trainModel(const std::vector<IRFeatures>& features,
                    const std::vector<double>& performance);
    bool saveModel(const std::string& path);
    bool loadModel(const std::string& path);
    
    // Python bridge for advanced ML
    void setPythonBridge(std::shared_ptr<class PythonBridge> bridge);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// Feature extractor implementation
class FeatureExtractor {
public:
    IRFeatures extract(Function* func);
    
private:
    void analyzeControlFlow(Function* func, IRFeatures& features);
    void analyzeDataFlow(Function* func, IRFeatures& features);
    void analyzeLoops(Function* func, IRFeatures& features);
    size_t countLoops(Function* func);
    double calculateCyclomaticComplexity(Function* func);
};

} // namespace compiler

#endif // AI_OPTIMIZER_H
