#include "DistortionProcessor.h"

#include <algorithm>
#include <cmath>

namespace daw
{
void DistortionProcessor::setDriveDb (float newDriveDb) noexcept
{
    driveDb.store (newDriveDb, std::memory_order_relaxed);
}

float DistortionProcessor::getDriveDb() const noexcept
{
    return driveDb.load();
}

void DistortionProcessor::setTone (float newTone) noexcept
{
    tone.store (std::clamp (newTone, 0.0f, 1.0f), std::memory_order_relaxed);
}

float DistortionProcessor::getTone() const noexcept
{
    return tone.load();
}

void DistortionProcessor::setMode (DistortionMode newMode) noexcept
{
    mode.store (static_cast<int> (newMode), std::memory_order_relaxed);
}

DistortionMode DistortionProcessor::getMode() const noexcept
{
    return static_cast<DistortionMode> (mode.load());
}

void DistortionProcessor::prepare (double newSampleRate, int) noexcept
{
    sampleRate = newSampleRate;
}

void DistortionProcessor::reset() noexcept
{
    for (int channel = 0; channel < maxChannels; ++channel)
    {
        toneState[channel] = 0.0f;
        dcInputState[channel] = 0.0f;
        dcOutputState[channel] = 0.0f;
    }
}

void DistortionProcessor::process (float* const* channels, int numChannels, int numSamples) noexcept
{
    if (numChannels > maxChannels)
        numChannels = maxChannels;

    if (sampleRate <= 0.0)
        return;

    const float drive = dBToLinear (driveDb.load());
    const auto modeValue = static_cast<DistortionMode> (mode.load());

    const float toneValue = tone.load();
    const double toneCutoff = 800.0 * std::pow (10.0, 2.0 * toneValue);
    const float toneCoeff = static_cast<float> (1.0 - std::exp (-2.0 * 3.14159265358979323846 * toneCutoff / sampleRate));

    constexpr float dcCoeff = 0.995f;

    const auto processChannel = [&] (auto shaper)
    {
        for (int channel = 0; channel < numChannels; ++channel)
        {
            if (channels[channel] == nullptr)
                continue;

            float lpState = toneState[channel];
            float dcInput = dcInputState[channel];
            float dcOutput = dcOutputState[channel];

            for (int i = 0; i < numSamples; ++i)
            {
                const float input = channels[channel][i];

                // Classic DC blocker on the input: y[n] = x[n] - x[n-1] + R * y[n-1]
                const float dcBlocked = input - dcInput + dcCoeff * dcOutput;
                dcInput = input;
                dcOutput = dcBlocked;

                float sample = shaper (dcBlocked * drive);

                lpState += toneCoeff * (sample - lpState);

                channels[channel][i] = lpState;
            }

            toneState[channel] = lpState;
            dcInputState[channel] = dcInput;
            dcOutputState[channel] = dcOutput;
        }
    };

    switch (modeValue)
    {
        case DistortionMode::soft:
            processChannel ([] (float x) { return shapers::softClip (x); });
            break;
        case DistortionMode::hard:
            processChannel ([] (float x) { return shapers::hardClip (x); });
            break;
        case DistortionMode::cubic:
            processChannel ([] (float x) { return shapers::cubicClip (x); });
            break;
        case DistortionMode::asymmetric:
            processChannel ([] (float x) { return shapers::asymmetricClip (x); });
            break;
        default:
            break;
    }
}

float DistortionProcessor::dBToLinear (float gainDb) noexcept
{
    return std::pow (10.0f, gainDb * 0.05f);
}
} // namespace daw
