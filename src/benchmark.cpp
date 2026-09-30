#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <algorithm>
#include <functional>

#include "../Includes/LinearAllocator.h"
#include "../Includes/StackAllocator.h"
#include "../Includes/PoolAllocator.h"
#include "../Includes/FreeListAllocator.h"

// Workload: N allocations of a 16-byte object followed by N frees (the Linear
// allocator can't free individual blocks, so it does N allocations + one Reset).
// Each allocator is set up (Init) outside the timed region, then the workload is
// repeated REPS times and the median is reported.

struct Vector4 {
    float x, y, z, w;
};

const int NUM_OPERATIONS = 1000000;
const int REPS = 7;
const size_t TOTAL_SIZE = 512 * 1024 * 1024; // 512 MB

// Accumulating returned pointers here stops the compiler from optimising the loops away.
volatile uintptr_t g_sink = 0;

double TimeMs(const std::function<void()>& fn) {
    auto start = std::chrono::steady_clock::now();
    fn();
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

double MedianMs(const std::function<void()>& fn) {
    std::vector<double> samples;
    for (int r = 0; r < REPS; ++r) samples.push_back(TimeMs(fn));
    std::sort(samples.begin(), samples.end());
    return samples[REPS / 2];
}

void Report(const char* name, double ms, double baselineMs) {
    double nsPerOp = ms * 1e6 / NUM_OPERATIONS;
    std::cout << std::left << std::setw(22) << name
              << std::right << std::setw(10) << std::fixed << std::setprecision(2) << ms << " ms"
              << std::setw(10) << std::setprecision(1) << nsPerOp << " ns/op"
              << std::setw(9) << std::setprecision(1) << baselineMs / ms << "x" << std::endl;
}

int main() {
    std::cout << "Operations: " << NUM_OPERATIONS << " alloc + free, object size "
              << sizeof(Vector4) << " bytes, median of " << REPS << " runs\n" << std::endl;

    std::vector<void*> ptrs(NUM_OPERATIONS);

    double newDeleteMs = MedianMs([&] {
        for (int i = 0; i < NUM_OPERATIONS; ++i) {
            ptrs[i] = new Vector4();
            g_sink ^= reinterpret_cast<uintptr_t>(ptrs[i]);
        }
        for (int i = 0; i < NUM_OPERATIONS; ++i) delete static_cast<Vector4*>(ptrs[i]);
    });

    LinearAllocator linear(TOTAL_SIZE);
    linear.Init();
    double linearMs = MedianMs([&] {
        for (int i = 0; i < NUM_OPERATIONS; ++i)
            g_sink ^= reinterpret_cast<uintptr_t>(linear.Allocate(sizeof(Vector4), alignof(Vector4)));
        linear.Reset();
    });

    StackAllocator stack(TOTAL_SIZE);
    stack.Init();
    double stackMs = MedianMs([&] {
        for (int i = 0; i < NUM_OPERATIONS; ++i) {
            ptrs[i] = stack.Allocate(sizeof(Vector4), alignof(Vector4));
            g_sink ^= reinterpret_cast<uintptr_t>(ptrs[i]);
        }
        for (int i = NUM_OPERATIONS - 1; i >= 0; --i) stack.Deallocate(ptrs[i]); // LIFO only
    });

    PoolAllocator pool(TOTAL_SIZE, sizeof(Vector4), alignof(Vector4));
    pool.Init();
    double poolMs = MedianMs([&] {
        for (int i = 0; i < NUM_OPERATIONS; ++i) {
            ptrs[i] = pool.Allocate(sizeof(Vector4));
            g_sink ^= reinterpret_cast<uintptr_t>(ptrs[i]);
        }
        for (int i = 0; i < NUM_OPERATIONS; ++i) pool.Deallocate(ptrs[i]);
    });

    FreeListAllocator freeList(TOTAL_SIZE);
    freeList.Init();
    double freeListMs = MedianMs([&] {
        for (int i = 0; i < NUM_OPERATIONS; ++i) {
            ptrs[i] = freeList.Allocate(sizeof(Vector4), alignof(Vector4));
            g_sink ^= reinterpret_cast<uintptr_t>(ptrs[i]);
        }
        for (int i = 0; i < NUM_OPERATIONS; ++i) freeList.Deallocate(ptrs[i]);
    });

    std::cout << std::left << std::setw(22) << "Allocator" << std::right << std::setw(13) << "Median"
              << std::setw(16) << "Per op" << std::setw(10) << "Speedup" << std::endl;
    Report("new/delete", newDeleteMs, newDeleteMs);
    Report("Linear (alloc+Reset)", linearMs, newDeleteMs);
    Report("Stack", stackMs, newDeleteMs);
    Report("Pool", poolMs, newDeleteMs);
    Report("Free List", freeListMs, newDeleteMs);
    return 0;
}
