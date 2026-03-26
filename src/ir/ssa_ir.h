#ifndef SSA_IR_H
#define SSA_IR_H

#include "ir.h"
#include <stack>
#include <unordered_set>

namespace compiler {

// Dominator tree node
struct DomTreeNode {
    BasicBlock* block;
    DomTreeNode* parent;
    std::vector<DomTreeNode*> children;
    size_t depth;
    
    explicit DomTreeNode(BasicBlock* b) : block(b), parent(nullptr), depth(0) {}
};

// Dominance Frontier
class DominanceFrontier {
public:
    void compute(Function* func);
    std::vector<BasicBlock*>& getFrontier(BasicBlock* block);
    
private:
    std::unordered_map<BasicBlock*, std::vector<BasicBlock*>> frontier_;
    
    void computeDomTree(Function* func);
    void computeFrontier(Function* func);
};

// SSA Constructor using iterated dominance frontier
class SSAConstructor {
public:
    void construct(Function* func);
    
private:
    void placePhiNodes(Function* func);
    void renameVariables(Function* func);
    void renameBlock(BasicBlock* block, std::unordered_map<std::string, std::stack<Value*>>& stacks);
    
    DominanceFrontier domFrontier_;
    std::unordered_set<std::string> globals_;
    std::unordered_map<BasicBlock*, std::unordered_set<std::string>> blockDefs_;
};

// Phi insertion helper
struct PhiPlacement {
    BasicBlock* block;
    std::string varName;
    IRType type;
};

} // namespace compiler

#endif // SSA_IR_H
