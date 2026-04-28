#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <immintrin.h>
#include <thread>

#include "common/constants.h"

/*
MPSC Bounded Ring Queue 

- Lock-free multiple-producer, single-consumer (MPSC) queue
- Fixed-size circular buffer (power-of-2 capacity)
- Deterministic memory usage: no dynamic allocations during push
- Avoids ABA problems and ambiguity between full and empty states

Slot States (tracked via `seq`):
  State     seq value
  --------------------
  free      pos
  ready     pos + 1
  recycled  pos + Capacity

Mechanism:
1. Producers atomically claim slots via `tail.compare_exchange_weak`
2. Each slot spins until it becomes free (seq == pos)
3. Once claimed, producer writes `data` and publishes the slot (seq = pos + 1)
4. Consumer reads slots in order, marks them reusable by incrementing seq by Capacity

Features:
- No locks or mutexes
- Single consumer ensures simple, safe head progression
- Minimal spinning with optional `_mm_pause` and backoff to reduce tail latency
- Safe wraparound: tail never laps head
*/

template<typename T, size_t Capacity = 1<<16 >
class MPSC_bounded_ring {
    static_assert((Capacity & (Capacity - 1)) == 0,
                  "Capacity must be power of 2");
private:
    using value_type = T;
    static constexpr size_t MASK = Capacity - 1;

    struct Cell {
        std::atomic<size_t> seq;
        T data;
    };

    std::unique_ptr<Cell[]> buffer;    
    
    alignas(CACHELINE_SIZE) std::atomic<size_t> tail{0}; // producers
    alignas(CACHELINE_SIZE) size_t head = 0;             // single consumer

public:
    MPSC_bounded_ring() 
        : buffer(std::make_unique<Cell[]>(Capacity))
    {
        for (size_t i = 0; i < Capacity; ++i)
            buffer[i].seq.store(i, std::memory_order_relaxed);
    }

    // Non-copyable
    MPSC_bounded_ring(const MPSC_bounded_ring&) = delete;
    MPSC_bounded_ring& operator=(const MPSC_bounded_ring&) = delete;

    // multiple producers
    bool push(const T& item) {
        Cell* cell;
        size_t pos = tail.load(std::memory_order_relaxed);

        for (;;) {
            cell = &buffer[pos & MASK];

            size_t seq = cell->seq.load(std::memory_order_acquire);
            intptr_t diff = (intptr_t)seq - (intptr_t)pos;

            if (diff == 0) {
                // Try to claim this slot
                if (tail.compare_exchange_weak(
                        pos, pos + 1,
                        std::memory_order_relaxed))
                {
                    break;
                }
            }
            else if (diff < 0) {
                // Queue is full
                return false;
            }
            else {
                // Another producer moved ahead, retry
                pos = tail.load(std::memory_order_relaxed);
            }
        }

        // Write data
        cell->data = item;

        // Publish (release to consumer)
        cell->seq.store(pos + 1, std::memory_order_release);

        return true;
    }


    // single consumer
    bool pop(T& out) {
        Cell& cell = buffer[head & MASK];
        size_t seq = cell.seq.load(std::memory_order_acquire);

        if (seq != head + 1) return false; // empty

        out = cell.data;
        cell.seq.store(head + Capacity, std::memory_order_release);
        ++head;
        return true;
    }
};