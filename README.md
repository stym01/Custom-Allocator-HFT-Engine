# Custom Memory Allocators & Order Matching Engine (C++17)

Four custom memory allocators written from scratch — **Linear, Stack, Pool and Free-List** — and a small limit-order-book matching engine that uses them so that the matching hot path never calls `new`/`malloc`.

---

## What's in the repo

| Path | What it is |
|---|---|
| `Includes/Allocator.h` | Common interface (`Init`, `Allocate`, `Deallocate`, `Reset`) |
| `Includes/LinearAllocator.h` | Bump-pointer allocator; frees everything at once with `Reset()` |
| `Includes/StackAllocator.h` | Like Linear, plus LIFO frees using a small header per block |
| `Includes/PoolAllocator.h` | Fixed-size blocks; the free list is stored inside the free blocks |
| `Includes/FreeListAllocator.h` | General-purpose: address-sorted free list, first-fit, coalescing on free |
| `src/OrderMatcher.cpp` | Price-time priority order book (buy/sell limit orders) |
| `src/benchmark.cpp` | Allocator micro-benchmark against `new`/`delete` |
| `src/Examples/` | Small usage examples for the Linear, Stack and Pool allocators |

## How the order book uses the allocators

- **Pool allocator for orders.** Every `Order` has the same size, so resting orders are carved out of a pre-allocated pool. Adding an order is a free-list pop and removing a filled order is a push — both O(1), with no heap fragmentation and no `new`/`malloc` on the matching path.
- **Linear allocator for incoming messages.** Each incoming order message is decoded into scratch memory from a linear allocator, which is reset after the message is processed. (Messages are generated in-process by a simulation loop; there is no network I/O.)
- **Matching.** Buy orders are kept highest-price-first and sell orders lowest-price-first. An incoming order matches against the best opposite price while prices cross, partially or fully filling resting orders; any remainder is inserted into the book.

## Benchmark

`src/benchmark.cpp` performs **1,000,000 allocations of a 16-byte object followed by 1,000,000 frees**, for each allocator. Allocators are initialised outside the timed region, the workload is repeated 7 times, and the **median** is reported. The Linear allocator cannot free individual blocks, so it does 1M allocations followed by a single `Reset()`.

Environment: WSL2 (Ubuntu 24.04), g++ 13.3, `-O2`, 8 vCPUs.

| Allocator | Median time | Per alloc+free | vs `new`/`delete` |
|---|---:|---:|---:|
| `new` / `delete` (glibc) | ~19–23 ms | ~20 ns | 1× |
| **Linear** (alloc + one reset) | ~1.2 ms | ~1.2 ns | **~15×** |
| Stack (LIFO frees) | ~8.5 ms | ~8.5 ns | ~2.5× |
| **Pool** | ~7–9.5 ms | ~8 ns | **~2.5×** |
| Free List (general purpose) | ~16–24 ms | ~20 ns | ~1× |

Takeaways:
- The more restrictions an allocator imposes (fixed size, LIFO, bulk free), the faster it is. Linear and Pool win because allocation is a pointer bump or a single list pop.
- A general-purpose free-list allocator is not faster than glibc, which already has per-thread caches for small allocations.
- An earlier, unoptimised build on Windows (MinGW) showed a much larger gap for `new`/`delete` (≈117 ms vs ≈7.35 ms for the pool over 500K operations), because the default Windows heap is slower than glibc's. The numbers above are the ones to compare against.

Run it yourself (numbers vary between runs and machines):

```bash
g++ -std=c++17 -O2 -I Includes src/benchmark.cpp -o benchmark
./benchmark
```

## Allocators in brief

| Allocator | Allocate | Free | Constraint |
|---|---|---|---|
| Linear | O(1) pointer bump | all at once (`Reset`) | no individual frees |
| Stack | O(1) + header | O(1), LIFO order only | frees must be in reverse order |
| Pool | O(1) list pop | O(1) list push | one fixed block size |
| Free List | O(n) first-fit over free blocks | O(n) sorted insert + coalescing | none (general purpose) |

## Current limitations and next steps

- The book is a sorted singly linked list per side, so inserting a resting order is O(n) in the number of resting orders. Next step: price levels in a sorted map (or a tick-indexed array) with a FIFO queue per level.
- Prices are `double`; production engines use integer ticks.
- Trades are printed with `std::cout` inside the matching loop; a real engine would log to a ring buffer off the hot path.
- No cancel/modify yet (needs an order-id → order index for O(1) cancel).
- The matching engine itself isn't benchmarked yet — only the allocators are. Next step: replay a message file and record per-message latency percentiles (p50/p99).

## Build

No build system needed; tested with g++ on Linux/WSL (MinGW on Windows also works).

```bash
git clone https://github.com/stym01/Custom-Allocator-HFT-Engine.git
cd Custom-Allocator-HFT-Engine

g++ -std=c++17 -O2 -I Includes src/OrderMatcher.cpp -o OrderMatcher
./OrderMatcher

g++ -std=c++17 -O2 -I Includes src/benchmark.cpp -o benchmark
./benchmark
```
