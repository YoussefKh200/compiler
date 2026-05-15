#include "ir.h"
#include <sstream>

namespace compiler {

std::string Value::toString() const {
    if (!name.empty()) {
        return "%" + name;
    }
    return "%" + std::to_string(id);
}

bool Value::isConstant() const {
    return dynamic_cast<const Constant*>(this) != nullptr;
}

std::string Constant::toString() const {
    std::stringstream ss;
    if (std::holds_alternative<int64_t>(data)) {
        ss << std::get<int64_t>(data);
    } else if (std::holds_alternative<double>(data)) {
        ss << std::get<double>(data);
    } else if (std::holds_alternative<bool>(data)) {
        ss << (std::get<bool>(data) ? "true" : "false");
    }
    return ss.str();
}

std::string Instruction::toString() const {
    std::stringstream ss;
    
    // Result name
    if (type != IRType::Void) {
        ss << toString() << " = ";
    }
    
    // Opcode
    switch (opcode) {
        case Opcode::Add: ss << "add"; break;
        case Opcode::Sub: ss << "sub"; break;
        case Opcode::Mul: ss << "mul"; break;
        case Opcode::SDiv: ss << "sdiv"; break;
        case Opcode::UDiv: ss << "udiv"; break;
        case Opcode::SRem: ss << "srem"; break;
        case Opcode::FAdd: ss << "fadd"; break;
        case Opcode::FSub: ss << "fsub"; break;
        case Opcode::FMul: ss << "fmul"; break;
        case Opcode::FDiv: ss << "fdiv"; break;
        case Opcode::And: ss << "and"; break;
        case Opcode::Or: ss << "or"; break;
        case Opcode::Xor: ss << "xor"; break;
        case Opcode::Shl: ss << "shl"; break;
        case Opcode::LShr: ss << "lshr"; break;
        case Opcode::AShr: ss << "ashr"; break;
        case Opcode::ICmpEq: ss << "icmp eq"; break;
        case Opcode::ICmpNe: ss << "icmp ne"; break;
        case Opcode::ICmpSlt: ss << "icmp slt"; break;
        case Opcode::ICmpSle: ss << "icmp sle"; break;
        case Opcode::ICmpSgt: ss << "icmp sgt"; break;
        case Opcode::ICmpSge: ss << "icmp sge"; break;
        case Opcode::Load: ss << "load"; break;
        case Opcode::Store: ss << "store"; break;
        case Opcode::Alloca: ss << "alloca"; break;
        case Opcode::Ret: ss << "ret"; break;
        case Opcode::Br: ss << "br"; break;
        case Opcode::CondBr: ss << "br"; break;
        case Opcode::Call: ss << "call"; break;
        case Opcode::Phi: ss << "phi"; break;
        default: ss << "unknown"; break;
    }
    
    // Type and operands
    if (opcode != Opcode::Ret && opcode != Opcode::Br && opcode != Opcode::CondBr) {
        ss << " " << irTypeToString(type);
    }
    
    for (size_t i = 0; i < operands.size(); ++i) {
        ss << (i == 0 ? " " : ", ");
        if (operands[i]) {
            ss << operands[i]->toString();
        } else {
            ss << "null";
        }
    }
    
    return ss.str();
}

void BasicBlock::addInstruction(std::unique_ptr<Instruction> inst) {
    instructions.push_back(std::move(inst));
}

Instruction* BasicBlock::getTerminator() const {
    if (instructions.empty()) return nullptr;
    auto* last = instructions.back().get();
    switch (last->opcode) {
        case Opcode::Ret:
        case Opcode::Br:
        case Opcode::CondBr:
        case Opcode::Switch:
        case Opcode::Unreachable:
            return last;
        default:
            return nullptr;
    }
}

bool BasicBlock::isTerminated() const {
    return getTerminator() != nullptr;
}

BasicBlock* Function::createBasicBlock(const std::string& name) {
    auto bb = std::make_unique<BasicBlock>(name, parent->getNextId(), this);
    auto* ptr = bb.get();
    basicBlocks.push_back(std::move(bb));
    return ptr;
}

BasicBlock* Function::getEntryBlock() const {
    if (basicBlocks.empty()) return nullptr;
    return basicBlocks[0].get();
}

size_t Function::getInstructionCount() const {
    size_t count = 0;
    for (const auto& bb : basicBlocks) {
        count += bb->instructions.size();
    }
    return count;
}

Function* Module::createFunction(const std::string& name, IRType retType, bool external) {
    auto func = std::make_unique<Function>(name, retType, nextValueId++, this, external);
    auto* ptr = func.get();
    functions.push_back(std::move(func));
    return ptr;
}

GlobalVariable* Module::createGlobal(const std::string& name, IRType type, bool constant) {
    auto global = std::make_unique<GlobalVariable>(name, type, nextValueId++, constant);
    auto* ptr = global.get();
    globals.push_back(std::move(global));
    return ptr;
}

std::string Module::toString() const {
    std::stringstream ss;
    
    ss << "; Module: " << name << "\n\n";
    
    // Globals
    for (const auto& global : globals) {
        ss << "@" << global->name << " = ";
        if (global->isConstant) ss << "constant ";
        else ss << "global ";
        ss << irTypeToString(global->type);
        if (global->initializer) {
            ss << " " << global->initializer->toString();
        }
        ss << "\n";
    }
    if (!globals.empty()) ss << "\n";
    
    // Functions
    for (const auto& func : functions) {
        ss << "define " << irTypeToString(func->returnType) << " @" << func->name << "(";
        for (size_t i = 0; i < func->parameters.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << irTypeToString(func->parameters[i].second) << " %" << func->parameters[i].first;
        }
        ss << ") {\n";
        
        for (const auto& bb : func->basicBlocks) {
            ss << bb->name << ":\n";
            for (const auto& inst : bb->instructions) {
                ss << "    " << inst->toString() << "\n";
            }
        }
        ss << "}\n\n";
    }
    
    return ss.str();
}

std::string irTypeToString(IRType type) {
    switch (type) {
        case IRType::Void: return "void";
        case IRType::I1: return "i1";
        case IRType::I8: return "i8";
        case IRType::I16: return "i16";
        case IRType::I32: return "i32";
        case IRType::I64: return "i64";
        case IRType::F32: return "f32";
        case IRType::F64: return "f64";
        case IRType::Ptr: return "ptr";
        case IRType::Array: return "array";
        case IRType::Struct: return "struct";
        case IRType::Function: return "func";
    }
    return "unknown";
}

size_t getTypeSize(IRType type) {
    switch (type) {
        case IRType::Void: return 0;
        case IRType::I1: return 1;
        case IRType::I8: return 1;
        case IRType::I16: return 2;
        case IRType::I32: return 4;
        case IRType::I64: return 8;
        case IRType::F32: return 4;
        case IRType::F64: return 8;
        case IRType::Ptr: return 8;
        default: return 0;
    }
}

} // namespace compiler
