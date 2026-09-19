#include "ReverbProcessor.h"

#include <algorithm>
#include <cmath>

#include <juce_core/juce_core.h>

namespace daw
{
namespace
{
constexpr int combBaseDelays[ReverbProcessor::numCombs] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
constexpr int allPassBaseDelays[ReverbProcessor::numAllPasses] = { 225, 556 };
constexpr float allPassFeedback = 0.5f;
constexpr int stereoOffset = 23;
constexpr float wetOutputScale = 0.25f;
} // namespace

void ReverbProcessor::setWetDry (float newWetDry) noexcept
{
    wetDry.store (std::clamp (newWetDry, 0.0f, 1.0f), std::memory_order_relaxed);
}

float ReverbProcessor::getWetDry() const noexcept
{
    return wetDry.load();
}

void ReverbProcessor::setDecay (float newDecay) noexcept
{
    decay.store (std::clamp (newDecay, 0.0f, 1.0f), std::memory_order_relaxed);
}

float ReverbProcessor::getDecay() const noexcept
{
    return decay.load();
}

void ReverbProcessor::setSize (float newSize) noexcept
{
    size.store (std::clamp (newSize, 0.0f, 1.0f), std::memory_order_relaxed);
}

float ReverbProcessor::getSize() const noexcept
{
    return size.load();
}

void ReverbProcessor::prepare (double newSampleRate, int) noexcept
{
    sampleRate = newSampleRate;

    const double rateFactor = sampleRate / 44100.0;
    const int maxDelay = static_cast<int> (combBaseDelays[numCombs - 1] * rateFactor * 1.3) + stereoOffset;

    auto bufferSize = juce::nextPowerOfTwo (std::max (2, maxDelay));

    bufferMask = bufferSize - 1;

    for (auto& stateRow : combStates)
        for (auto& state : stateRow)
            state.buffer.assign (static_cast<size_t> (bufferSize), 0.0f);

    for (auto& stateRow : allPassStates)
        for (auto& state : stateRow)
            state.buffer.assign (static_cast<size_t> (bufferSize), 0.0f);
}

void ReverbProcessor::reset() noexcept
{
    for (auto& stateRow : combStates)
        for (auto& state : stateRow)
        {
            std::fill (state.buffer.begin(), state.buffer.end(), 0.0f);
            state.index = 0;
        }

    for (auto& stateRow : allPassStates)
        for (auto& state : stateRow)
        {
            std::fill (state.buffer.begin(), state.buffer.end(), 0.0f);
            state.index = 0;
        }
}

void ReverbProcessor::process (float* const* channels, int numChannelsIn, int numSamples) noexcept
{
    if (numChannelsIn > maxChannels)
        numChannelsIn = maxChannels;

    if (sampleRate <= 0.0 || bufferMask <= 0)
        return;

    const float feedback = 0.6f + 0.37f * decay.load();
    const float wetAmount = wetDry.load();
    const float dryAmount = 1.0f - wetAmount;
    const float sizeScale = 0.75f + 0.5f * size.load();

    const double rateFactor = sampleRate / 44100.0;

    int combDelays[numCombs][maxChannels];
    int allPassDelays[numAllPasses];

    for (int comb = 0; comb < numCombs; ++comb)
    {
        const int baseDelay = static_cast<int> (combBaseDelays[comb] * rateFactor * sizeScale);
        combDelays[comb][0] = baseDelay;
        combDelays[comb][1] = baseDelay + stereoOffset;
    }

    for (int allPass = 0; allPass < numAllPasses; ++allPass)
        allPassDelays[allPass] = static_cast<int> (allPassBaseDelays[allPass] * rateFactor);

    const int mask = bufferMask;

    for (int channel = 0; channel < numChannelsIn; ++channel)
    {
        if (channels[channel] == nullptr)
            continue;

        for (int n = 0; n < numSamples; ++n)
        {
            const float input = channels[channel][n];

            float combSum = 0.0f;

            for (int comb = 0; comb < numCombs; ++comb)
            {
                auto& state = combStates[comb][channel];

                const int readIndex = (state.index - combDelays[comb][channel]) & mask;
                const float delayed = state.buffer[static_cast<size_t> (readIndex)];

                state.buffer[static_cast<size_t> (state.index)] = input + delayed * feedback;
                state.index = (state.index + 1) & mask;

                combSum += delayed;
            }

            float processed = combSum;

            for (int allPass = 0; allPass < numAllPasses; ++allPass)
            {
                auto& state = allPassStates[allPass][channel];

                const int readIndex = (state.index - allPassDelays[allPass]) & mask;
                const float delayed = state.buffer[static_cast<size_t> (readIndex)];

                state.buffer[static_cast<size_t> (state.index)] = processed + delayed * allPassFeedback;
                state.index = (state.index + 1) & mask;

                processed = -processed + delayed;
            }

            channels[channel][n] = dryAmount * input + wetAmount * processed * wetOutputScale;
        }
    }
}
} // namespace daw
