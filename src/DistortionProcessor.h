#pragma once

#include <atomic>

#include "AudioProcessor.h"
#include "DistortionShaper.h"

namespace daw
{
class DistortionProcessor final : public AudioProcessor
{
public:
    juce::String getName() const override { return "Distortion"; }
    void setDriveDb (float driveDb) noexcept;
    float getDriveDb() const noexcept;

    void setTone (float tone) noexcept;
    float getTone() const noexcept;

    void setMode (DistortionMode mode) noexcept;
    DistortionMode getMode() const noexcept;

    void prepare (double sampleRate, int bufferSize) noexcept override;
    void process (float* const* channels, int numChannels, int numSamples) noexcept override;
    void reset() noexcept override;

private:
    static constexpr int maxChannels = 8;

    static float dBToLinear (float gainDb) noexcept;

    std::atomic<float> driveDb { 12.0f };
    std::atomic<float> tone { 0.5f };
    std::atomic<int> mode { static_cast<int> (DistortionMode::soft) };

    double sampleRate = 0.0;
    float toneState[maxChannels] = {};
    float dcState[maxChannels] = {};
};
} // namespace daw
