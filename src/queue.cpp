#include "queue.h"
#include <stdexcept>

template <typename T>
MPMCQueue<T>::MPMCQueue(size_t cap)
    : buffer(cap), capacity(cap), tail(0), head(0) {
    if ((cap & (cap - 1)) != 0)
        throw std::invalid_argument("capacity must be power of 2");

    for (size_t i = 0; i < cap; i++)
        buffer[i].sequence.store(i, std::memory_order_relaxed);
}

template <typename T>
bool MPMCQueue<T>::push(T item) {
    size_t pos;
    Slot* slot;

    while (true) {
        pos = tail.load(std::memory_order_relaxed);
        slot = &buffer[pos & (capacity - 1)];
        size_t seq = slot->sequence.load(std::memory_order_acquire);
        intptr_t diff = (intptr_t)seq - (intptr_t)pos;

        if (diff == 0) {
            // slot is free, try to claim it
            if (tail.compare_exchange_weak(pos, pos + 1,
                    std::memory_order_relaxed))
                break;
        } else if (diff < 0) {
            // queue is full
            return false;
        }
        // diff > 0 means another producer claimed this slot, retry
    }

    slot->data = std::move(item);
    slot->sequence.store(pos + 1, std::memory_order_release);
    return true;
}

template <typename T>
std::optional<T> MPMCQueue<T>::pop() {
    size_t pos;
    Slot* slot;

    while (true) {
        pos = head.load(std::memory_order_relaxed);
        slot = &buffer[pos & (capacity - 1)];
        size_t seq = slot->sequence.load(std::memory_order_acquire);
        intptr_t diff = (intptr_t)seq - (intptr_t)(pos + 1);

        if (diff == 0) {
            // slot is full, try to claim it
            if (head.compare_exchange_weak(pos, pos + 1,
                    std::memory_order_relaxed))
                break;
        } else if (diff < 0) {
            // queue is empty
            return std::nullopt;
        }
        // diff > 0 means another consumer claimed this slot, retry
    }

    T item = std::move(slot->data);
    slot->sequence.store(pos + capacity, std::memory_order_release);
    return item;
}

template class MPMCQueue<std::string>;
