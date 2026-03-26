#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <vector>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>

namespace compiler {

// Arena allocator for fast AST/IR node allocation
class MemoryPool {
public:
    explicit MemoryPool(size_t blockSize = 64 * 1024) 
        : blockSize_(blockSize), currentOffset_(0) {
        allocateNewBlock();
    }

    ~MemoryPool() {
        for (auto& block : blocks_) {
            ::operator delete(block, blockSize_, std::align_val_t{alignof(std::max_align_t)});
        }
    }

    // Non-copyable, non-movable
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    template<typename T, typename... Args>
    T* allocate(Args&&... args) {
        static_assert(alignof(T) <= alignof(std::max_align_t), "Alignment too large");
        
        size_t size = sizeof(T);
        size_t alignment = alignof(T);
        
        // Align current offset
        size_t alignedOffset = (currentOffset_ + alignment - 1) & ~(alignment - 1);
        
        if (alignedOffset + size > blockSize_) {
            allocateNewBlock();
            alignedOffset = 0;
        }

        void* ptr = static_cast<char*>(blocks_.back()) + alignedOffset;
        currentOffset_ = alignedOffset + size;
        
        return new (ptr) T(std::forward<Args>(args)...);
    }

    void reset() {
        for (size_t i = 1; i < blocks_.size(); ++i) {
            ::operator delete(blocks_[i], blockSize_, std::align_val_t{alignof(std::max_align_t)});
        }
        blocks_.resize(1);
        currentOffset_ = 0;
    }

private:
    void allocateNewBlock() {
        void* block = ::operator new(blockSize_, std::align_val_t{alignof(std::max_align_t)});
        blocks_.push_back(block);
        currentOffset_ = 0;
    }

    std::vector<void*> blocks_;
    size_t blockSize_;
    size_t currentOffset_;
};

// Smart pointer for pool-allocated objects
template<typename T>
class PoolPtr {
public:
    explicit PoolPtr(T* ptr = nullptr) : ptr_(ptr) {}
    
    T* get() const { return ptr_; }
    T* operator->() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    explicit operator bool() const { return ptr_ != nullptr; }

private:
    T* ptr_;
};

} // namespace compiler

#endif // MEMORY_POOL_H
