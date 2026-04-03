#include "lexer/lexer.h"
#include "parser/parser.h"
#include "semantic/semantic_analyzer.h"
#include "ir/ir_builder.h"
#include "ir/ssa_ir.h"
#include "optimizer/optimizer.h"
#include "backend/llvm_backend.h"
#include "jit/jit_engine.h"
#include "ai_optimizer/ai_optimizer.h"
#include "diagnostics/error_reporter.h"
#include "utils/memory_pool.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>

using namespace compiler;

enum class OutputMode {
    AST,
    IR,
    LLVM_IR,
    Assembly,
    Object,
    Executable,
    JIT
};

struct CompilerOptions {
    std::string inputFile;
    std::string outputFile;
    OutputMode mode = OutputMode::Executable;
    OptLevel optLevel = OptLevel::O2;
    bool enableAI = false;
    std::string targetTriple;
    bool verbose = false;
    std::vector<std::string> libraryPaths;
    std::vector<std::string> libraries;
};

void printUsage(const char* program) {
    std::cout << "Usage: " << program << " [options] <input_file>\n"
              << "Options:\n"
              << "  -o <file>       Output file\n"
              << "  -O0, -O1, -O2, -O3  Optimization level (default: O2)\n"
              << "  -Os             Optimize for size\n"
              << "  --emit-ast      Print AST and exit\n"
              << "  --emit-ir       Print internal IR and exit\n"
              << "  --emit-llvm     Print LLVM IR and exit\n"
              << "  --emit-asm      Emit assembly\n"
              << "  --emit-obj      Emit object file\n"
              << "  --jit           Execute using JIT\n"
              << "  --ai            Enable AI optimization\n"
              << "  --target <triple> Set target triple\n"
              << "  -v              Verbose output\n"
              << "  -L <path>       Library search path\n"
              << "  -l <lib>        Link library\n"
              << "  -h, --help      Show this help\n";
}

CompilerOptions parseArgs(int argc, char* argv[]) {
    CompilerOptions opts;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            std::exit(0);
        } else if (arg == "-o" && i + 1 < argc) {
            opts.outputFile = argv[++i];
        } else if (arg == "-O0") {
            opts.optLevel = OptLevel::O0;
        } else if (arg == "-O1") {
            opts.optLevel = OptLevel::O1;
        } else if (arg == "-O2") {
            opts.optLevel = OptLevel::O2;
        } else if (arg == "-O3") {
            opts.optLevel = OptLevel::O3;
        } else if (arg == "-Os") {
            opts.optLevel = OptLevel::Os;
        } else if (arg == "--emit-ast") {
            opts.mode = OutputMode::AST;
        } else if (arg == "--emit-ir") {
            opts.mode = OutputMode::IR;
        } else if (arg == "--emit-llvm") {
            opts.mode = OutputMode::LLVM_IR;
        } else if (arg == "--emit-asm") {
            opts.mode = OutputMode::Assembly;
        } else if (arg == "--emit-obj") {
            opts.mode = OutputMode::Object;
        } else if (arg == "--jit") {
            opts.mode = OutputMode::JIT;
        } else if (arg == "--ai") {
            opts.enableAI = true;
        } else if (arg == "--target" && i + 1 < argc) {
            opts.targetTriple = argv[++i];
        } else if (arg == "-v") {
            opts.verbose = true;
        } else if (arg == "-L" && i + 1 < argc) {
            opts.libraryPaths.push_back(argv[++i]);
        } else if (arg == "-l" && i + 1 < argc) {
            opts.libraries.push_back(argv[++i]);
        } else if (arg[0] != '-') {
            opts.inputFile = arg;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage(argv[0]);
            std::exit(1);
        }
    }
    
    if (opts.inputFile.empty()) {
        std::cerr << "Error: No input file specified\n";
        printUsage(argv[0]);
        std::exit(1);
    }
    
    if (opts.outputFile.empty()) {
        if (opts.mode == OutputMode::Executable) {
            opts.outputFile = "a.out";
        } else if (opts.mode == OutputMode::Object) {
            opts.outputFile = "a.o";
        } else if (opts.mode == OutputMode::Assembly) {
            opts.outputFile = "a.s";
        }
    }
    
    return opts;
}

int main(int argc, char* argv[]) {
    auto opts = parseArgs(argc, argv);
    
    // Read input file
    std::ifstream file(opts.inputFile);
    if (!file) {
        std::cerr << "Error: Cannot open input file: " << opts.inputFile << "\n";
        return 1;
    }
    
    if (opts.verbose) {
        std::cerr << "Compiling: " << opts.inputFile << "\n";
    }
    
    // Phase 1: Lexing
    MemoryPool pool;
    Lexer lexer(file, opts.inputFile);
    auto tokens = lexer.tokenize();
    
    if (ErrorReporter::instance().hasErrors()) {
        ErrorReporter::instance().printAll();
        return 1;
    }
    
    if (opts.verbose) {
        std::cerr << "Lexed " << tokens.size() << " tokens\n";
    }
    
    // Phase 2: Parsing
    Parser parser(lexer, pool);
    auto ast = parser.parseProgram();
    
    if (parser.hadError() || ErrorReporter::instance().hasErrors()) {
        ErrorReporter::instance().printAll();
        return 1;
    }
    
    if (opts.mode == OutputMode::AST) {
        ASTPrinter printer;
        ast->accept(printer);
        std::cout << printer.print(*ast);
        return 0;
    }
    
    if (opts.verbose) {
        std::cerr << "Parsed AST successfully\n";
    }
    
    // Phase 3: Semantic Analysis
    SemanticAnalyzer sema;
    if (!sema.analyze(*ast)) {
        ErrorReporter::instance().printAll();
        return 1;
    }
    
    if (opts.verbose) {
        std::cerr << "Semantic analysis passed\n";
    }
    
    // Phase 4: IR Generation
    Module irModule(opts.inputFile);
    // IRGenVisitor irGen(&irModule); // Implementation would convert AST to IR
    // ast->accept(irGen);
    
    // Phase 5: SSA Construction
    // for (auto& func : irModule.functions) {
    //     SSAConstructor ssa;
    //     ssa.construct(func.get());
    // }
    
    if (opts.mode == OutputMode::IR) {
        std::cout << irModule.toString();
        return 0;
    }
    
    // Phase 6: Optimization
    if (opts.optLevel != OptLevel::O0) {
        PassManager pm;
        pm.setOptLevel(opts.optLevel);
        
        // Add standard passes
        pm.addPass(std::make_unique<ConstantFoldingPass>());
        pm.addPass(std::make_unique<DeadCodeEliminationPass>());
        
        if (opts.optLevel >= OptLevel::O2) {
            pm.addPass(std::make_unique<LoopInvariantCodeMotionPass>());
            pm.addPass(std::make_unique<InlineExpansionPass>());
        }
        
        if (opts.enableAI) {
            // AI-guided optimization
            AIOptimizer aiOpt;
            if (aiOpt.initialize()) {
                for (auto& func : irModule.functions) {
                    aiOpt.optimizeWithAI(func.get(), pm);
                }
            }
        }
        
        pm.runOnModule(&irModule);
    }
    
    // Phase 7: Code Generation
    LLVMBackend::initializeTargets();
    LLVMBackend backend;
    
    if (!opts.targetTriple.empty()) {
        backend.setTargetTriple(opts.targetTriple);
    }
    
    if (!backend.generateModule(&irModule)) {
        std::cerr << "Error: Code generation failed\n";
        return 1;
    }
    
    // Run LLVM optimizations
    backend.runLLVMOptimizations(opts.optLevel);
    
    // Output generation
    switch (opts.mode) {
        case OutputMode::LLVM_IR:
            if (!backend.emitLLVMIR(opts.outputFile)) {
                std::cerr << "Error: Failed to emit LLVM IR\n";
                return 1;
            }
            break;
            
        case OutputMode::Assembly:
            if (!backend.emitAssembly(opts.outputFile)) {
                std::cerr << "Error: Failed to emit assembly\n";
                return 1;
            }
            break;
            
        case OutputMode::Object:
            if (!backend.emitObjectFile(opts.outputFile)) {
                std::cerr << "Error: Failed to emit object file\n";
                return 1;
            }
            break;
            
        case OutputMode::JIT: {
            JITEngine jit;
            if (!jit.initialize()) {
                std::cerr << "Error: Failed to initialize JIT\n";
                return 1;
            }
            
            // Add module and execute
            // jit.addModule(&irModule);
            // jit.execute<int>("main");
            break;
        }
            
        case OutputMode::Executable:
            // Emit object and link
            if (!backend.emitObjectFile("temp.o")) {
                std::cerr << "Error: Failed to emit object file\n";
                return 1;
            }
            
            // Link using system linker
            std::stringstream linkCmd;
            linkCmd << "clang temp.o -o " << opts.outputFile;
            for (const auto& path : opts.libraryPaths) {
                linkCmd << " -L" << path;
            }
            for (const auto& lib : opts.libraries) {
                linkCmd << " -l" << lib;
            }
            
            if (system(linkCmd.str().c_str()) != 0) {
                std::cerr << "Error: Linking failed\n";
                return 1;
            }
            
            std::remove("temp.o");
            break;
            
        default:
            break;
    }
    
    if (opts.verbose) {
        std::cerr << "Output written to: " << opts.outputFile << "\n";
    }
    
    return 0;
}
