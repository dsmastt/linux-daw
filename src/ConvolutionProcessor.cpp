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
    impulseResponseFilePath = file.getFullPathName();
    hasImpulseResponse_.store (true, std::memory_order_release);
    prepareReady.store (false, std::memory_order_release);

    prepared[0] = PreparedPartitions {};
    prepared[1] = PreparedPartitions {};

    return {};
}

void ConvolutionProcessor::clearImpulseResponse()
{
    hasImpulseResponse_.store (false, std::memory_order_release);
    impulseResponse.clear();
    impulseResponseSampleRate = 0.0;
    impulseResponseFilePath.clear();
    prepareReady.store (false, std::memory_order_release);

    prepared[0] = PreparedPartitions {};
    prepared[1] = PreparedPartitions {};
}

int ConvolutionProcessor::getImpulseResponseLength() const noexcept
{
    const auto& active = prepared[activeIndex.load (std::memory_order_acquire)];

    return active.processedImpulseResponse.empty()
             ? static_cast<int> (impulseResponse.size())
             : static_cast<int> (active.processedImpulseResponse.size());
}

void ConvolutionProcessor::prepare (double sampleRate, int bufferSize) noexcept
{
    if (!hasImpulseResponse_.load (std::memory_order_acquire) || impulseResponse.empty()
        || bufferSize <= 0 || sampleRate <= 0.0)
    {
        prepareReady.store (false, std::memory_order_release);
        return;
    }

    if (prepareReady.load (std::memory_order_acquire)
        && prepared[activeIndex.load (std::memory_order_acquire)].partitionSize == bufferSize)
        return;

    const int expectedFftSize = juce::nextPowerOfTwo (2 * bufferSize);

    if (static_cast<int> (inputHistory.size()) != expectedFftSize)
    {
        inputHistory.assign (static_cast<size_t> (expectedFftSize), 0.0f);
        fftBuffer.assign (static_cast<size_t> (2 * expectedFftSize), 0.0f);
        accumulator.assign (static_cast<size_t> (2 * expectedFftSize), 0.0f);
    }

    targetSampleRate.store (sampleRate, std::memory_order_relaxed);
    targetBufferSize.store (bufferSize, std::memory_order_relaxed);
    preparePending.store (true, std::memory_order_release);

    if (prepareThread == nullptr)
    {
        prepareThread = std::make_unique<PrepareThread> (*this);
        prepareThread->startThread();
    }
}

bool ConvolutionProcessor::buildPendingPreparation()
{
    if (!preparePending.exchange (false, std::memory_order_acquire))
        return false;

    const double sampleRate = targetSampleRate.load (std::memory_order_relaxed);
    const int bufferSize = targetBufferSize.load (std::memory_order_relaxed);

    if (!hasImpulseResponse_.load (std::memory_order_acquire) || impulseResponse.empty()
        || bufferSize <= 0 || sampleRate <= 0.0)
        return false;

    const int inactive = 1 - activeIndex.load (std::memory_order_relaxed);

    auto& target = prepared[inactive];
    target = PreparedPartitions {};

    buildInto (target, sampleRate, bufferSize);

    activeIndex.store (inactive, std::memory_order_release);
    prepareReady.store (true, std::memory_order_release);

    return true;
}

void ConvolutionProcessor::buildInto (PreparedPartitions& target, double sampleRate, int bufferSize) noexcept
{
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

    target.processedImpulseResponse = ir;

    target.partitionSize = bufferSize;
    target.fftSize = juce::nextPowerOfTwo (2 * bufferSize);
    target.numPartitions = static_cast<int> ((ir.size() + static_cast<size_t> (bufferSize) - 1) / static_cast<size_t> (bufferSize));

    const auto order = juce::findHighestSetBit (static_cast<uint32_t> (target.fftSize));

    target.fft = std::make_unique<juce::dsp::FFT> (order);

    target.irSpectra.assign (static_cast<size_t> (target.numPartitions) * 2 * static_cast<size_t> (target.fftSize), 0.0f);

    std::vector<float> workBuffer (static_cast<size_t> (2 * target.fftSize), 0.0f);

    for (int partition = 0; partition < target.numPartitions; ++partition)
    {
        std::fill (workBuffer.begin(), workBuffer.end(), 0.0f);

        const auto offset = static_cast<size_t> (partition) * static_cast<size_t> (bufferSize);
        const auto count = std::min (ir.size() - offset, static_cast<size_t> (bufferSize));
        std::copy_n (ir.data() + offset, count, workBuffer.begin());

        target.fft->performRealOnlyForwardTransform (workBuffer.data());

        std::copy (workBuffer.begin(), workBuffer.end(),
                   target.irSpectra.begin() + static_cast<ptrdiff_t> (static_cast<size_t> (partition) * 2 * static_cast<size_t> (target.fftSize)));
    }
}

void ConvolutionProcessor::PrepareThread::run()
{
    while (!threadShouldExit())
    {
        if (owner.buildPendingPreparation())
            continue;

        juce::Thread::sleep (5);
    }
}

void ConvolutionProcessor::reset() noexcept
{
    std::fill (inputHistory.begin(), inputHistory.end(), 0.0f);
    std::fill (fftBuffer.begin(), fftBuffer.end(), 0.0f);
    std::fill (accumulator.begin(), accumulator.end(), 0.0f);

    numChannels = 0;
}

void ConvolutionProcessor::process (float* const* channels, int numChannelsIn, int numSamples) noexcept
{
    if (numChannelsIn > maxChannels)
        numChannelsIn = maxChannels;

    numChannels = numChannelsIn;

    if (!prepareReady.load (std::memory_order_acquire))
        return;

    const auto& active = prepared[activeIndex.load (std::memory_order_acquire)];

    if (active.fft == nullptr || active.irSpectra.empty() || numSamples != active.partitionSize)
        return;

    const int partitionSize = active.partitionSize;
    const int fftSize = active.fftSize;
    const int numPartitions = active.numPartitions;

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

        active.fft->performRealOnlyForwardTransform (fftBuffer.data());

        std::fill (accumulator.begin(), accumulator.end(), 0.0f);

        const auto* inputSpectrum = fftBuffer.data();

        for (int partition = 0; partition < numPartitions; ++partition)
        {
            const auto* irPartition = active.irSpectra.data() + static_cast<size_t> (partition) * 2 * static_cast<size_t> (fftSize);

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

        active.fft->performRealOnlyInverseTransform (fftBuffer.data());

        std::memcpy (channels[channel], fftBuffer.data() + (fftSize - partitionSize),
                     static_cast<size_t> (partitionSize) * sizeof (float));
    }
}
} // namespace daw
