#include <iostream>
#include <chrono>
#include <vector>

#include "../Includes/LinearAllocator.h"
#include "../Includes/StackAllocator.h"
#include "../Includes/PoolAllocator.h"
#include "../Includes/FreeListAllocator.h" 

struct Vector4 {
    float x, y, z, w;
};

const int NUM_OPERATIONS = 500000; 
const size_t TOTAL_SIZE = 512 * 1024 * 1024; // 512 MB

class Timer {
    std::chrono::high_resolution_clock::time_point start;
public:
    void Start() { start = std::chrono::high_resolution_clock::now(); }
    double Stop() {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        return elapsed.count();
    }
};

int main() {
    std::cout << "Benchmark started" << std::endl;
    std::cout << "Operations: " << NUM_OPERATIONS << std::endl;
    std::cout << "Object Size: " << sizeof(Vector4) << " bytes" << std::endl;

    Timer timer;

    {
        std::vector<Vector4*> ptrs(NUM_OPERATIONS);

        std::cout << "Testing Standard new/delete..." << std::endl;
        timer.Start();

        for (int i = 0; i < NUM_OPERATIONS; ++i) {
            ptrs[i] = new Vector4();
        }
        
        for (int i = 0; i < NUM_OPERATIONS; ++i) {
            delete ptrs[i];
        }

        std::cout << "Result: " << timer.Stop() << " ms" << std::endl;
    }

    {
        std::cout << "Testing Linear Allocator..." << std::endl;
        LinearAllocator* linear = new LinearAllocator(TOTAL_SIZE);
        linear->Init();
        
        timer.Start();
        
        for (int i = 0; i < NUM_OPERATIONS; ++i) {
             linear->Allocate(sizeof(Vector4), alignof(Vector4));
        }
        linear->Reset();

        std::cout << "Result: " << timer.Stop() << " ms" << std::endl;
        delete linear;
    }

    {
        std::cout << "Testing Stack Allocator..." << std::endl;
        StackAllocator* stack = new StackAllocator(TOTAL_SIZE);
        stack->Init();
        
        std::vector<void*> ptrs(NUM_OPERATIONS);

        timer.Start();
        
        for (int i = 0; i < NUM_OPERATIONS; ++i) {
            ptrs[i] = stack->Allocate(sizeof(Vector4), alignof(Vector4));
        }
        
        //deallocate (MUST be Reverse Order)
        for (int i = NUM_OPERATIONS - 1; i >= 0; --i) {
            stack->Deallocate(ptrs[i]);
        }

        std::cout << "Result: " << timer.Stop() << " ms" << std::endl;
        delete stack;
    }

    {
        std::cout << "Testing Pool Allocator..." << std::endl;
        PoolAllocator* pool = new PoolAllocator(TOTAL_SIZE, sizeof(Vector4), alignof(Vector4));
        pool->Init();
        
        std::vector<void*> ptrs(NUM_OPERATIONS);

        timer.Start();

        for (int i = 0; i < NUM_OPERATIONS; ++i) {
            ptrs[i] = pool->Allocate(sizeof(Vector4));
        }
        
        for (int i = 0; i < NUM_OPERATIONS; ++i) {
            pool->Deallocate(ptrs[i]);
        }

        std::cout << "Result: " << timer.Stop() << " ms" << std::endl;
        delete pool;
    }
    
    {
        std::cout << "Testing Free List Allocator..." << std::endl;
        FreeListAllocator* freeList = new FreeListAllocator(TOTAL_SIZE);
        freeList->Init();
        
        std::vector<void*> ptrs(NUM_OPERATIONS);

        timer.Start();

        for (int i = 0; i < NUM_OPERATIONS; ++i) {
            ptrs[i] = freeList->Allocate(sizeof(Vector4), alignof(Vector4));
        }

        for (int i = 0; i < NUM_OPERATIONS; ++i) {
            freeList->Deallocate(ptrs[i]);
        }

        std::cout << "Result: " << timer.Stop() << " ms" << std::endl;
        delete freeList;
    }

    return 0;
}