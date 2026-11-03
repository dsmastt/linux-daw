#include "Tuner.h"

#include <algorithm>
#include <cmath>

namespace daw
{
namespace
{
constexpr float silenceThreshold = 0.0008f;
}

void Tuner::pushSamples (const float* samples, int numSamples) noexcept
{
    if (samples == nullptr || numSamples <= 0)
        return;

    auto position = writePosition.load (std::memory_order_relaxed);
    const auto mask = static_cast<std::uint64_t> (ringSize - 1);

    for (int i = 0; i < numSamples; ++i)
        ring[static_cast<size_t> ((position + static_cast<std::uint64_t> (i)) & mask)] = samples[i];

    writePosition.store (position + static_cast<std::uint64_t> (numSamples),
                         std::memory_order_release);
}

Tuner::Reading Tuner::analyse (double sampleRate) noexcept
{
    Reading result;

    if (sampleRate <= 0.0)
        return result;

    const int decimation = std::max (1, static_cast<int> (std::lround (sampleRate / 48000.0)));
    const auto requiredRaw = static_cast<std::uint64_t> (analysisWindow * decimation);

    const auto position = writePosition.load (std::memory_order_acquire);

    if (position < requiredRaw)
        return result;

    const auto mask = static_cast<std::uint64_t> (ringSize - 1);
    const auto start = position - requiredRaw;

    for (int i = 0; i < analysisWindow; ++i)
        analysis[static_cast<size_t> (i)] =
            ring[static_cast<size_t> ((start + static_cast<std::uint64_t> (i * decimation)) & mask)];

    double mean = 0.0;

    for (const auto value : analysis)
        mean += value;

    mean /= static_cast<double> (analysisWindow);

    double energy = 0.0;

    for (auto& value : analysis)
    {
        value = static_cast<float> (value - mean);
        energy += static_cast<double> (value) * value;
    }

    const auto rms = static_cast<float> (std::sqrt (energy / analysisWindow));

    if (rms < silenceThreshold)
        return result;

    const double effectiveSampleRate = sampleRate / decimation;
    const int minLag = std::max (2, static_cast<int> (std::floor (effectiveSampleRate / 1500.0)));
    const int maxLag = std::min (analysisWindow - 2,
                                 static_cast<int> (std::ceil (effectiveSampleRate / 55.0)));

    if (maxLag <= minLag + 2)
        return result;

    const int comparisonLength = analysisWindow - maxLag;

    if (yinDifference.size() < static_cast<size_t> (maxLag + 2))
        yinDifference.resize (static_cast<size_t> (maxLag + 2));

    yinDifference[0] = 1.0f;
    double runningSum = 0.0;

    for (int lag = 1; lag <= maxLag; ++lag)
    {
        double difference = 0.0;

        for (int i = 0; i < comparisonLength; ++i)
        {
            const double delta = static_cast<double> (analysis[static_cast<size_t> (i)])
                                 - analysis[static_cast<size_t> (i + lag)];
            difference += delta * delta;
        }

        runningSum += difference;

        yinDifference[static_cast<size_t> (lag)] =
            runningSum > 0.0
                ? static_cast<float> (difference * lag / runningSum)
                : 1.0f;
    }

    constexpr float threshold = 0.15f;
    int period = -1;

    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        if (yinDifference[static_cast<size_t> (lag)] < threshold)
        {
            period = lag;
            break;
        }
    }

    if (period < 0)
    {
        float bestValue = 1.0e9f;

        for (int lag = minLag; lag <= maxLag; ++lag)
        {
            const auto value = yinDifference[static_cast<size_t> (lag)];

            if (value < bestValue)
            {
                bestValue = value;
                period = lag;
            }
        }

        if (period < 0 || bestValue > 0.35f)
            return result;
    }

    while (period + 1 <= maxLag
           && yinDifference[static_cast<size_t> (period + 1)]
                < yinDifference[static_cast<size_t> (period)])
        ++period;

    while (period - 1 >= minLag
           && yinDifference[static_cast<size_t> (period - 1)]
                < yinDifference[static_cast<size_t> (period)])
        --period;

    float refinedPeriod = static_cast<float> (period);

    if (period > minLag && period < maxLag)
    {
        const auto a = yinDifference[static_cast<size_t> (period - 1)];
        const auto b = yinDifference[static_cast<size_t> (period)];
        const auto c = yinDifference[static_cast<size_t> (period + 1)];
        const auto denominator = a - 2.0f * b + c;

        if (std::abs (denominator) > 1.0e-9f)
            refinedPeriod += juce::jlimit (-1.0f, 1.0f, 0.5f * (a - c) / denominator);
    }

    if (refinedPeriod <= 0.0f)
        return result;

    const auto frequency = static_cast<float> (effectiveSampleRate / refinedPeriod);

    if (frequency < 55.0f || frequency > 1500.0f)
        return result;

    result.hasSignal = true;
    result.frequencyHz = frequency;
    result.confidence = juce::jlimit (0.0f, 1.0f,
                                      1.0f - yinDifference[static_cast<size_t> (period)]);

    return result;
}

void Tuner::reset() noexcept
{
    ring.fill (0.0f);
    writePosition.store (0, std::memory_order_release);
}

juce::String Tuner::getNoteName (float frequencyHz, float& centsOffset, int& octave)
{
    static const char* const noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

    if (frequencyHz <= 0.0f)
    {
        centsOffset = 0.0f;
        octave = 0;
        return "-";
    }

    const auto midiNote = 69.0 + 12.0 * std::log2 (static_cast<double> (frequencyHz) / 440.0);
    const auto nearestNote = static_cast<int> (std::lround (midiNote));
    const auto noteIndex = (nearestNote % 12 + 12) % 12;

    octave = nearestNote / 12 - 1;

    const auto targetFrequency = 440.0 * std::pow (2.0, (nearestNote - 69) / 12.0);
    centsOffset = static_cast<float> (1200.0 * std::log2 (static_cast<double> (frequencyHz) / targetFrequency));

    return noteNames[noteIndex];
}
} // namespace daw
