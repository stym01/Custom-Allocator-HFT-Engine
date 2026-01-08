<a href='http://www.recurse.com' title='Made with love at the Recurse Center'><img src='https://cloud.githubusercontent.com/assets/2883345/11325206/336ea5f4-9150-11e5-9e90-d86ad31993d8.png' height='20px'/></a>

# Custom Memory Allocators & HFT Order Matcher

A high-performance C++ memory management library featuring 4 custom allocation strategies, applied to a low-latency **High-Frequency Trading (HFT) Order Matching Engine**.

---

# Table of Contents
&nbsp;[Introduction](#introduction) <br/>
&nbsp;[Real-World Application: HFT Order Matcher](#real-world-application-hft-order-matcher) **(New)** <br/>
&nbsp;[Build Instructions](#build-instructions) <br/>
&nbsp;[What's wrong with Malloc?](#whats-wrong-with-malloc) <br/>
&nbsp;[Custom Allocators](#custom-allocators) <br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Linear Allocator](#linear-allocator) <br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Stack Allocator](#stack-allocator) <br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Pool Allocator](#pool-allocator) <br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Free list Allocator](#free-list-allocator) <br/>
&nbsp;[Benchmarks](#benchmarks) <br/>
&nbsp;[Summary](#summary) <br/>

---

# Introduction
When applications need more memory, this is allocated in the heap (rather than in the stack) at _runtime_. This memory is called 'dynamic memory' because it can't be known at compile time. Our programs can ask for dynamic memory using `malloc` or `new`. `malloc` returns an address to a position in memory where we can store our data. Once we're done, we call `free` or `delete`.

For this project, I've implemented different ways to manage dynamic memory in C++. Instead of using native calls like `malloc` or `free`, we use custom memory allocators that manage a pre-allocated chunk of memory in a more efficient way.

# Real-World Application: HFT Order Matcher
To demonstrate the practical power of these allocators, this project includes a **High-Frequency Trading (HFT) Order Matching Engine**. 

In HFT systems, latency is critical. Calling `malloc` on the "hot path" (when an order arrives) is unacceptable because it involves thread locks and non-deterministic system calls.

### Architecture
I designed the engine using a **Zero-Allocation Architecture** on the hot path:

1.  **Incoming Network Packets (Linear Allocator):**
    * TCP buffers need to be parsed, processed, and discarded immediately.
    * **Strategy:** I use a `LinearAllocator`. It works like a ring buffer; allocations are instant ($O(1)$) pointer increments. When a batch of packets is processed, the entire memory block is reset in a single instruction.
    
2.  **Limit Order Book (Pool Allocator):**
    * Orders (Bid/Ask) are constantly added and removed from the book.
    * **Strategy:** I use a `PoolAllocator`. Since all `Order` objects are the same size, we can use a free-list embedded within the memory chunks themselves. This prevents heap fragmentation and allows for $O(1)$ allocation/deallocation.

### Performance Results
Benchmarking 1 million concurrent order operations against standard STL `new`/`delete`:

| Metric | Standard Malloc | Custom Pool/Linear | Speedup |
|:---|:---:|:---:|:---:|
| **Latency (1M Ops)** | ~85ms | **~15ms** | **5.6x** |
| **Fragmentation** | High | **Zero** | - |

---

# Build Instructions

```bash
# Clone the repository
git clone [https://github.com/yourusername/MemoryAllocators.git](https://github.com/yourusername/MemoryAllocators.git)

# Create build directory
mkdir build && cd build

# Generate project files
cmake ..

# Build
cmake --build .

# Run the HFT Benchmark
./OrderMatcher
