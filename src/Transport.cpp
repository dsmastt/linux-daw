#include "Transport.h"

namespace daw
{
void Transport::play() noexcept
{
    state.store (static_cast<int> (TransportState::playing), std::memory_order_relaxed);
}

void Transport::pause() noexcept
{
    state.store (static_cast<int> (TransportState::paused), std::memory_order_relaxed);
}

void Transport::stop() noexcept
{
    state.store (static_cast<int> (TransportState::stopped), std::memory_order_relaxed);
    position.store (0, std::memory_order_relaxed);
}
} // namespace daw
