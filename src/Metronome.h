#pragma once

#include <atomic>
#include <cstdint>
#include <vector>

#include <juce_core/juce_core.h>

namespace daw
{
class Metronome
{
public:
    void prepare (double sampleRate) noexcept;
    void reset() noexcept;

    void setEnabled (bool shouldBeEnabled) noexcept { enabled.store (shouldBeEnabled, std::memory_order_relaxed); }
    bool isEnabled() const noexcept { return enabled.load (std::memory_order_relaxed); }

    void setVolume (float newVolume) noexcept
    {
        volume.store (juce::jlimit (0.0f, 1.0f, newVolume), std::memory_order_relaxed);
    }

    float getVolume() const noexcept { return volume.load (std::memory_order_relaxed); }

    void process (float* const* outputChannels,
                  int numChannels,
                  int numSamples,
                  std::int64_t transportPosition,
                  bool transportPlaying,
                  double bpm) noexcept;

private:
    std::atomic<bool> enabled { false };
    std::atomic<float> volume { 0.5f };

    std::vector<float> accentClick;
    std::vector<float> normalClick;

    double preparedSampleRate = 48000.0;
    int clickCursor = -1;
    bool clickAccent = false;
    std::int64_t lastTransportPosition = -1;
};
} // namespace daw
