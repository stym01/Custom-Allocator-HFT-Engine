#ifndef STACK_ALLOCATOR_H
#define STACK_ALLOCATOR_H

#include "Allocator.h"
#include <cstdlib>

class StackAllocator : public Allocator {
protected:
    void* m_current_pos;

    struct AllocationHeader {
        uint8_t padding;
    };

public:
    StackAllocator(size_t totalSize) : Allocator(totalSize) {}

    void Init() override {
        if (m_start_ptr != nullptr) free(m_start_ptr);
        m_start_ptr = malloc(m_total_size);
        m_current_pos = m_start_ptr;
    }

    ~StackAllocator() {
        if (m_start_ptr != nullptr) free(m_start_ptr);
    }

    void* Allocate(size_t size, size_t alignment = 8) override {
        uintptr_t current_address = (uintptr_t)m_current_pos;

        size_t padding = 0;
        uintptr_t headerAddress = current_address + sizeof(AllocationHeader); // Address if header starts HERE
        size_t mask = alignment - 1; 
        
        uintptr_t rawAddress = current_address;
        
        size_t offset = sizeof(AllocationHeader); 
        
        size_t needed_padding = 0;
        uintptr_t payload_address = rawAddress + sizeof(AllocationHeader);
        
        if (payload_address & mask) {
            needed_padding = alignment - (payload_address & mask);
        }
        
        //so total size consumed = sizeof(Header) + needed_padding + size
        size_t total_alloc_size = sizeof(AllocationHeader) + needed_padding + size;

        if (m_used_memory + total_alloc_size > m_total_size) {
            return nullptr;
        }

        uintptr_t header_address = rawAddress + needed_padding;
        uintptr_t data_address   = header_address + sizeof(AllocationHeader);

        AllocationHeader* headerPtr = (AllocationHeader*)header_address;
        headerPtr->padding = (uint8_t)needed_padding;

        m_current_pos = (void*)(data_address + size);
        m_used_memory += total_alloc_size;
        m_num_allocations++;

        return (void*)data_address;
    }

    void Deallocate(void* ptr) override {

        uintptr_t data_start = (uintptr_t)ptr;
        uintptr_t current_top = (uintptr_t)m_current_pos;

        uintptr_t header_address = data_start - sizeof(AllocationHeader);
        AllocationHeader* headerPtr = (AllocationHeader*)header_address;
        
        uintptr_t block_start = header_address - headerPtr->padding;
        
        m_current_pos = (void*)block_start;
        
        m_used_memory = m_used_memory - (current_top - block_start);
        m_num_allocations--;
    }

    void Reset() override {
        m_current_pos = m_start_ptr;
        m_used_memory = 0;
        m_num_allocations = 0;
    }
};

#endif