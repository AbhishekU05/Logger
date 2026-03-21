#pragma once
#include <atomic>
#include <vector>
#include <optional>
#include <cstddef>

template <typename T>
class MPMCQueue {
public:
    explicit MPMCQueue(size_t capacity);

    bool push(T item);
    std::optional<T> pop();

    size_t size() const { return capacity; }

private:
    struct Slot {
        std::atomic<size_t> sequence;
        T data;
    };

    std::vector<Slot> buffer;
    size_t capacity;

    // keep producer and consumer counters on separate cache lines
    // to avoid false sharing
    alignas(64) std::atomic<size_t> tail;
    alignas(64) std::atomic<size_t> head;
};
