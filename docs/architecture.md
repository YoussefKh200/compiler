# Compiler Architecture

## Overview

This compiler follows a modern multi-pass architecture with the following stages:

1. **Lexical Analysis** - Source code → Token stream
2. **Parsing** - Token stream → Abstract Syntax Tree (AST)
3. **Semantic Analysis** - AST → Typed AST with symbol resolution
4. **IR Generation** - Typed AST → Static Single Assignment (SSA) IR
5. **Optimization** - SSA IR → Optimized SSA IR
6. **Code Generation** - SSA IR → LLVM IR → Machine code

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
- Pass manager with configurable levels
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

## Data Flow


## Memory Management

- Arena allocator (`MemoryPool`) for AST nodes
- RAII for IR values
- LLVM manages its own memory
