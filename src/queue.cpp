#include "queue.h"

template <typename T>
MPMCQueue<T>::MPMCQueue(size_t cap)
    : buffer(cap), capacity(cap), head(0), tail(0) {
    for (auto& s : buffer) {
        s.full.store(false, std::memory_order_relaxed);
    }
}

template <typename T>
bool MPMCQueue<T>::push(const T& item) {
    size_t t = tail.fetch_add(1, std::memory_order_relaxed);
    Slot& slot = buffer[t % capacity];

    if (slot.full.load(std::memory_order_acquire)) {
        return false; // queue full
    }

    slot.data = item;
    slot.full.store(true, std::memory_order_release);
    return true;
}

template <typename T>
std::optional<T> MPMCQueue<T>::pop() {
    size_t h = head.fetch_add(1, std::memory_order_relaxed);
    Slot& slot = buffer[h % capacity];

    if (!slot.full.load(std::memory_order_acquire)) {
        return std::nullopt;
    }

    T item = slot.data;
    slot.full.store(false, std::memory_order_release);
    return item;
}

// Explicit instantiation
template class MPMCQueue<std::string>;
