#include "NoiseGateProcessor.h"

#include <cmath>

namespace daw
{
void NoiseGateProcessor::setThresholdDb (float newThresholdDb) noexcept
{
    thresholdDb.store (newThresholdDb, std::memory_order_relaxed);
}

float NoiseGateProcessor::getThresholdDb() const noexcept
{
    return thresholdDb.load();
}

void NoiseGateProcessor::setAttackMs (float newAttackMs) noexcept
{
    attackMs.store (newAttackMs, std::memory_order_relaxed);
}

float NoiseGateProcessor::getAttackMs() const noexcept
{
    return attackMs.load();
}

void NoiseGateProcessor::setReleaseMs (float newReleaseMs) noexcept
{
    releaseMs.store (newReleaseMs, std::memory_order_relaxed);
}

float NoiseGateProcessor::getReleaseMs() const noexcept
{
    return releaseMs.load();
}

void NoiseGateProcessor::prepare (double newSampleRate, int) noexcept
{
    sampleRate = newSampleRate;
}

void NoiseGateProcessor::reset() noexcept
{
    for (int channel = 0; channel < maxChannels; ++channel)
    {
        envelope[channel] = 0.0f;
        gain[channel] = 0.0f;
    }

    numChannels = 0;
}

void NoiseGateProcessor::process (float* const* channels, int numChannelsIn, int numSamples) noexcept
{
    if (numChannelsIn > maxChannels)
        numChannelsIn = maxChannels;

    numChannels = numChannelsIn;

    const float attackCoeff  = coefficientForTime (attackMs.load(),  sampleRate);
    const float releaseCoeff = coefficientForTime (releaseMs.load(), sampleRate);
    const float threshold    = dBToLinear (thresholdDb.load());

    for (int channel = 0; channel < numChannelsIn; ++channel)
    {
        if (channels[channel] == nullptr)
            continue;

        float env = envelope[channel];
        float gateGain = gain[channel];

        for (int i = 0; i < numSamples; ++i)
        {
            const float absSample = std::abs (channels[channel][i]);

            env += (absSample > env ? attackCoeff : releaseCoeff) * (absSample - env);

            const float target = env > threshold ? 1.0f : 0.0f;
            gateGain += (target > gateGain ? attackCoeff : releaseCoeff) * (target - gateGain);

            channels[channel][i] *= gateGain;
        }

        envelope[channel] = env;
        gain[channel] = gateGain;
    }
}

float NoiseGateProcessor::dBToLinear (float gainDb) noexcept
{
    return std::pow (10.0f, gainDb * 0.05f);
}

float NoiseGateProcessor::coefficientForTime (float milliseconds, double rate) noexcept
{
    if (milliseconds <= 0.0f || rate <= 0.0)
        return 1.0f;

    return static_cast<float> (1.0 - std::exp (-1.0 / (milliseconds * 0.001 * rate)));
}
} // namespace daw
