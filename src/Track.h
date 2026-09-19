#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <juce_core/juce_core.h>

#include "Clip.h"
#include "DistortionShaper.h"
#include "ProcessorChain.h"

namespace juce
{
class AudioPluginInstance;
struct PluginDescription;
} // namespace juce

namespace daw
{
class CompressorProcessor;
class ConvolutionProcessor;
class DistortionProcessor;
class GainProcessor;
class NoiseGateProcessor;
class ParametricEqProcessor;
class ReverbProcessor;
class Vst3HostProcessor;

class Track
{
public:
    Track();
    ~Track();

    void setVolumeDb (float volumeDb) noexcept;
    float getVolumeDb() const noexcept;
    void setPan (float pan) noexcept;
    float getPan() const noexcept;
    void setMuted (bool shouldBeMuted) noexcept;
    bool isMuted() const noexcept;
    void setSoloed (bool shouldBeSoloed) noexcept;
    bool isSoloed() const noexcept;

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

    int getNumProcessors() const noexcept { return chain.size(); }

    int addClip (std::int64_t startSamples, std::int64_t lengthSamples, std::int64_t sourceOffsetSamples = 0);
    void removeClip (int clipIndex) noexcept;
    void moveClip (int clipIndex, std::int64_t newStartSamples) noexcept;
    void resizeClip (int clipIndex, std::int64_t newLengthSamples) noexcept;
    int getNumClips() const noexcept { return clipCount.load(); }
    const Clip& getClip (int clipIndex) const noexcept { return clips[static_cast<size_t> (clipIndex)]; }

    void prepare (double sampleRate, int bufferSize) noexcept;
    void reset() noexcept;

    void process (const float* const* input,
                  float* const* output,
                  int numChannels,
                  int numSamples,
                  bool anyTrackSoloed,
                  std::int64_t transportPositionSamples,
                  const ClipSource& clipSource,
                  juce::MidiBuffer& midiEvents) noexcept;

private:
    static constexpr int maxChannels = 2;
    static constexpr int maxBlockFrames = 2048;

    ProcessorChain chain;
    GainProcessor* gainProcessor = nullptr;
    NoiseGateProcessor* gateProcessor = nullptr;
    ParametricEqProcessor* eqProcessor = nullptr;
    CompressorProcessor* compressorProcessor = nullptr;
    DistortionProcessor* distortionProcessor = nullptr;
    Vst3HostProcessor* vst3Processor = nullptr;
    ConvolutionProcessor* convolutionProcessor = nullptr;
    ReverbProcessor* reverbProcessor = nullptr;

    std::atomic<float> volumeLinear { 1.0f };
    float volumeDb = 0.0f;
    std::atomic<float> panValue { 0.0f };
    std::atomic<bool> muted { false };
    std::atomic<bool> soloed { false };

    std::vector<std::vector<float>> scratch;
    int numChannels = 0;

    Clip clips[maxClipsPerTrack];
    std::atomic<int> clipCount { 0 };
};
} // namespace daw
