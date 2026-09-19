#include "GainProcessor.h"

#include <cmath>

#include <juce_audio_basics/juce_audio_basics.h>

namespace daw
{
void GainProcessor::setGainDb (float gainDb) noexcept
{
    targetGainDb = gainDb;
    targetGain.store (gainDbToLinear (gainDb), std::memory_order_relaxed);
}

float GainProcessor::getGainDb() const noexcept
{
    return targetGainDb;
}

void GainProcessor::setSampleRate (double sampleRate) noexcept
{
    constexpr double smoothingTimeSeconds = 0.005;

    if (sampleRate > 0.0)
        smoothingCoefficient = static_cast<float> (1.0 - std::exp (-1.0 / (smoothingTimeSeconds * sampleRate)));
    else
        smoothingCoefficient = 1.0f;
}

void GainProcessor::reset() noexcept
{
    currentGain = targetGain.load (std::memory_order_relaxed);
}

void GainProcessor::process (float* const* channels, int numChannels, int numSamples) noexcept
{
    const float target = targetGain.load (std::memory_order_relaxed);

    if (juce::approximatelyEqual (currentGain, target))
    {
        if (juce::approximatelyEqual (currentGain, 1.0f))
            return;

        for (int channel = 0; channel < numChannels; ++channel)
            if (channels[channel] != nullptr)
                juce::FloatVectorOperations::multiply (channels[channel], target, numSamples);

        return;
    }

    const float coefficient = smoothingCoefficient;

    for (int channel = 0; channel < numChannels; ++channel)
    {
        if (channels[channel] == nullptr)
            continue;

        float gain = currentGain;

        for (int i = 0; i < numSamples; ++i)
        {
            gain += coefficient * (target - gain);
            channels[channel][i] *= gain;
        }

        if (channel == 0)
            currentGain = gain;
    }
}

float GainProcessor::gainDbToLinear (float gainDb) noexcept
{
    return std::pow (10.0f, gainDb * 0.05f);
}
} // namespace daw
