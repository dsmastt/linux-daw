#pragma once

#include <atomic>
#include <vector>

#include "AudioProcessor.h"

namespace daw
{
class ReverbProcessor final : public AudioProcessor
{
public:
    static constexpr int maxChannels = 2;
    static constexpr int numCombs = 8;
    static constexpr int numAllPasses = 2;

    void setWetDry (float wetDry) noexcept;
    float getWetDry() const noexcept;

    void setDecay (float decay) noexcept;
    float getDecay() const noexcept;

    void setSize (float size) noexcept;
    float getSize() const noexcept;

    void prepare (double sampleRate, int bufferSize) noexcept override;
    void process (float* const* channels, int numChannels, int numSamples) noexcept override;
    void reset() noexcept override;

private:
    struct DelayState
    {
        std::vector<float> buffer;
        int index = 0;
    };

    std::atomic<float> wetDry { 0.2f };
    std::atomic<float> decay { 0.5f };
    std::atomic<float> size { 0.5f };

    double sampleRate = 0.0;
    int bufferMask = 0;
    DelayState combStates[numCombs][maxChannels];
    DelayState allPassStates[numAllPasses][maxChannels];
};
} // namespace daw
