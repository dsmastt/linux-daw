#pragma once

#include <memory>
#include <vector>

#include <juce_core/juce_core.h>

#include "DistortionShaper.h"
#include "Track.h"
#include "Transport.h"

namespace juce
{
class AudioPluginInstance;
struct PluginDescription;
} // namespace juce

namespace daw
{
class PlaybackProcessor;

class AudioGraph
{
public:
    static constexpr int numTracks = 4;

    AudioGraph();
    ~AudioGraph();

    Track& getTrack (int index) { return *tracks[static_cast<size_t> (index)]; }
    const Track& getTrack (int index) const { return *tracks[static_cast<size_t> (index)]; }

    // Convenience accessors for the input track (track 0).
    void setGainDb (float gainDb) noexcept;
    float getGainDb() const noexcept;

    void setGateThresholdDb (float thresholdDb) noexcept;
    float getGateThresholdDb() const noexcept;
    void setGateAttackMs (float attackMs) noexcept;
    float getGateAttackMs() const noexcept;
    void setGateReleaseMs (float releaseMs) noexcept;
    float getGateReleaseMs() const noexcept;

    void setEqBandParameters (int band, double frequencyHz, float gainDb, float q) noexcept;
    int getNumEqBands() const noexcept;
    double getEqBandFrequency (int band) const noexcept;
    float getEqBandGain (int band) const noexcept;
    float getEqBandQ (int band) const noexcept;

    void setCompressorThresholdDb (float thresholdDb) noexcept;
    void setCompressorRatio (float ratio) noexcept;
    void setCompressorAttackMs (float attackMs) noexcept;
    void setCompressorReleaseMs (float releaseMs) noexcept;
    void setCompressorMakeupGainDb (float makeupGainDb) noexcept;

    void setDistortionDriveDb (float driveDb) noexcept;
    void setDistortionTone (float tone) noexcept;
    void setDistortionMode (DistortionMode mode) noexcept;

    juce::String loadImpulseResponse (const juce::File& file);
    void clearImpulseResponse() noexcept;
    bool hasImpulseResponse() const noexcept;
    int getImpulseResponseLength() const noexcept;

    void setReverbWetDry (float wetDry) noexcept;
    void setReverbDecay (float decay) noexcept;
    void setReverbSize (float size) noexcept;

    juce::String loadVst3Plugin (const juce::PluginDescription& description, double sampleRate, int bufferSize);
    void unloadVst3Plugin() noexcept;
    bool isVst3PluginLoaded() const noexcept;
    juce::String getVst3PluginName() const noexcept;
    void setVst3PluginBypassed (bool shouldBeBypassed) noexcept;
    bool isVst3PluginBypassed() const noexcept;
    juce::AudioPluginInstance* getVst3PluginInstance() const noexcept;

    // Playback (graph-level, mixed into the master output).
    juce::String loadPlaybackFile (const juce::File& file);
    juce::String getPlaybackFilePath() const noexcept;
    void playPlayback() noexcept;
    void pausePlayback() noexcept;
    void stopPlayback() noexcept;
    bool isPlaybackPlaying() const noexcept;
    double getPlaybackPositionSeconds() const noexcept;
    double getPlaybackLengthSeconds() const noexcept;
    std::int64_t getPlaybackLengthFrames() const noexcept;
    void setPlaybackPositionSeconds (double seconds) noexcept;

    // Transport.
    Transport& getTransport() noexcept { return transport; }
    const Transport& getTransport() const noexcept { return transport; }
    void playTransport() noexcept;
    void pauseTransport() noexcept;
    void stopTransport() noexcept;

    int getNumProcessors() const noexcept;

    void resetToDefaults() noexcept;

    void prepare (double sampleRate, int bufferSize) noexcept;
    void reset() noexcept;

    void processWithInput (const float* const* inputChannels,
                           float* const* outputChannels,
                           int numChannels,
                           int numSamples,
                           juce::MidiBuffer& midiEvents) noexcept;

private:
    std::vector<std::unique_ptr<Track>> tracks;
    std::unique_ptr<PlaybackProcessor> playbackProcessor;
    Transport transport;
};
} // namespace daw
