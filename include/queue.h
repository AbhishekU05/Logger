#pragma once
#include <atomic>
#include <vector>
#include <optional>

template <typename T>
class MPMCQueue {
public:
    explicit MPMCQueue(size_t capacity);

    bool push(const T& item);
    std::optional<T> pop();

private:
    struct Slot {
        std::atomic<bool> full;
        T data;
    };

    std::vector<Slot> buffer;
    size_t capacity;

    std::atomic<size_t> head;
    std::atomic<size_t> tail;
};
