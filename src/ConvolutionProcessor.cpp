#include "ConvolutionProcessor.h"

#include <algorithm>
#include <cstring>

#include <juce_audio_formats/juce_audio_formats.h>

namespace daw
{
juce::String ConvolutionProcessor::loadImpulseResponse (const juce::File& file)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));

    if (reader == nullptr)
        return "Could not read impulse response file: " + file.getFullPathName();

    juce::AudioBuffer<float> buffer (static_cast<int> (reader->numChannels),
                                     static_cast<int> (reader->lengthInSamples));
    reader->read (&buffer, 0, static_cast<int> (reader->lengthInSamples), 0, true, true);

    std::vector<float> loaded (static_cast<size_t> (reader->lengthInSamples));
    std::copy_n (buffer.getReadPointer (0), loaded.size(), loaded.begin());

    impulseResponse = std::move (loaded);
    impulseResponseSampleRate = reader->sampleRate;
    hasImpulseResponse_.store (true, std::memory_order_release);
    partitionsReady = false;

    return {};
}

void ConvolutionProcessor::clearImpulseResponse()
{
    hasImpulseResponse_.store (false, std::memory_order_release);
    impulseResponse.clear();
    processedImpulseResponse.clear();
    irSpectra.clear();
    impulseResponseSampleRate = 0.0;
    partitionsReady = false;
    numPartitions = 0;
}

int ConvolutionProcessor::getImpulseResponseLength() const noexcept
{
    return processedImpulseResponse.empty()
             ? static_cast<int> (impulseResponse.size())
             : static_cast<int> (processedImpulseResponse.size());
}

void ConvolutionProcessor::prepare (double sampleRate, int bufferSize) noexcept
{
    buildPartitions (sampleRate, bufferSize);
}

void ConvolutionProcessor::reset() noexcept
{
    std::fill (inputHistory.begin(), inputHistory.end(), 0.0f);
    std::fill (fftBuffer.begin(), fftBuffer.end(), 0.0f);
    std::fill (accumulator.begin(), accumulator.end(), 0.0f);

    for (auto& history : channelHistories)
        std::fill (history.begin(), history.end(), 0.0f);

    numChannels = 0;
}

void ConvolutionProcessor::buildPartitions (double sampleRate, int bufferSize) noexcept
{
    partitionsReady = false;

    if (!hasImpulseResponse_.load() || impulseResponse.empty() || bufferSize <= 0 || sampleRate <= 0.0)
        return;

    std::vector<float> ir = impulseResponse;

    if (!juce::approximatelyEqual (impulseResponseSampleRate, sampleRate))
    {
        const double ratio = impulseResponseSampleRate / sampleRate;
        const auto outLength = std::max<size_t> (1, static_cast<size_t> (std::llround (ir.size() / ratio)));

        std::vector<float> resampled (outLength);
        juce::CatmullRomInterpolator interpolator;
        interpolator.process (ratio, ir.data(), resampled.data(), static_cast<int> (outLength));

        ir = std::move (resampled);
    }

    processedImpulseResponse = ir;

    partitionSize = bufferSize;
    fftSize = juce::nextPowerOfTwo (2 * partitionSize);
    numPartitions = static_cast<int> ((ir.size() + static_cast<size_t> (partitionSize) - 1) / static_cast<size_t> (partitionSize));

    const auto order = juce::findHighestSetBit (static_cast<uint32_t> (fftSize));

    fft = std::make_unique<juce::dsp::FFT> (order);

    irSpectra.assign (static_cast<size_t> (numPartitions) * 2 * static_cast<size_t> (fftSize), 0.0f);
    fftBuffer.assign (static_cast<size_t> (2 * fftSize), 0.0f);
    inputHistory.assign (static_cast<size_t> (fftSize), 0.0f);
    accumulator.assign (static_cast<size_t> (2 * fftSize), 0.0f);

    for (auto& history : channelHistories)
        history.assign (ir.size() + static_cast<size_t> (partitionSize), 0.0f);

    for (int partition = 0; partition < numPartitions; ++partition)
    {
        std::fill (fftBuffer.begin(), fftBuffer.end(), 0.0f);

        const auto offset = static_cast<size_t> (partition) * static_cast<size_t> (partitionSize);
        const auto count = std::min (ir.size() - offset, static_cast<size_t> (partitionSize));
        std::copy_n (ir.data() + offset, count, fftBuffer.begin());

        fft->performRealOnlyForwardTransform (fftBuffer.data());

        std::copy (fftBuffer.begin(), fftBuffer.end(),
                   irSpectra.begin() + static_cast<ptrdiff_t> (static_cast<size_t> (partition) * 2 * static_cast<size_t> (fftSize)));
    }

    partitionsReady = true;
}

void ConvolutionProcessor::updateChannelHistories (const float* const* channels, int numChannelsIn, int numSamples) noexcept
{
    const auto irLength = processedImpulseResponse.size();

    if (irLength == 0)
        return;

    const auto historyLength = irLength + static_cast<size_t> (numSamples) - 1;

    for (int channel = 0; channel < numChannelsIn; ++channel)
    {
        if (channels[channel] == nullptr)
            continue;

        auto& history = channelHistories[channel];

        if (history.size() < historyLength)
            history.assign (historyLength, 0.0f);

        std::memmove (history.data(), history.data() + numSamples, (historyLength - static_cast<size_t> (numSamples)) * sizeof (float));
        std::memcpy (history.data() + (historyLength - static_cast<size_t> (numSamples)), channels[channel], static_cast<size_t> (numSamples) * sizeof (float));
    }
}

void ConvolutionProcessor::processWithDirectConvolution (float* const* channels, int numChannelsIn, int numSamples) noexcept
{
    const auto& ir = processedImpulseResponse;

    if (ir.empty())
        return;

    updateChannelHistories (channels, numChannelsIn, numSamples);

    const int irLength = static_cast<int> (ir.size());

    for (int channel = 0; channel < numChannelsIn; ++channel)
    {
        if (channels[channel] == nullptr)
            continue;

        const auto& history = channelHistories[channel];
        const auto historyStart = history.size() - static_cast<size_t> (numSamples);

        for (int n = 0; n < numSamples; ++n)
        {
            float accumulated = 0.0f;

            for (int k = 0; k < irLength; ++k)
                accumulated += ir[static_cast<size_t> (k)] * history[historyStart + static_cast<size_t> (n - k)];

            channels[channel][n] = accumulated;
        }
    }
}

void ConvolutionProcessor::process (float* const* channels, int numChannelsIn, int numSamples) noexcept
{
    if (numChannelsIn > maxChannels)
        numChannelsIn = maxChannels;

    numChannels = numChannelsIn;

    if (!partitionsReady || numSamples != partitionSize)
    {
        processWithDirectConvolution (channels, numChannelsIn, numSamples);
        return;
    }

    updateChannelHistories (channels, numChannelsIn, numSamples);

    for (int channel = 0; channel < numChannelsIn; ++channel)
    {
        if (channels[channel] == nullptr)
            continue;

        std::memmove (inputHistory.data(), inputHistory.data() + partitionSize,
                      static_cast<size_t> (fftSize - partitionSize) * sizeof (float));
        std::memcpy (inputHistory.data() + (fftSize - partitionSize), channels[channel],
                     static_cast<size_t> (partitionSize) * sizeof (float));

        std::fill (fftBuffer.begin() + fftSize, fftBuffer.end(), 0.0f);
        std::copy (inputHistory.begin(), inputHistory.end(), fftBuffer.begin());

        fft->performRealOnlyForwardTransform (fftBuffer.data());

        std::fill (accumulator.begin(), accumulator.end(), 0.0f);

        const auto* inputSpectrum = fftBuffer.data();

        for (int partition = 0; partition < numPartitions; ++partition)
        {
            const auto* irPartition = irSpectra.data() + static_cast<size_t> (partition) * 2 * static_cast<size_t> (fftSize);

            for (int bin = 0; bin < fftSize; ++bin)
            {
                const size_t index = static_cast<size_t> (2 * bin);

                const float xr = inputSpectrum[index];
                const float xi = inputSpectrum[index + 1];
                const float hr = irPartition[index];
                const float hi = irPartition[index + 1];

                accumulator[index]     += hr * xr - hi * xi;
                accumulator[index + 1] += hr * xi + hi * xr;
            }
        }

        std::copy (accumulator.begin(), accumulator.end(), fftBuffer.begin());

        fft->performRealOnlyInverseTransform (fftBuffer.data());

        std::memcpy (channels[channel], fftBuffer.data() + (fftSize - partitionSize),
                     static_cast<size_t> (partitionSize) * sizeof (float));
    }
}
} // namespace daw
