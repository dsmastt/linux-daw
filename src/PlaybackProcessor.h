#pragma once

#include <atomic>
#include <cstdint>
#include <vector>

#include <juce_core/juce_core.h>

#include "AudioProcessor.h"

namespace daw
{
class PlaybackProcessor final : public AudioProcessor
{
public:
    juce::String loadFile (const juce::File& file);
    void clearFile() noexcept;

    juce::String getLoadedFilePath() const noexcept { return loadedFilePath; }

    void play() noexcept;
    void pause() noexcept;
    void stop() noexcept;

    bool isPlaying() const noexcept { return playing.load(); }

    std::int64_t getPositionFrames() const noexcept { return position.load(); }
    void setPositionFrames (std::int64_t frames) noexcept;
    void setPositionSeconds (double seconds) noexcept;

    std::int64_t getLengthFrames() const noexcept { return playbackFrames; }
    double getPositionSeconds() const noexcept;
    double getLengthSeconds() const noexcept;

    bool isPrepared() const noexcept { return prepared; }
    const float* getPreparedData() const noexcept { return playbackData.empty() ? nullptr : playbackData.data(); }
    std::int64_t getPreparedFrames() const noexcept { return playbackFrames; }
    int getPreparedChannels() const noexcept { return playbackChannels; }

    void prepare (double sampleRate, int bufferSize) noexcept override;
    void process (float* const* channels, int numChannels, int numSamples) noexcept override;
    void reset() noexcept override;

private:
    static constexpr int maxChannels = 2;

    std::vector<float> sourceData;
    double sourceSampleRate = 0.0;
    int sourceChannels = 0;
    juce::String loadedFilePath;

    std::vector<float> playbackData;
    int playbackChannels = 0;
    std::int64_t playbackFrames = 0;
    double playbackSampleRate = 0.0;

    std::atomic<bool> playing { false };
    std::atomic<std::int64_t> position { 0 };
    std::atomic<double> pendingPositionSeconds { -1.0 };

    bool prepared = false;
    int numChannels = 0;
};
} // namespace daw
