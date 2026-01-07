#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <cstddef>  //  for using size_t...
#include <cstdint>  // for uintptr_t...

class Allocator {
protected:
    void* m_start_ptr;      
    size_t m_total_size;    
    size_t m_used_memory;   
    size_t m_num_allocations; 

public:
    Allocator(size_t totalSize) 
        : m_total_size(totalSize), m_used_memory(0), m_num_allocations(0), m_start_ptr(nullptr) {
    }

    virtual ~Allocator() { m_start_ptr = nullptr; }

    virtual void* Allocate(size_t size, size_t alignment = 8) = 0;
    virtual void Deallocate(void* ptr) = 0;
    virtual void Init() = 0;
    virtual void Reset(){}

    void* GetStart() const { return m_start_ptr; }
    size_t GetUsedMemory() const { return m_used_memory; }
    size_t GetNumAllocations() const { return m_num_allocations; }
};

#endif