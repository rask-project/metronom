#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <optional>

// Lock-free queue for one producer thread and one consumer thread; a push to a full queue is dropped.
template <typename T, std::size_t Capacity> class SpscQueue
{
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");

public:
    bool push(const T &value) noexcept
    {
        const std::size_t head = m_head.load(std::memory_order_relaxed);
        if (head - m_tail.load(std::memory_order_acquire) == Capacity) {
            return false;
        }

        m_items[head & (Capacity - 1)] = value;
        m_head.store(head + 1, std::memory_order_release);
        return true;
    }

    std::optional<T> pop() noexcept
    {
        const std::size_t tail = m_tail.load(std::memory_order_relaxed);
        if (tail == m_head.load(std::memory_order_acquire)) {
            return std::nullopt;
        }

        const T value = m_items[tail & (Capacity - 1)];
        m_tail.store(tail + 1, std::memory_order_release);
        return value;
    }

    // Only while neither thread uses the queue.
    void clear() noexcept { m_tail.store(m_head.load(std::memory_order_relaxed), std::memory_order_relaxed); }

private:
    std::array<T, Capacity> m_items{};
    std::atomic<std::size_t> m_head{0};
    std::atomic<std::size_t> m_tail{0};
};
