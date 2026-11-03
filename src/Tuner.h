#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

#include <juce_core/juce_core.h>

namespace daw
{
class Tuner
{
public:
    struct Reading
    {
        bool hasSignal = false;
        float frequencyHz = 0.0f;
        float confidence = 0.0f;
    };

    static constexpr int ringSize = 32768;
    static constexpr int analysisWindow = 2048;

    void pushSamples (const float* samples, int numSamples) noexcept;
    Reading analyse (double sampleRate) noexcept;
    void reset() noexcept;

    static juce::String getNoteName (float frequencyHz, float& centsOffset, int& octave);

private:
    std::array<float, ringSize> ring {};
    std::atomic<std::uint64_t> writePosition { 0 };

    std::array<float, analysisWindow> analysis {};
    std::vector<float> yinDifference;
};
} // namespace daw
