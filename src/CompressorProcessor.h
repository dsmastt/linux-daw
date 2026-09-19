#pragma once

#include <atomic>

#include "AudioProcessor.h"

namespace daw
{
class CompressorProcessor final : public AudioProcessor
{
public:
    void setThresholdDb (float thresholdDb) noexcept;
    float getThresholdDb() const noexcept;

    void setRatio (float ratio) noexcept;
    float getRatio() const noexcept;

    void setAttackMs (float attackMs) noexcept;
    float getAttackMs() const noexcept;

    void setReleaseMs (float releaseMs) noexcept;
    float getReleaseMs() const noexcept;

    void setMakeupGainDb (float makeupGainDb) noexcept;
    float getMakeupGainDb() const noexcept;

    void prepare (double sampleRate, int bufferSize) noexcept override;
    void process (float* const* channels, int numChannels, int numSamples) noexcept override;
    void reset() noexcept override;

private:
    static constexpr int maxChannels = 8;

    static float dBToLinear (float gainDb) noexcept;
    static float coefficientForTime (float milliseconds, double sampleRate) noexcept;

    std::atomic<float> thresholdDb { -10.0f };
    std::atomic<float> ratio { 4.0f };
    std::atomic<float> attackMs { 5.0f };
    std::atomic<float> releaseMs { 100.0f };
    std::atomic<float> makeupGainDb { 0.0f };

    double sampleRate = 0.0;
    float envelope[maxChannels] = {};
    float gain[maxChannels] = {};
};
} // namespace daw
