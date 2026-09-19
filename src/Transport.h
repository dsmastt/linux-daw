#pragma once

#include <atomic>
#include <cstdint>

#include <juce_core/juce_core.h>

namespace daw
{
enum class TransportState
{
    stopped,
    playing,
    paused,
};

class Transport
{
public:
    Transport() = default;

    void play() noexcept;
    void pause() noexcept;
    void stop() noexcept;

    TransportState getState() const noexcept { return static_cast<TransportState> (state.load()); }
    bool isPlaying() const noexcept { return getState() == TransportState::playing; }

    std::int64_t getPositionSamples() const noexcept { return position.load(); }
    void setPositionSamples (std::int64_t samples) noexcept { position.store (samples); }

    void setTempoBpm (double bpm) noexcept { tempoBpm.store (bpm, std::memory_order_relaxed); }
    double getTempoBpm() const noexcept { return tempoBpm.load(); }

    void advance (int numSamples) noexcept { position.fetch_add (numSamples); }

private:
    std::atomic<int> state { static_cast<int> (TransportState::stopped) };
    std::atomic<std::int64_t> position { 0 };
    std::atomic<double> tempoBpm { 120.0 };
};
} // namespace daw
