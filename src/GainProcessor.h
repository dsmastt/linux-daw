#pragma once

#include <atomic>

#include "AudioProcessor.h"

namespace daw
{
class GainProcessor final : public AudioProcessor
{
public:
    void setGainDb (float gainDb) noexcept;
    float getGainDb() const noexcept;

    void setSampleRate (double sampleRate) noexcept;

    void prepare (double sampleRate, int bufferSize) noexcept override;
    void process (float* const* channels, int numChannels, int numSamples) noexcept override;
    void reset() noexcept override;

private:
    static float gainDbToLinear (float gainDb) noexcept;

    std::atomic<float> targetGain { 1.0f };
    float targetGainDb = 0.0f;
    float currentGain = 1.0f;
    float smoothingCoefficient = 0.0f;
};
} // namespace daw
