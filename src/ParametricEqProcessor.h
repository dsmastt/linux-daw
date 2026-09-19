#pragma once

#include <atomic>

#include "AudioProcessor.h"

namespace daw
{
class ParametricEqProcessor final : public AudioProcessor
{
public:
    static constexpr int maxBands = 4;

    void setBandParameters (int band, double frequencyHz, float gainDb, float q) noexcept;

    double getBandFrequency (int band) const noexcept;
    float getBandGainDb (int band) const noexcept;
    float getBandQ (int band) const noexcept;

    void prepare (double sampleRate, int bufferSize) noexcept override;
    void process (float* const* channels, int numChannels, int numSamples) noexcept override;
    void reset() noexcept override;

private:
    static constexpr int maxChannels = 8;

    struct Coefficients
    {
        std::atomic<float> b0 { 1.0f };
        std::atomic<float> b1 { 0.0f };
        std::atomic<float> b2 { 0.0f };
        std::atomic<float> a1 { 0.0f };
        std::atomic<float> a2 { 0.0f };
    };

    void updateCoefficients (int band) noexcept;

    std::atomic<float> frequencies[maxBands] = { 100.0f, 500.0f, 2000.0f, 5000.0f };
    std::atomic<float> gains[maxBands] = { 0.0f, 0.0f, 0.0f, 0.0f };
    std::atomic<float> qValues[maxBands] = { 0.7071f, 0.7071f, 0.7071f, 0.7071f };
    Coefficients coefficients[maxBands];
    std::atomic<double> sampleRate { 0.0 };

    float z1[maxBands][maxChannels] = {};
    float z2[maxBands][maxChannels] = {};
};
} // namespace daw
