#ifndef POOL_ALLOCATOR_H
#define POOL_ALLOCATOR_H

#include "Allocator.h"
#include <cstdlib>

class PoolAllocator : public Allocator {
private:
    struct FreeHeader {
        FreeHeader* next;
    };

    FreeHeader* m_free_list_head; 
    size_t m_chunk_size;
    size_t m_alignment;

public:
    PoolAllocator(size_t totalSize, size_t chunkSize, size_t alignment = 8) 
        : Allocator(totalSize), m_chunk_size(chunkSize), m_alignment(alignment) {
            
        if (m_chunk_size < sizeof(FreeHeader*)) {
            m_chunk_size = sizeof(FreeHeader*);
        }
        
        //align the chunk size so every block starts at an aligned address...ex if 10 then make chunk as 16 if align is 8
        size_t mask = m_alignment - 1;
        if (m_chunk_size & mask) {
             m_chunk_size += m_alignment - (m_chunk_size & mask);
        }
    }

    void Init() override {
        if (m_start_ptr != nullptr) free(m_start_ptr);
        m_start_ptr = malloc(m_total_size);
        
        Reset(); //build LL
    }

    ~PoolAllocator() {
        if (m_start_ptr != nullptr) free(m_start_ptr);
    }

    void* Allocate(size_t size, size_t alignment = 8) override {

        if (m_free_list_head == nullptr) {
            return nullptr; 
        }

        FreeHeader* free_block = m_free_list_head;

        m_free_list_head = m_free_list_head->next;

        m_used_memory += m_chunk_size;
        m_num_allocations++;

        return (void*)free_block;
    }

    void Deallocate(void* ptr) override {
        
        FreeHeader* header = (FreeHeader*)ptr;

        header->next = m_free_list_head;

        m_free_list_head = header;

        m_used_memory -= m_chunk_size;
        m_num_allocations--;
    }

    void Reset() override {
        m_used_memory = 0;
        m_num_allocations = 0;

        size_t nChunks = m_total_size / m_chunk_size;

        m_free_list_head = (FreeHeader*)m_start_ptr;
        FreeHeader* current = m_free_list_head;

        for (size_t i = 0; i < nChunks - 1; ++i) {

            uintptr_t next_addr = (uintptr_t)current + m_chunk_size;
        
            current->next = (FreeHeader*)next_addr;
            
            current = current->next;
        }

        current->next = nullptr;
    }
};

#endif