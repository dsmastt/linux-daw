#pragma once

#include <atomic>

#include "AudioProcessor.h"

namespace daw
{
class NoiseGateProcessor final : public AudioProcessor
{
public:
    juce::String getName() const override { return "Gate"; }
    void setThresholdDb (float thresholdDb) noexcept;
    float getThresholdDb() const noexcept;

    void setAttackMs (float attackMs) noexcept;
    float getAttackMs() const noexcept;

    void setReleaseMs (float releaseMs) noexcept;
    float getReleaseMs() const noexcept;

    void prepare (double sampleRate, int bufferSize) noexcept override;
    void process (float* const* channels, int numChannels, int numSamples) noexcept override;
    void reset() noexcept override;

private:
    static constexpr int maxChannels = 8;

    static float dBToLinear (float gainDb) noexcept;
    static float coefficientForTime (float milliseconds, double sampleRate) noexcept;

    std::atomic<float> thresholdDb { -60.0f };
    std::atomic<float> attackMs { 1.0f };
    std::atomic<float> releaseMs { 100.0f };

    double sampleRate = 0.0;
    int numChannels = 0;
    float envelope[maxChannels] = {};
    float gain[maxChannels] = {};
};
} // namespace daw
