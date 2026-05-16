# Compiler

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![License](https://img.shields.io/badge/license-MIT-blue.svg)]()
[![LLVM](https://img.shields.io/badge/LLVM-17.x-yellow.svg)]()

A modern multi-pass compiler with AI-assisted optimization, featuring full LLVM backend integration and JIT execution support.


## Features

- 🚀 **Multi-pass architecture** with clear separation of concerns
- 🧠 **AI-powered optimization** using neural networks for pass ranking
- ⚡ **JIT compilation** for immediate execution and REPL support
- 🔧 **LLVM backend** with target-specific optimizations
- 📊 **SSA-based IR** for efficient data-flow analysis
- 🎯 **Full type system** with type inference

## Compilation Pipeline

| Stage | Component | Input → Output |
|-------|-----------|----------------|
| 1 | Lexical Analysis | Source code → Token stream |
| 2 | Parsing | Token stream → Abstract Syntax Tree (AST) |
| 3 | Semantic Analysis | AST → Typed AST with symbol resolution |
| 4 | IR Generation | Typed AST → Static Single Assignment (SSA) IR |
| 5 | Optimization | SSA IR → Optimized SSA IR |
| 6 | Code Generation | SSA IR → LLVM IR → Machine code |

## Project Structure

compiler/
├── src/
│ ├── lexer/ # Tokenization & lexical analysis
│ ├── parser/ # Recursive descent parser
│ ├── semantic/ # Type checking & symbol resolution
│ ├── ir/ # SSA intermediate representation
│ ├── optimizer/ # Pass manager & optimizations
│ ├── backend/ # LLVM code generation
│ ├── jit/ # LLVM ORC JIT engine
│ └── ai_optimizer/ # ML-based optimization
├── ai_module/ # Python neural network server
├── tests/ # Test suites
└── examples/ # Sample programs



## Key Components

### Lexer (`src/lexer/`)
- Tokenizes source code into a stream of tokens
- Handles identifiers, literals, operators, and keywords
- Provides lookahead for parser
- Error recovery with synchronization

### Parser (`src/parser/`)
- Recursive descent parser with operator precedence
- Builds AST with proper source locations
- Handles expressions, statements, and declarations
- Error recovery to continue parsing after errors

### Semantic Analyzer (`src/semantic/`)
- Type checking with full type system
- Symbol table with scope management
- Type inference for local variables
- Control flow validation

### IR (`src/ir/`)
- SSA-based intermediate representation
- Basic blocks with instructions
- Phi nodes for SSA form
- Type system mapped to LLVM types

### Optimizer (`src/optimizer/`)
- Pass manager with configurable optimization levels (`-O0` to `-O3`)
- Constant folding
- Dead code elimination
- Loop invariant code motion
- Function inlining

### Backend (`src/backend/`)
- LLVM integration for code generation
- Target-specific optimizations
- Object file, assembly, and executable output

### JIT (`src/jit/`)
- LLVM ORC JIT for immediate execution
- Function lookup and execution
- REPL support

### AI Optimizer (`src/ai_optimizer/`, `ai_module/`)
- Feature extraction from IR
- Neural network for pass ranking
- Python server for model inference
- Feedback loop for continuous learning

## Overview

![Compiler Overview](Capture%20d'écran%202026-05-16%20131827.png)

## Quick Start

### Prerequisites

- C++17 compatible compiler
- LLVM 17.x or later
- Python 3.9+ (for AI module)
- CMake 3.15+

### Building

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)

USAGE:
# Compile a source file
./compiler input.src -o output

# Run with optimizations
./compiler input.src -O2 -o output

# JIT execute directly
./compiler input.src --jit

# Interactive REPL
./compiler --repl

# Enable AI optimization
./compiler input.src --ai-optimize -O2


Optimization Levels
Level	Description
-O0	No optimizations, fast compilation
-O1	Basic optimizations (constant folding, DCE)
-O2	Standard optimizations + loop optimizations
-O3	Aggressive optimizations + inlining
--ai-optimize	ML-guided pass selection (with any -O level)


Examples
See the examples/ directory for sample programs:

// factorial.src
function factorial(n: int) -> int {
    if n <= 1 return 1;
    return n * factorial(n - 1);
}

print(factorial(5));  // Output: 120

