#include "ParametricEqProcessor.h"

#include <cmath>

namespace daw
{
void ParametricEqProcessor::setBandParameters (int band, double frequencyHz, float gainDb, float q) noexcept
{
    if (band < 0 || band >= maxBands)
        return;

    frequencies[band].store (static_cast<float> (frequencyHz), std::memory_order_relaxed);
    gains[band].store (gainDb, std::memory_order_relaxed);
    qValues[band].store (q, std::memory_order_relaxed);

    updateCoefficients (band);
}

double ParametricEqProcessor::getBandFrequency (int band) const noexcept
{
    return band >= 0 && band < maxBands ? frequencies[band].load() : 0.0;
}

float ParametricEqProcessor::getBandGainDb (int band) const noexcept
{
    return band >= 0 && band < maxBands ? gains[band].load() : 0.0f;
}

float ParametricEqProcessor::getBandQ (int band) const noexcept
{
    return band >= 0 && band < maxBands ? qValues[band].load() : 0.0f;
}

void ParametricEqProcessor::prepare (double newSampleRate, int) noexcept
{
    sampleRate.store (newSampleRate, std::memory_order_relaxed);

    for (int band = 0; band < maxBands; ++band)
        updateCoefficients (band);
}

void ParametricEqProcessor::reset() noexcept
{
    for (int band = 0; band < maxBands; ++band)
        for (int channel = 0; channel < maxChannels; ++channel)
        {
            z1[band][channel] = 0.0f;
            z2[band][channel] = 0.0f;
        }
}

void ParametricEqProcessor::process (float* const* channels, int numChannels, int numSamples) noexcept
{
    const double rate = sampleRate.load (std::memory_order_relaxed);

    if (rate <= 0.0 || numChannels > maxChannels)
        return;

    for (int band = 0; band < maxBands; ++band)
    {
        if (std::abs (gains[band].load (std::memory_order_relaxed)) < 0.01f)
            continue;

        const auto b0 = coefficients[band].b0.load (std::memory_order_relaxed);
        const auto b1 = coefficients[band].b1.load (std::memory_order_relaxed);
        const auto b2 = coefficients[band].b2.load (std::memory_order_relaxed);
        const auto a1 = coefficients[band].a1.load (std::memory_order_relaxed);
        const auto a2 = coefficients[band].a2.load (std::memory_order_relaxed);

        for (int channel = 0; channel < numChannels; ++channel)
        {
            if (channels[channel] == nullptr)
                continue;

            float zz1 = z1[band][channel];
            float zz2 = z2[band][channel];

            for (int i = 0; i < numSamples; ++i)
            {
                const float input = channels[channel][i];
                const float output = b0 * input + zz1;

                zz1 = b1 * input - a1 * output + zz2;
                zz2 = b2 * input - a2 * output;

                channels[channel][i] = output;
            }

            z1[band][channel] = zz1;
            z2[band][channel] = zz2;
        }
    }
}

void ParametricEqProcessor::updateCoefficients (int band) noexcept
{
    if (band < 0 || band >= maxBands)
        return;

    const double rate = sampleRate.load (std::memory_order_relaxed);
    const float gainDb = gains[band].load (std::memory_order_relaxed);
    const float frequency = frequencies[band].load (std::memory_order_relaxed);
    const float q = qValues[band].load (std::memory_order_relaxed);

    if (rate <= 0.0 || frequency <= 0.0f || q <= 0.0f || std::abs (gainDb) < 0.01f)
    {
        coefficients[band].b0.store (1.0f, std::memory_order_relaxed);
        coefficients[band].b1.store (0.0f, std::memory_order_relaxed);
        coefficients[band].b2.store (0.0f, std::memory_order_relaxed);
        coefficients[band].a1.store (0.0f, std::memory_order_relaxed);
        coefficients[band].a2.store (0.0f, std::memory_order_relaxed);
        return;
    }

    const double w0 = 2.0 * 3.14159265358979323846 * frequency / rate;
    const float cosW0 = static_cast<float> (std::cos (w0));
    const float sinW0 = static_cast<float> (std::sin (w0));
    const float A = std::pow (10.0f, gainDb * 0.025f);
    const float alpha = sinW0 / (2.0f * q);

    const float b0 = 1.0f + alpha * A;
    const float b1 = -2.0f * cosW0;
    const float b2 = 1.0f - alpha * A;
    const float a0 = 1.0f + alpha / A;
    const float a1 = -2.0f * cosW0;
    const float a2 = 1.0f - alpha / A;

    coefficients[band].b0.store (b0 / a0, std::memory_order_relaxed);
    coefficients[band].b1.store (b1 / a0, std::memory_order_relaxed);
    coefficients[band].b2.store (b2 / a0, std::memory_order_relaxed);
    coefficients[band].a1.store (a1 / a0, std::memory_order_relaxed);
    coefficients[band].a2.store (a2 / a0, std::memory_order_relaxed);
}
} // namespace daw
