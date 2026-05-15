#include "optimizer.h"
#include "../utils/timer.h"
#include <algorithm>

namespace compiler {

void PassManager::addPass(std::unique_ptr<OptimizationPass> pass) {
    passes_.push_back(std::move(pass));
}

void PassManager::runOnFunction(Function* func) {
    for (auto& pass : passes_) {
        Timer timer;
        timer.start();
        
        bool changed = pass->runOnFunction(func);
        
        timer.stop();
        
        Stats stat;
        stat.passName = pass->getName();
        stat.changed = changed;
        stat.timeMs = timer.elapsedMs();
        stats_.push_back(stat);
    }
}

void PassManager::runOnModule(Module* mod) {
    for (auto& pass : passes_) {
        Timer timer;
        timer.start();
        
        bool changed = pass->runOnModule(mod);
        
        for (auto& func : mod->functions) {
            if (pass->runOnFunction(func.get())) {
                changed = true;
            }
        }
        
        timer.stop();
        
        Stats stat;
        stat.passName = pass->getName();
        stat.changed = changed;
        stat.timeMs = timer.elapsedMs();
        stats_.push_back(stat);
    }
}

void PassManager::clearPasses() {
    passes_.clear();
    stats_.clear();
}

// Constant Folding Implementation
bool ConstantFoldingPass::runOnFunction(Function* func) {
    bool changed = false;
    
    for (auto& bb : func->basicBlocks) {
        for (auto& inst : bb->instructions) {
            if (foldInstruction(inst.get())) {
                changed = true;
            }
        }
    }
    
    return changed;
}

bool ConstantFoldingPass::foldInstruction(Instruction* inst) {
    // Check if all operands are constants
    std::vector<Constant*> constants;
    for (auto* op : inst->operands) {
        if (auto* c = dynamic_cast<Constant*>(op)) {
            constants.push_back(c);
        } else {
            return false;
        }
    }
    
    if (constants.empty()) return false;
    
    Constant* result = nullptr;
    
    switch (inst->opcode) {
        case Opcode::Add:
        case Opcode::Sub:
        case Opcode::Mul:
        case Opcode::SDiv:
        case Opcode::SRem:
        case Opcode::FAdd:
        case Opcode::FSub:
        case Opcode::FMul:
        case Opcode::FDiv:
            if (constants.size() >= 2) {
                result = evaluateBinaryOp(inst->opcode, constants[0], constants[1]);
            }
            break;
            
        case Opcode::ICmpEq:
        case Opcode::ICmpNe:
        case Opcode::ICmpSlt:
        case Opcode::ICmpSle:
        case Opcode::ICmpSgt:
        case Opcode::ICmpSge:
            if (constants.size() >= 2) {
                result = evaluateBinaryOp(inst->opcode, constants[0], constants[1]);
            }
            break;
            
        case Opcode::Trunc:
        case Opcode::ZExt:
        case Opcode::SExt:
        case Opcode::FPToSI:
        case Opcode::SIToFP:
            if (constants.size() >= 1) {
                result = evaluateCast(inst->opcode, constants[0], inst->type);
            }
            break;
            
        default:
            break;
    }
    
    if (result) {
        // Replace instruction with constant
        inst->opcode = Opcode::Ret; // Mark for deletion
        // In real implementation, replace all uses with result
        return true;
    }
    
    return false;
}

Constant* ConstantFoldingPass::evaluateBinaryOp(Opcode op, Constant* lhs, Constant* rhs) {
    // Integer operations
    if (std::holds_alternative<int64_t>(lhs->data) && std::holds_alternative<int64_t>(rhs->data)) {
        int64_t l = std::get<int64_t>(lhs->data);
        int64_t r = std::get<int64_t>(rhs->data);
        int64_t result = 0;
        
        switch (op) {
            case Opcode::Add: result = l + r; break;
            case Opcode::Sub: result = l - r; break;
            case Opcode::Mul: result = l * r; break;
            case Opcode::SDiv: result = r != 0 ? l / r : 0; break;
            case Opcode::SRem: result = r != 0 ? l % r : 0; break;
            case Opcode::ICmpEq: return new Constant(IRType::I1, l == r, 0);
            case Opcode::ICmpNe: return new Constant(IRType::I1, l != r, 0);
            case Opcode::ICmpSlt: return new Constant(IRType::I1, l < r, 0);
            case Opcode::ICmpSle: return new Constant(IRType::I1, l <= r, 0);
            case Opcode::ICmpSgt: return new Constant(IRType::I1, l > r, 0);
            case Opcode::ICmpSge: return new Constant(IRType::I1, l >= r, 0);
            default: return nullptr;
        }
        
        return new Constant(lhs->type, result, 0);
    }
    
    // Float operations
    if (std::holds_alternative<double>(lhs->data) && std::holds_alternative<double>(rhs->data)) {
        double l = std::get<double>(lhs->data);
        double r = std::get<double>(rhs->data);
        double result = 0;
        
        switch (op) {
            case Opcode::FAdd: result = l + r; break;
            case Opcode::FSub: result = l - r; break;
            case Opcode::FMul: result = l * r; break;
            case Opcode::FDiv: result = r != 0 ? l / r : 0; break;
            default: return nullptr;
        }
        
        return new Constant(lhs->type, result, 0);
    }
    
    return nullptr;
}

Constant* ConstantFoldingPass::evaluateCast(Opcode op, Constant* val, IRType destType) {
    if (std::holds_alternative<int64_t>(val->data)) {
        int64_t v = std::get<int64_t>(val->data);
        
        switch (op) {
            case Opcode::Trunc:
                switch (destType) {
                    case IRType::I8: return new Constant(destType, static_cast<int8_t>(v), 0);
                    case IRType::I16: return new Constant(destType, static_cast<int16_t>(v), 0);
                    case IRType::I32: return new Constant(destType, static_cast<int32_t>(v), 0);
                    default: break;
                }
                break;
            case Opcode::ZExt:
            case Opcode::SExt:
                return new Constant(destType, v, 0);
            case Opcode::SIToFP:
                return new Constant(destType, static_cast<double>(v), 0);
            default:
                break;
        }
    }
    
    if (std::holds_alternative<double>(val->data)) {
        double v = std::get<double>(val->data);
        
        if (op == Opcode::FPToSI) {
            return new Constant(destType, static_cast<int64_t>(v), 0);
        }
    }
    
    return nullptr;
}

// Dead Code Elimination
bool DeadCodeEliminationPass::runOnFunction(Function* func) {
    bool changed = false;
    std::unordered_set<Instruction*> liveInstructions;
    
    // Mark all side-effecting instructions as live
    for (auto& bb : func->basicBlocks) {
        for (auto& inst : bb->instructions) {
            switch (inst->opcode) {
                case Opcode::Ret:
                case Opcode::Br:
                case Opcode::CondBr:
                case Opcode::Switch:
                case Opcode::Store:
                case Opcode::Call:
                    liveInstructions.insert(inst.get());
                    break;
                default:
                    break;
            }
        }
    }
    
    // Iteratively mark instructions that produce values used by live instructions
    bool added;
    do {
        added = false;
        for (auto& bb : func->basicBlocks) {
            for (auto& inst : bb->instructions) {
                if (liveInstructions.count(inst.get())) continue;
                
                for (auto* op : inst->operands) {
                    if (auto* opInst = dynamic_cast<Instruction*>(op)) {
                        if (liveInstructions.count(opInst)) {
                            liveInstructions.insert(inst.get());
                            added = true;
                            break;
                        }
                    }
                }
            }
        }
    } while (added);
    
    // Remove dead instructions
    for (auto& bb : func->basicBlocks) {
        auto& insts = bb->instructions;
        insts.erase(
            std::remove_if(insts.begin(), insts.end(),
                [&liveInstructions](const std::unique_ptr<Instruction>& inst) {
                    return !liveInstructions.count(inst.get()) && 
                           inst->opcode != Opcode::Ret &&
                           inst->opcode != Opcode::Br &&
                           inst->opcode != Opcode::CondBr;
                }),
            insts.end()
        );
    }
    
    return changed;
}

// Loop Invariant Code Motion
bool LoopInvariantCodeMotionPass::runOnFunction(Function* func) {
    // Simplified implementation
    // Real implementation would require full loop analysis
    return false;
}

// Inline Expansion
bool InlineExpansionPass::runOnFunction(Function* func) {
    bool changed = false;
    
    for (auto& bb : func->basicBlocks) {
        for (auto& inst : bb->instructions) {
            if (inst->opcode == Opcode::Call) {
                // Check if callee is small enough to inline
                if (auto* callee = dynamic_cast<Function*>(inst->operands[0])) {
                    if (callee->getInstructionCount() <= threshold_ && 
                        callee != func && !callee->isExternal) {
                        // Inline the function
                        // Real implementation would clone callee body
                        changed = true;
                    }
                }
            }
        }
    }
    
    return changed;
}

// Dominator Tree (placeholder)
std::unordered_map<Function*, DominatorTree*> DominatorTreePass::results;

bool DominatorTreePass::runOnFunction(Function* func) {
    // Compute dominator tree
    // Real implementation would use standard algorithms
    return false;
}

// Loop Analysis (placeholder)
bool LoopAnalysisPass::runOnFunction(Function* func) {
    // Identify natural loops
    return false;
}

} // namespace compiler
