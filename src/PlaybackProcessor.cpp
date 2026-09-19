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

    sourceData = std::move (loaded);
    sourceSampleRate = reader->sampleRate;
    sourceChannels = numSourceChannels;
    loadedFilePath = file.getFullPathName();

    playing.store (false, std::memory_order_relaxed);
    position.store (0, std::memory_order_relaxed);
    pendingPositionSeconds.store (-1.0, std::memory_order_relaxed);
    prepared = false;

    return {};
}

void PlaybackProcessor::clearFile() noexcept
{
    sourceData.clear();
    sourceSampleRate = 0.0;
    sourceChannels = 0;
    loadedFilePath.clear();

    playbackData.clear();
    playbackChannels = 0;
    playbackFrames = 0;
    playbackSampleRate = 0.0;

    playing.store (false, std::memory_order_relaxed);
    position.store (0, std::memory_order_relaxed);
    pendingPositionSeconds.store (-1.0, std::memory_order_relaxed);
    prepared = false;
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
    position.store (std::clamp<std::int64_t> (frames, 0, playbackFrames), std::memory_order_relaxed);
}

void PlaybackProcessor::setPositionSeconds (double seconds) noexcept
{
    pendingPositionSeconds.store (seconds, std::memory_order_relaxed);

    if (playbackSampleRate > 0.0)
        setPositionFrames (static_cast<std::int64_t> (seconds * playbackSampleRate));
}

double PlaybackProcessor::getPositionSeconds() const noexcept
{
    return playbackSampleRate > 0.0
             ? static_cast<double> (position.load()) / playbackSampleRate
             : 0.0;
}

double PlaybackProcessor::getLengthSeconds() const noexcept
{
    return playbackSampleRate > 0.0
             ? static_cast<double> (playbackFrames) / playbackSampleRate
             : 0.0;
}

void PlaybackProcessor::prepare (double sampleRate, int) noexcept
{
    numChannels = 2;

    if (sourceData.empty() || sourceChannels <= 0 || sampleRate <= 0.0)
    {
        prepared = false;
        return;
    }

    if (!prepared || !juce::approximatelyEqual (playbackSampleRate, sampleRate))
    {
        const std::int64_t sourceFrames = static_cast<std::int64_t> (sourceData.size()) / sourceChannels;
        const double ratio = sourceSampleRate / sampleRate;
        const auto targetFrames = std::max<std::int64_t> (1, static_cast<std::int64_t> (std::llround (sourceFrames / ratio)));

        std::vector<float> resampled (static_cast<size_t> (targetFrames * sourceChannels), 0.0f);
        juce::CatmullRomInterpolator interpolator;

        for (int channel = 0; channel < sourceChannels; ++channel)
        {
            std::vector<float> channelInput (static_cast<size_t> (sourceFrames), 0.0f);

            for (std::int64_t frame = 0; frame < sourceFrames; ++frame)
                channelInput[static_cast<size_t> (frame)] = sourceData[static_cast<size_t> (frame * sourceChannels + channel)];

            std::vector<float> channelOutput (static_cast<size_t> (targetFrames), 0.0f);

            interpolator.process (ratio, channelInput.data(), channelOutput.data(), static_cast<int> (targetFrames));

            for (std::int64_t frame = 0; frame < targetFrames; ++frame)
                resampled[static_cast<size_t> (frame * sourceChannels + channel)] = channelOutput[static_cast<size_t> (frame)];
        }

        playbackData = std::move (resampled);
        playbackChannels = sourceChannels;
        playbackFrames = targetFrames;
        playbackSampleRate = sampleRate;

        prepared = true;
    }

    const double pendingSeconds = pendingPositionSeconds.load();

    if (pendingSeconds >= 0.0)
    {
        pendingPositionSeconds.store (-1.0, std::memory_order_relaxed);
        setPositionFrames (static_cast<std::int64_t> (pendingSeconds * playbackSampleRate));
    }
}

void PlaybackProcessor::reset() noexcept
{
    numChannels = 0;
}

void PlaybackProcessor::process (float* const* channels, int numChannelsIn, int numSamples) noexcept
{
    if (!prepared || !playing.load (std::memory_order_relaxed) || playbackFrames <= 0)
        return;

    auto currentPosition = position.load (std::memory_order_relaxed);

    if (currentPosition >= playbackFrames)
    {
        playing.store (false, std::memory_order_relaxed);
        return;
    }

    const int framesToCopy = static_cast<int> (std::min<std::int64_t> (numSamples, playbackFrames - currentPosition));
    const int channelsToWrite = std::min (numChannelsIn, playbackChannels);

    for (int frame = 0; frame < framesToCopy; ++frame)
    {
        for (int channel = 0; channel < channelsToWrite; ++channel)
        {
            if (channels[channel] == nullptr)
                continue;

            channels[channel][frame] += playbackData[static_cast<size_t> ((currentPosition + frame) * playbackChannels + channel)];
        }
    }

    currentPosition += framesToCopy;
    position.store (currentPosition, std::memory_order_relaxed);

    if (currentPosition >= playbackFrames)
        playing.store (false, std::memory_order_relaxed);
}
} // namespace daw
