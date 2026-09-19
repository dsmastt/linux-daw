#include "CompressorProcessor.h"

#include <cmath>

namespace daw
{
void CompressorProcessor::setThresholdDb (float newThresholdDb) noexcept
{
    thresholdDb.store (newThresholdDb, std::memory_order_relaxed);
}

float CompressorProcessor::getThresholdDb() const noexcept
{
    return thresholdDb.load();
}

void CompressorProcessor::setRatio (float newRatio) noexcept
{
    ratio.store (newRatio, std::memory_order_relaxed);
}

float CompressorProcessor::getRatio() const noexcept
{
    return ratio.load();
}

void CompressorProcessor::setAttackMs (float newAttackMs) noexcept
{
    attackMs.store (newAttackMs, std::memory_order_relaxed);
}

float CompressorProcessor::getAttackMs() const noexcept
{
    return attackMs.load();
}

void CompressorProcessor::setReleaseMs (float newReleaseMs) noexcept
{
    releaseMs.store (newReleaseMs, std::memory_order_relaxed);
}

float CompressorProcessor::getReleaseMs() const noexcept
{
    return releaseMs.load();
}

void CompressorProcessor::setMakeupGainDb (float newMakeupGainDb) noexcept
{
    makeupGainDb.store (newMakeupGainDb, std::memory_order_relaxed);
}

float CompressorProcessor::getMakeupGainDb() const noexcept
{
    return makeupGainDb.load();
}

void CompressorProcessor::prepare (double newSampleRate, int) noexcept
{
    sampleRate = newSampleRate;
}

void CompressorProcessor::reset() noexcept
{
    for (int channel = 0; channel < maxChannels; ++channel)
    {
        envelope[channel] = 0.0f;
        gain[channel] = 1.0f;
    }
}

void CompressorProcessor::process (float* const* channels, int numChannels, int numSamples) noexcept
{
    if (numChannels > maxChannels)
        numChannels = maxChannels;

    const float attackCoeff  = coefficientForTime (attackMs.load(),  sampleRate);
    const float releaseCoeff = coefficientForTime (releaseMs.load(), sampleRate);
    const float threshold    = dBToLinear (thresholdDb.load());
    const float makeup       = dBToLinear (makeupGainDb.load());
    const float ratioValue   = ratio.load() > 1.0f ? ratio.load() : 1.0f;
    const float ratioExponent = (1.0f / ratioValue) - 1.0f;

    for (int channel = 0; channel < numChannels; ++channel)
    {
        if (channels[channel] == nullptr)
            continue;

        float env = envelope[channel];
        float compGain = gain[channel];

        for (int i = 0; i < numSamples; ++i)
        {
            const float absSample = std::abs (channels[channel][i]);

            env += (absSample > env ? attackCoeff : releaseCoeff) * (absSample - env);

            float targetGain = 1.0f;

            if (env > threshold && threshold > 0.0f)
                targetGain = std::pow (env / threshold, ratioExponent);

            targetGain *= makeup;

            compGain += (targetGain < compGain ? attackCoeff : releaseCoeff) * (targetGain - compGain);

            channels[channel][i] *= compGain;
        }

        envelope[channel] = env;
        gain[channel] = compGain;
    }
}

float CompressorProcessor::dBToLinear (float gainDb) noexcept
{
    return std::pow (10.0f, gainDb * 0.05f);
}

float CompressorProcessor::coefficientForTime (float milliseconds, double rate) noexcept
{
    if (milliseconds <= 0.0f || rate <= 0.0)
        return 1.0f;

    return static_cast<float> (1.0 - std::exp (-1.0 / (milliseconds * 0.001 * rate)));
}
} // namespace daw
