#include "ai_optimizer.h"
#include "../utils/timer.h"
#include <curl/curl.h>
#include <json/json.h>
#include <sstream>
#include <fstream>

namespace compiler {

// HTTP callback for libcurl
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

class AIOptimizer::Impl {
public:
    std::string serverUrl = "http://localhost:5000";
    bool usePythonBridge = false;
    std::vector<IRFeatures> history;
    
    bool sendFeatures(const IRFeatures& features, std::string& response) {
        CURL* curl = curl_easy_init();
        if (!curl) return false;
        
        Json::Value root;
        root["features"]["basicBlockCount"] = static_cast<Json::UInt64>(features.basicBlockCount);
        root["features"]["instructionCount"] = static_cast<Json::UInt64>(features.instructionCount);
        root["features"]["loopCount"] = static_cast<Json::UInt64>(features.loopCount);
        root["features"]["callCount"] = static_cast<Json::UInt64>(features.callCount);
        root["features"]["memoryOpCount"] = static_cast<Json::UInt64>(features.memoryOpCount);
        root["features"]["branchCount"] = static_cast<Json::UInt64>(features.branchCount);
        root["features"]["cyclomaticComplexity"] = features.cyclomaticComplexity;
        root["features"]["averageBlockSize"] = features.averageBlockSize;
        root["features"]["instructionDensity"] = features.instructionDensity;
        root["features"]["hasRecursion"] = features.hasRecursion;
        root["features"]["hasIndirectCalls"] = features.hasIndirectCalls;
        root["features"]["hasExceptionHandling"] = features.hasExceptionHandling;
        root["features"]["hasVectorizableLoops"] = features.hasVectorizableLoops;
        root["features"]["hasMemoryAliasing"] = features.hasMemoryAliasing;
        root["features"]["previousExecutionTime"] = features.previousExecutionTime;
        
        Json::StreamWriterBuilder builder;
        std::string jsonData = Json::writeString(builder, root);
        
        std::string readBuffer;
        
        curl_easy_setopt(curl, CURLOPT_URL, (serverUrl + "/analyze").c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        CURLcode res = curl_easy_perform(curl);
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        if (res != CURLE_OK) {
            return false;
        }
        
        response = readBuffer;
        return true;
    }
    
    bool sendFeedback(const std::string& funcHash, const IRFeatures& features,
                      const std::vector<std::string>& passes, double execTime) {
        CURL* curl = curl_easy_init();
        if (!curl) return false;
        
        Json::Value root;
        root["function_hash"] = funcHash;
        root["features"]["basicBlockCount"] = static_cast<Json::UInt64>(features.basicBlockCount);
        // ... other features ...
        Json::Value passesArray(Json::arrayValue);
        for (const auto& p : passes) {
            passesArray.append(p);
        }
        root["passes"] = passesArray;
        root["execution_time"] = execTime;
        
        Json::StreamWriterBuilder builder;
        std::string jsonData = Json::writeString(builder, root);
        
        curl_easy_setopt(curl, CURLOPT_URL, (serverUrl + "/feedback").c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        CURLcode res = curl_easy_perform(curl);
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        return res == CURLE_OK;
    }
};

AIOptimizer::AIOptimizer() : impl_(std::make_unique<Impl>()) {}

AIOptimizer::~AIOptimizer() = default;

bool AIOptimizer::initialize(const std::string& modelPath) {
    // Check if Python server is running
    std::string response;
    if (!impl_->sendFeatures(IRFeatures{}, response)) {
        std::cerr << "Warning: AI server not available at " << impl_->serverUrl << "\n";
        std::cerr << "Start with: python3 ai_module/model/optimization_ranker.py\n";
        return false;
    }
    
    return true;
}

IRFeatures AIOptimizer::extractFeatures(Function* func) {
    FeatureExtractor extractor;
    return extractor.extract(func);
}

std::vector<OptimizationRecommendation> AIOptimizer::rankOptimizations(Function* func) {
    std::vector<OptimizationRecommendation> recommendations;
    
    auto features = extractFeatures(func);
    std::string response;
    
    if (!impl_->sendFeatures(features, response)) {
        // Fallback to heuristic-based ranking
        recommendations.push_back({"ConstantFolding", 0.8, 9, "Always beneficial", {}});
        recommendations.push_back({"DCE", 0.7, 8, "Removes dead code", {}});
        if (features.loopCount > 0) {
            recommendations.push_back({"LICM", 0.6, 7, "Loops detected", {}});
        }
        return recommendations;
    }
    
    // Parse JSON response
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errors;
    
    std::istringstream responseStream(response);
    if (!Json::parseFromStream(builder, responseStream, &root, &errors)) {
        return recommendations;
    }
    
    const auto& recs = root["recommendations"];
    for (const auto& rec : recs) {
        OptimizationRecommendation recommendation;
        recommendation.passName = rec["pass"].asString();
        recommendation.predictedBenefit = rec["benefit"].asDouble();
        recommendation.priority = static_cast<int>(rec["benefit"].asDouble() * 10);
        recommendation.rationale = "AI predicted benefit: " + 
                                   std::to_string(recommendation.predictedBenefit);
        
        const auto& params = rec["parameters"];
        for (const auto& member : params.getMemberNames()) {
            recommendation.parameters[member] = params[member].asDouble();
        }
        
        recommendations.push_back(recommendation);
    }
    
    return recommendations;
}

bool AIOptimizer::optimizeWithAI(Function* func, PassManager& pm) {
    auto recommendations = rankOptimizations(func);
    
    std::vector<std::string> appliedPasses;
    
    for (const auto& rec : recommendations) {
        if (rec.predictedBenefit < 0.5) continue;
        
        // Create pass based on recommendation
        std::unique_ptr<OptimizationPass> pass;
        
        if (rec.passName == "ConstantFolding") {
            pass = std::make_unique<ConstantFoldingPass>();
        } else if (rec.passName == "DCE") {
            pass = std::make_unique<DeadCodeEliminationPass>();
        } else if (rec.passName == "LICM") {
            pass = std::make_unique<LoopInvariantCodeMotionPass>();
        } else if (rec.passName == "Inlining") {
            auto inlinePass = std::make_unique<InlineExpansionPass>();
            if (rec.parameters.count("threshold")) {
                inlinePass->setThreshold(static_cast<size_t>(rec.parameters.at("threshold")));
            }
            pass = std::move(inlinePass);
        }
        
        if (pass) {
            pm.addPass(std::move(pass));
            appliedPasses.push_back(rec.passName);
        }
    }
    
    // Store for feedback
    impl_->history.push_back(extractFeatures(func));
    
    return !appliedPasses.empty();
}

void AIOptimizer::reportExecutionTime(const std::string& functionName, double timeMs) {
    // Send feedback to improve model
    if (!impl_->history.empty()) {
        impl_->sendFeedback(functionName, impl_->history.back(), {}, timeMs);
    }
}

void AIOptimizer::reportOptimizationResult(const std::string& passName, bool improved) {
    // Store result for learning
}

bool AIOptimizer::trainModel(const std::vector<IRFeatures>& features,
                              const std::vector<double>& performance) {
    // Trigger training on Python server
    return false;
}

bool AIOptimizer::saveModel(const std::string& path) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;
    
    Json::Value root;
    root["path"] = path;
    
    Json::StreamWriterBuilder builder;
    std::string jsonData = Json::writeString(builder, root);
    
    curl_easy_setopt(curl, CURLOPT_URL, (impl_->serverUrl + "/save").c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());
    
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    CURLcode res = curl_easy_perform(curl);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    return res == CURLE_OK;
}

bool AIOptimizer::loadModel(const std::string& path) {
    // Model loaded on server side
    return true;
}

void AIOptimizer::setPythonBridge(std::shared_ptr<PythonBridge> bridge) {
    // Set up direct Python integration if needed
}

// FeatureExtractor implementation
IRFeatures FeatureExtractor::extract(Function* func) {
    IRFeatures features;
    
    features.basicBlockCount = func->basicBlocks.size();
    
    analyzeControlFlow(func, features);
    analyzeDataFlow(func, features);
    analyzeLoops(func, features);
    
    return features;
}

void FeatureExtractor::analyzeControlFlow(Function* func, IRFeatures& features) {
    size_t branches = 0;
    for (const auto& bb : func->basicBlocks) {
        branches += bb->successors.size();
    }
    features.branchCount = branches;
    
    // Calculate cyclomatic complexity: E - N + 2P
    size_t edges = 0;
    size_t nodes = func->basicBlocks.size();
    for (const auto& bb : func->basicBlocks) {
        edges += bb->successors.size();
    }
    features.cyclomaticComplexity = static_cast<double>(edges) - nodes + 2;
}

void FeatureExtractor::analyzeDataFlow(Function* func, IRFeatures& features) {
    size_t instCount = 0;
    size_t memOps = 0;
    size_t calls = 0;
    
    for (const auto& bb : func->basicBlocks) {
        for (const auto& inst : bb->instructions) {
            instCount++;
            
            switch (inst->opcode) {
                case Opcode::Load:
                case Opcode::Store:
                case Opcode::Alloca:
                    memOps++;
                    break;
                case Opcode::Call:
                    calls++;
                    break;
                default:
                    break;
            }
        }
    }
    
    features.instructionCount = instCount;
    features.memoryOpCount = memOps;
    features.callCount = calls;
    features.averageBlockSize = instCount / static_cast<double>(func->basicBlocks.size());
    features.instructionDensity = instCount / static_cast<double>(func->basicBlocks.size());
}

void FeatureExtractor::analyzeLoops(Function* func, IRFeatures& features) {
    features.loopCount = countLoops(func);
    features.hasRecursion = false; // Would need call graph analysis
    features.hasIndirectCalls = false;
    features.hasExceptionHandling = false;
    features.hasVectorizableLoops = false;
    features.hasMemoryAliasing = false;
}

size_t FeatureExtractor::countLoops(Function* func) {
    // Simple heuristic: backedges in CFG
    size_t loops = 0;
    for (const auto& bb : func->basicBlocks) {
        for (auto* succ : bb->successors) {
            // If successor has lower index (simplified), it's a backedge
            // Real implementation needs proper dominator analysis
        }
    }
    return loops;
}

double FeatureExtractor::calculateCyclomaticComplexity(Function* func) {
    return 0.0; // Calculated in analyzeControlFlow
}

} // namespace compiler
