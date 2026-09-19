#pragma once

#include <atomic>

namespace daw
{
template <typename ValueType>
void updateAtomicMax (std::atomic<ValueType>& target, ValueType value)
{
    auto current = target.load (std::memory_order_relaxed);

    while (value > current && !target.compare_exchange_weak (current, value, std::memory_order_relaxed))
    {
    }
}
} // namespace daw
