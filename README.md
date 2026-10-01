# High Performance Logger

A multithreaded C++ logger built to explore concurrency, latency, and performance tradeoffs in systems programming.

## Overview

Two implementations of the same logger interface, benchmarked against each other:

- **Mutex logger** — `std::queue` + `std::mutex` + `std::condition_variable` + `ofstream`
- **Lock-free logger** — custom MPMC queue + atomic operations + raw `write()` syscall + `sched_yield`

Same benchmark, same interface, honest numbers.  
 
---

## Results

Benchmark: 7 generator threads, 100,000 events each, stress mode (no sleep between logs), Intel i5-10300H @ 2.50GHz, 4 physical cores / 8 logical cores.

```
                  Mutex           Lock-Free
Throughput        1.3M logs/sec   4.6M logs/sec   (3.5x)
p50               1530 ns         649 ns          (2.4x)
p99               31 µs           3.9 µs          (8x)
p999              55 µs           6.7 µs          (8x)
Drops             0               0
```

---

## Architecture

### Mutex Logger

```
producer 1 ──┐
producer 2 ──┤
producer 3 ──┼──► mutex + std::queue ──► worker thread ──► ofstream
producer 4 ──┤
producer 5 ──┘
```

- Producers serialize through one mutex
- Worker blocks on condition variable when queue is empty
- Unbounded queue — never drops, but will OOM under sustained overload
- ofstream buffered writes to disk

### Lock-Free Logger

```
producer 1 ──┐
producer 2 ──┤
producer 3 ──┼──► MPMC lock-free queue ──► worker thread ──► write() syscall
producer 4 ──┤
producer 5 ──┘
```

- Producers use compare-and-swap to claim slots — no mutex
- Worker spins briefly then yields via `sched_yield()` when queue is empty
- Bounded queue (262,144 slots) — drops messages when full under extreme load
- Batches messages into a single contiguous buffer per drain cycle
- One `write()` syscall per batch instead of one per message

---

## MPMC Queue Design

The lock-free queue uses sequence numbers per slot to coordinate producers and consumers without locks.

Each slot has a generation counter:
- `sequence == position` → slot is empty, safe to write
- `sequence == position + 1` → slot is full, safe to read

Producers and consumers use `compare_exchange_weak` to atomically claim slots. Head and tail counters are cache-line aligned to prevent false sharing.

Queue capacity must be a power of 2 — enables `pos & (capacity - 1)` instead of `pos % capacity`.

---

## What Was Learned

### Measurement
- p50 lies. p99 and p999 tell the real story — the gap between them is contention
- Benchmarks under 0.5 seconds are dominated by warmup and OS scheduling noise
- Always measure string construction separately from the log call itself
- `sleep_for(1µs)` actually sleeps ~60µs on Linux — OS scheduler resolution floor

### Concurrency
- Lock contention and IO contention are different bottlenecks — prove which one with ablation
- Batching only helps when the queue is backlogged — useless under light load
- `notify_one()` is a syscall — every `log()` call paid this cost in the mutex design
- Hyperthreading means 8 logical cores ≠ 8 independent cores

### Performance
- String construction with `+` operator does 7 heap allocations per message — `snprintf` does 1
- `ofstream <<` per message vs single `write()` syscall per batch — one syscall amortizes the overhead
- Thread pinning on a hyperthreaded machine can make things worse if you pin two threads to the same physical core
- The OS scheduler resolution (~60µs) sets a floor on max latency that userspace cannot escape

### Fundamental Limits
- Single writer throughput ceiling exists regardless of how fast producers are
- Multi-writer designs move the serialization bottleneck, not eliminate it
- Globally ordered logs + high throughput + low latency — pick two

---

## Building

```bash
mkdir build && cd build

# mutex logger
cmake .. -DUSE_LOCKFREE=OFF && make

# lock-free logger
cmake .. -DUSE_LOCKFREE=ON && make
```

## Running

```bash
# normal mode — exponential inter-arrival times, lambda=10000/sec/generator
./logger

# stress mode — no sleep, maximum pressure on logger
./logger --stress
```

---

## Project Structure

```
├── include/
│   ├── logger.h        # unified header, switches on USE_LOCKFREE
│   └── queue.h         # MPMC lock-free queue
├── src/
│   ├── main.cpp        # benchmark — 7 generators, Poisson arrivals
│   ├── logger_mutex.cpp
│   ├── logger_lockfree.cpp
│   └── queue.cpp
└── CMakeLists.txt
```
