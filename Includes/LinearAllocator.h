#ifndef LINEAR_ALLOCATOR_H
#define LINEAR_ALLOCATOR_H

#include "Allocator.h"
#include <cstdlib>   
#include <iostream>

class LinearAllocator : public Allocator {
protected:
    void* m_current_pos;

public:
    LinearAllocator(size_t totalSize) : Allocator(totalSize) {}

    void Init() override {
        if (m_start_ptr != nullptr) {
            free(m_start_ptr);
        }
        m_start_ptr = malloc(m_total_size); // asking the OS for raw bytes...
        m_current_pos = m_start_ptr;       
    }

    ~LinearAllocator() {
        if (m_start_ptr != nullptr) {
            free(m_start_ptr); 
            m_start_ptr = nullptr;
        }
    }

    void* Allocate(size_t size, size_t alignment = 8) override {
        
        uintptr_t current_address = (uintptr_t)m_current_pos;
        
        //padding for alignment
        size_t padding = 0;
        size_t mask = alignment - 1; 
        
        if (current_address & mask) {
            padding = alignment - (current_address & mask);
        }
        
        if (m_used_memory + padding + size > m_total_size) {
            std::cout << "Error: LinearAllocator full!" << std::endl;
            return nullptr;
        }
        uintptr_t aligned_address = current_address + padding;

        m_current_pos = (void*)(aligned_address + size);
        
        m_used_memory += padding + size;
        m_num_allocations++;

        return (void*)aligned_address; 
    }

    void Deallocate(void* ptr) override {
        //do nothing as it iwll only free all at once
    }

    void Reset() override {
        m_current_pos = m_start_ptr; 
        m_used_memory = 0;
        m_num_allocations = 0;
    }
};

#endif