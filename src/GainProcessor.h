#pragma once

#include <atomic>

namespace daw
{
class GainProcessor
{
public:
    void setGainDb (float gainDb) noexcept;
    float getGainDb() const noexcept;

    void setSampleRate (double sampleRate) noexcept;
    void reset() noexcept;

    void process (float* const* channels, int numChannels, int numSamples) noexcept;

private:
    static float gainDbToLinear (float gainDb) noexcept;

    std::atomic<float> targetGain { 1.0f };
    float targetGainDb = 0.0f;
    float currentGain = 1.0f;
    float smoothingCoefficient = 0.0f;
};
} // namespace daw
