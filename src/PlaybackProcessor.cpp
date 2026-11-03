#include "PlaybackProcessor.h"

#include <algorithm>
#include <cmath>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>

namespace daw
{
juce::String PlaybackProcessor::loadFile (const juce::File& file)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));

    if (reader == nullptr)
        return "Could not read audio file: " + file.getFullPathName();

    const auto numSourceChannels = static_cast<int> (reader->numChannels);
    const auto numSourceFrames = reader->lengthInSamples;

    juce::AudioBuffer<float> buffer (numSourceChannels, static_cast<int> (numSourceFrames));
    reader->read (&buffer, 0, static_cast<int> (numSourceFrames), 0, true, true);

    std::vector<float> loaded (static_cast<size_t> (numSourceFrames * numSourceChannels), 0.0f);

    for (int channel = 0; channel < numSourceChannels; ++channel)
    {
        const auto* readPointer = buffer.getReadPointer (channel);

        for (std::int64_t frame = 0; frame < numSourceFrames; ++frame)
            loaded[static_cast<size_t> (frame * numSourceChannels + channel)] = readPointer[frame];
    }

    {
        const juce::SpinLock::ScopedLockType lock (sourceLock);

        sourceData = std::move (loaded);
        sourceSampleRate = reader->sampleRate;
        sourceChannels = numSourceChannels;
        loadedFilePath = file.getFullPathName();
    }

    playing.store (false, std::memory_order_relaxed);
    position.store (0, std::memory_order_relaxed);
    pendingPositionSeconds.store (-1.0, std::memory_order_relaxed);

    // Stop using the previous buffer, but keep it alive: the audio thread
    // may still be reading it. The prepared slots are only replaced by the
    // preparation thread after it has built a complete new buffer.
    prepareReady.store (false, std::memory_order_release);

    if (configuredSampleRate > 0.0)
        prepare (configuredSampleRate, configuredBufferSize);

    return {};
}

void PlaybackProcessor::clearFile() noexcept
{
    {
        const juce::SpinLock::ScopedLockType lock (sourceLock);

        sourceData.clear();
        sourceSampleRate = 0.0;
        sourceChannels = 0;
        loadedFilePath.clear();
    }

    playing.store (false, std::memory_order_relaxed);
    position.store (0, std::memory_order_relaxed);
    pendingPositionSeconds.store (-1.0, std::memory_order_relaxed);

    // The audio thread may still be reading the prepared buffer, so leave
    // the slots intact and only mark them unavailable.
    prepareReady.store (false, std::memory_order_release);
}

void PlaybackProcessor::play() noexcept
{
    playing.store (true, std::memory_order_relaxed);
}

void PlaybackProcessor::pause() noexcept
{
    playing.store (false, std::memory_order_relaxed);
}

void PlaybackProcessor::stop() noexcept
{
    playing.store (false, std::memory_order_relaxed);
    position.store (0, std::memory_order_relaxed);
}

void PlaybackProcessor::setPositionFrames (std::int64_t frames) noexcept
{
    position.store (std::clamp<std::int64_t> (frames, 0, getLengthFrames()), std::memory_order_relaxed);
}

void PlaybackProcessor::setPositionSeconds (double seconds) noexcept
{
    pendingPositionSeconds.store (seconds, std::memory_order_relaxed);

    const auto& active = prepared[activeIndex.load (std::memory_order_acquire)];

    if (active.sampleRate > 0.0)
        setPositionFrames (static_cast<std::int64_t> (seconds * active.sampleRate));
}

std::int64_t PlaybackProcessor::getLengthFrames() const noexcept
{
    return prepareReady.load (std::memory_order_acquire)
             ? prepared[activeIndex.load (std::memory_order_acquire)].frames
             : 0;
}

double PlaybackProcessor::getPositionSeconds() const noexcept
{
    if (!prepareReady.load (std::memory_order_acquire))
        return 0.0;

    const auto& active = prepared[activeIndex.load (std::memory_order_acquire)];

    return active.sampleRate > 0.0
             ? static_cast<double> (position.load()) / active.sampleRate
             : 0.0;
}

double PlaybackProcessor::getLengthSeconds() const noexcept
{
    if (!prepareReady.load (std::memory_order_acquire))
        return 0.0;

    const auto& active = prepared[activeIndex.load (std::memory_order_acquire)];

    return active.sampleRate > 0.0
             ? static_cast<double> (active.frames) / active.sampleRate
             : 0.0;
}

const float* PlaybackProcessor::getPreparedData() const noexcept
{
    if (!prepareReady.load (std::memory_order_acquire))
        return nullptr;

    const auto& active = prepared[activeIndex.load (std::memory_order_acquire)];
    return active.data.empty() ? nullptr : active.data.data();
}

std::int64_t PlaybackProcessor::getPreparedFrames() const noexcept
{
    return prepareReady.load (std::memory_order_acquire)
             ? prepared[activeIndex.load (std::memory_order_acquire)].frames
             : 0;
}

int PlaybackProcessor::getPreparedChannels() const noexcept
{
    return prepareReady.load (std::memory_order_acquire)
             ? prepared[activeIndex.load (std::memory_order_acquire)].channels
             : 0;
}

std::int64_t PlaybackProcessor::getSourceFrames() const noexcept
{
    const juce::SpinLock::ScopedLockType lock (sourceLock);

    return sourceChannels > 0 ? static_cast<std::int64_t> (sourceData.size()) / sourceChannels : 0;
}

int PlaybackProcessor::getSourceChannels() const noexcept
{
    const juce::SpinLock::ScopedLockType lock (sourceLock);
    return sourceChannels;
}

void PlaybackProcessor::prepare (double sampleRate, int bufferSize) noexcept
{
    numChannels = maxChannels;

    if (sampleRate > 0.0)
    {
        configuredSampleRate = sampleRate;
        configuredBufferSize = bufferSize;
    }

    {
        const juce::SpinLock::ScopedLockType lock (sourceLock);

        if (sourceData.empty() || sourceChannels <= 0 || sampleRate <= 0.0)
        {
            prepareReady.store (false, std::memory_order_release);
            return;
        }
    }

    if (prepareReady.load (std::memory_order_acquire)
        && juce::approximatelyEqual (prepared[activeIndex.load (std::memory_order_acquire)].sampleRate, sampleRate))
        return;

    targetSampleRate.store (sampleRate, std::memory_order_relaxed);
    preparePending.store (true, std::memory_order_release);

    if (prepareThread == nullptr)
    {
        prepareThread = std::make_unique<PrepareThread> (*this);
        prepareThread->startThread();
    }
}

bool PlaybackProcessor::buildPendingPreparation()
{
    if (!preparePending.exchange (false, std::memory_order_acquire))
        return false;

    const double targetRate = targetSampleRate.load (std::memory_order_relaxed);

    std::vector<float> source;
    double sourceRate = 0.0;
    int sourceChannelCount = 0;

    {
        const juce::SpinLock::ScopedLockType lock (sourceLock);

        if (sourceData.empty() || sourceChannels <= 0 || targetRate <= 0.0)
            return false;

        source = sourceData;
        sourceRate = sourceSampleRate;
        sourceChannelCount = sourceChannels;
    }

    const std::int64_t sourceFrames = static_cast<std::int64_t> (source.size()) / sourceChannelCount;
    const double ratio = sourceRate / targetRate;
    const auto targetFrames = std::max<std::int64_t> (1, static_cast<std::int64_t> (std::llround (sourceFrames / ratio)));

    std::vector<float> resampled (static_cast<size_t> (targetFrames * sourceChannelCount), 0.0f);
    juce::CatmullRomInterpolator interpolator;

    for (int channel = 0; channel < sourceChannelCount; ++channel)
    {
        std::vector<float> channelInput (static_cast<size_t> (sourceFrames), 0.0f);

        for (std::int64_t frame = 0; frame < sourceFrames; ++frame)
            channelInput[static_cast<size_t> (frame)] = source[static_cast<size_t> (frame * sourceChannelCount + channel)];

        std::vector<float> channelOutput (static_cast<size_t> (targetFrames), 0.0f);

        interpolator.process (ratio, channelInput.data(), channelOutput.data(), static_cast<int> (targetFrames));

        for (std::int64_t frame = 0; frame < targetFrames; ++frame)
            resampled[static_cast<size_t> (frame * sourceChannelCount + channel)] = channelOutput[static_cast<size_t> (frame)];
    }

    const int inactive = 1 - activeIndex.load (std::memory_order_relaxed);

    auto& slot = prepared[inactive];
    slot.data = std::move (resampled);
    slot.frames = targetFrames;
    slot.channels = sourceChannelCount;
    slot.sampleRate = targetRate;

    const double pendingSeconds = pendingPositionSeconds.load (std::memory_order_relaxed);

    if (pendingSeconds >= 0.0)
        pendingPositionSeconds.store (-1.0, std::memory_order_relaxed);

    activeIndex.store (inactive, std::memory_order_release);
    prepareReady.store (true, std::memory_order_release);

    return true;
}

void PlaybackProcessor::PrepareThread::run()
{
    while (!threadShouldExit())
    {
        if (owner.buildPendingPreparation())
            continue;

        juce::Thread::sleep (5);
    }
}

void PlaybackProcessor::reset() noexcept
{
    numChannels = 0;
}

void PlaybackProcessor::process (float* const* channels, int numChannelsIn, int numSamples) noexcept
{
    if (!prepareReady.load (std::memory_order_acquire) || !playing.load (std::memory_order_relaxed))
        return;

    const auto& active = prepared[activeIndex.load (std::memory_order_acquire)];

    if (active.frames <= 0 || active.data.empty())
        return;

    auto currentPosition = position.load (std::memory_order_relaxed);

    if (currentPosition >= active.frames)
    {
        playing.store (false, std::memory_order_relaxed);
        return;
    }

    const int framesToCopy = static_cast<int> (std::min<std::int64_t> (numSamples, active.frames - currentPosition));
    const int channelsToWrite = std::min (numChannelsIn, active.channels);

    for (int frame = 0; frame < framesToCopy; ++frame)
    {
        for (int channel = 0; channel < channelsToWrite; ++channel)
        {
            if (channels[channel] == nullptr)
                continue;

            channels[channel][frame] += active.data[static_cast<size_t> ((currentPosition + frame) * active.channels + channel)];
        }
    }

    currentPosition += framesToCopy;
    position.store (currentPosition, std::memory_order_relaxed);

    if (currentPosition >= active.frames)
        playing.store (false, std::memory_order_relaxed);
}
} // namespace daw
