#include "Metronome.h"

#include <algorithm>
#include <cmath>

namespace daw
{
namespace
{
constexpr double twoPi = 6.283185307179586476925286766559;

void buildClick (std::vector<float>& destination, double sampleRate, double frequency, float amplitude)
{
    const int length = std::max (1, static_cast<int> (sampleRate * 0.035));

    destination.resize (static_cast<size_t> (length));

    for (int i = 0; i < length; ++i)
    {
        const double t = static_cast<double> (i) / sampleRate;
        const float envelope = static_cast<float> (std::exp (-t * 55.0)
                                                   * (1.0 - std::exp (-t * 2500.0)));

        destination[static_cast<size_t> (i)] = amplitude * envelope
                                                 * static_cast<float> (std::sin (twoPi * frequency * t));
    }
}
} // namespace

void Metronome::prepare (double sampleRate) noexcept
{
    preparedSampleRate = sampleRate > 0.0 ? sampleRate : 48000.0;

    buildClick (accentClick, preparedSampleRate, 1760.0, 0.9f);
    buildClick (normalClick, preparedSampleRate, 1320.0, 0.7f);

    reset();
}

void Metronome::reset() noexcept
{
    clickCursor = -1;
    clickAccent = false;
    lastTransportPosition = -1;
}

void Metronome::process (float* const* outputChannels,
                         int numChannels,
                         int numSamples,
                         std::int64_t transportPosition,
                         bool transportPlaying,
                         double bpm) noexcept
{
    if (! enabled.load (std::memory_order_relaxed)
        || ! transportPlaying
        || bpm <= 0.0
        || preparedSampleRate <= 0.0
        || numSamples <= 0)
    {
        clickCursor = -1;
        return;
    }

    if (transportPosition < lastTransportPosition)
        reset();

    lastTransportPosition = transportPosition;

    const auto beatLength = static_cast<std::int64_t> (std::llround (preparedSampleRate * 60.0 / bpm));

    if (beatLength <= 0)
        return;

    const auto firstBeat = transportPosition > 0
                             ? (transportPosition + beatLength - 1) / beatLength
                             : std::int64_t (0);

    constexpr int maxClicksPerBlock = 32;
    int clickOffsets[maxClicksPerBlock] = {};
    bool clickAccents[maxClicksPerBlock] = {};
    int clickCount = 0;

    for (auto beat = firstBeat;
         beat * beatLength < transportPosition + numSamples && clickCount < maxClicksPerBlock;
         ++beat)
    {
        const auto boundary = beat * beatLength;

        if (boundary < transportPosition)
            continue;

        clickOffsets[clickCount] = static_cast<int> (boundary - transportPosition);
        clickAccents[clickCount] = (beat % 4 == 0);
        ++clickCount;
    }

    const float gain = volume.load (std::memory_order_relaxed);

    int cursor = clickCursor;
    bool accent = clickAccent;
    int nextClick = 0;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        if (nextClick < clickCount && sample == clickOffsets[nextClick])
        {
            cursor = 0;
            accent = clickAccents[nextClick];
            ++nextClick;
        }

        if (cursor >= 0 && cursor < static_cast<int> (accentClick.size()))
        {
            const auto& activeClick = accent ? accentClick : normalClick;
            const float value = activeClick[static_cast<size_t> (cursor)] * gain;

            for (int channel = 0; channel < numChannels; ++channel)
                if (outputChannels[channel] != nullptr)
                    outputChannels[channel][sample] += value;

            ++cursor;

            if (cursor >= static_cast<int> (activeClick.size()))
                cursor = -1;
        }
    }

    clickCursor = cursor;
    clickAccent = accent;
}
} // namespace daw
