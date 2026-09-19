#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

#include "AudioProcessor.h"

namespace daw
{
class PlaybackProcessor final : public AudioProcessor
{
public:
    juce::String getName() const override { return "Playback"; }

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

    std::int64_t getLengthFrames() const noexcept;
    double getPositionSeconds() const noexcept;
    double getLengthSeconds() const noexcept;

    bool isPrepared() const noexcept { return prepareReady.load(); }
    const float* getPreparedData() const noexcept;
    std::int64_t getPreparedFrames() const noexcept;
    int getPreparedChannels() const noexcept;

    void prepare (double sampleRate, int bufferSize) noexcept override;
    void process (float* const* channels, int numChannels, int numSamples) noexcept override;
    void reset() noexcept override;

private:
    static constexpr int maxChannels = 2;

    struct PreparedData
    {
        std::vector<float> data;
        std::int64_t frames = 0;
        int channels = 0;
        double sampleRate = 0.0;
    };

    class PrepareThread final : public juce::Thread
    {
    public:
        explicit PrepareThread (PlaybackProcessor& ownerToUse)
            : juce::Thread ("PlaybackPreparation"), owner (ownerToUse)
        {
        }

        void run() override;

    private:
        PlaybackProcessor& owner;
    };

    bool buildPendingPreparation();

    std::vector<float> sourceData;
    double sourceSampleRate = 0.0;
    int sourceChannels = 0;
    juce::String loadedFilePath;

    PreparedData prepared[2];
    std::atomic<int> activeIndex { 0 };
    std::atomic<bool> prepareReady { false };
    std::atomic<bool> preparePending { false };
    std::atomic<double> targetSampleRate { 0.0 };

    std::unique_ptr<PrepareThread> prepareThread;

    std::atomic<bool> playing { false };
    std::atomic<std::int64_t> position { 0 };
    std::atomic<double> pendingPositionSeconds { -1.0 };

    int numChannels = 0;
};
} // namespace daw
