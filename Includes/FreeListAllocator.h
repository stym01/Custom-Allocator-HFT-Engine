#ifndef FREELIST_ALLOCATOR_H
#define FREELIST_ALLOCATOR_H

#include "Allocator.h"
#include <cstdlib>
#include <iostream>

class FreeListAllocator : public Allocator {
private:
    struct AllocationHeader { 
        size_t size; 
        size_t padding; 
    };

    struct Node {
        size_t size;
        Node* next;
    };

    Node* m_free_list_head; 

public:
    FreeListAllocator(size_t totalSize) : Allocator(totalSize) {
        m_free_list_head = nullptr;
    }

    void Init() override {
        if (m_start_ptr != nullptr) free(m_start_ptr);
        m_start_ptr = malloc(m_total_size);
        
        Reset();
    }

    ~FreeListAllocator() {
        if (m_start_ptr != nullptr) free(m_start_ptr);
    }

    void* Allocate(size_t size, size_t alignment = 8) override {
        // We need space for the Header + the Data
        // Total needed = sizeof(Header) + size + padding
        
        Node* prev_node = nullptr;
        Node* curr_node = m_free_list_head;

        // first fit
        while (curr_node != nullptr) {
            
            size_t padding = 0;
            // Calculate padding relative to the payload address
            // Address: [Header][Padding][Payload]
            // We want [Payload] to be aligned.
            
            uintptr_t current_addr = (uintptr_t)curr_node;
            uintptr_t header_end = current_addr + sizeof(AllocationHeader);
            
            size_t mask = alignment - 1;
            if (header_end & mask) {
                padding = alignment - (header_end & mask);
            }

            size_t required_space = size + padding + sizeof(AllocationHeader);

            // DOES IT FIT?
            if (curr_node->size >= required_space) {
                // FOUND ONE!
                
                // 1. Calculate remaining space (Splitting)
                // If the block is huge, we split it. If it's close to perfect, we take it all.
                size_t remaining = curr_node->size - required_space;

                // Minimum size for a free node is sizeof(Node)
                if (remaining > sizeof(Node)) {
                    // SPLIT: Create a new free node in the remaining space
                    Node* new_free_node = (Node*)((uintptr_t)curr_node + required_space);
                    new_free_node->size = remaining;
                    new_free_node->next = curr_node->next;

                    // Update the list links
                    if (prev_node) prev_node->next = new_free_node;
                    else m_free_list_head = new_free_node;
                } else {
                    // DON'T SPLIT: Just take the whole block (waste a tiny bit)
                    if (prev_node) prev_node->next = curr_node->next;
                    else m_free_list_head = curr_node->next;
                    
                    // Add the wasted space to required_space so we track it correctly
                    required_space = curr_node->size; 
                }

                // 2. Setup the Allocation Header
                uintptr_t header_addr = (uintptr_t)curr_node + padding;
                uintptr_t payload_addr = header_addr + sizeof(AllocationHeader);
                
                AllocationHeader* header = (AllocationHeader*)header_addr;
                header->size = required_space;
                header->padding = padding;

                m_used_memory += required_space;
                m_num_allocations++;

                return (void*)payload_addr;
            }

            // Next block
            prev_node = curr_node;
            curr_node = curr_node->next;
        }

        std::cout << "FreeListAllocator: No block big enough found!" << std::endl;
        return nullptr;
    }

    void Deallocate(void* ptr) override {
        // 1. Get the Header
        uintptr_t payload_addr = (uintptr_t)ptr;
        
        // We have to jump back over padding? No, we don't know padding yet.
        // Wait, the header is ALWAYS immediately before the payload?
        // No, in my Allocate logic above: [Start][Padding][Header][Payload] ? 
        // NO. The logic above was: [Start]...padding...[Header][Payload].
        // Actually, looking closely at my logic:
        // header_addr = curr_node + padding.
        // payload_addr = header_addr + sizeof(Header).
        // So YES, the header is strictly immediately before the payload.
        
        uintptr_t header_addr = payload_addr - sizeof(AllocationHeader);
        AllocationHeader* alloc_header = (AllocationHeader*)header_addr;
        
        // 2. Calculate the start of the block
        uintptr_t block_start = header_addr - alloc_header->padding;
        size_t block_size = alloc_header->size;

        // 3. Create a Free Node here
        Node* free_node = (Node*)block_start;
        free_node->size = block_size;
        free_node->next = nullptr;

        // 4. Insert into Sorted Linked List & Coalesce (Merge)
        // Ideally, we want the list sorted by address to make merging easy.
        
        Node* prev = nullptr;
        Node* curr = m_free_list_head;

        // Find insertion point (Sorted by Address)
        while (curr != nullptr && curr < free_node) {
            prev = curr;
            curr = curr->next;
        }
        
        // Insert
        if (prev) prev->next = free_node;
        else m_free_list_head = free_node;
        
        free_node->next = curr;

        // 5. Coalesce (Merge neighbors)
        // Check Next
        if (free_node->next != nullptr && 
           (uintptr_t)free_node + free_node->size == (uintptr_t)free_node->next) {
            
            free_node->size += free_node->next->size;
            free_node->next = free_node->next->next;
        }

        // Check Previous
        if (prev != nullptr && 
           (uintptr_t)prev + prev->size == (uintptr_t)free_node) {
            
            prev->size += free_node->size;
            prev->next = free_node->next;
        }

        m_used_memory -= block_size;
        m_num_allocations--;
    }

    void Reset() override {
        m_used_memory = 0;
        m_num_allocations = 0;

        // Create one giant free node
        Node* first_node = (Node*)m_start_ptr;
        first_node->size = m_total_size;
        first_node->next = nullptr;

        m_free_list_head = first_node;
    }
};

#endif