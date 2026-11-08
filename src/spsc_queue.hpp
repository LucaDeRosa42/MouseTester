#pragma once
// Coda lock-free a singolo produttore / singolo consumatore.
// Il thread di cattura scrive, il thread dell'interfaccia legge:
// nessun mutex, quindi la cattura non viene mai bloccata.

#include <atomic>
#include <cstddef>
#include <memory>

template <typename T, size_t Capacity>
class SpscQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity deve essere una potenza di 2");

public:
    bool push(const T& value) {
        const size_t head = head_.load(std::memory_order_relaxed);
        const size_t tail = tail_.load(std::memory_order_acquire);
        if (head - tail == Capacity) return false; // piena
        buffer_[head & kMask] = value;
        head_.store(head + 1, std::memory_order_release);
        return true;
    }

    bool pop(T& out) {
        const size_t tail = tail_.load(std::memory_order_relaxed);
        const size_t head = head_.load(std::memory_order_acquire);
        if (tail == head) return false; // vuota
        out = buffer_[tail & kMask];
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

private:
    static constexpr size_t kMask = Capacity - 1;
    std::unique_ptr<T[]> buffer_ = std::make_unique<T[]>(Capacity);
    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};
};
